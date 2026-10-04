/*
** CrabeLoader
** File description:
** Records a frame of mod ImGui calls on the script thread and replays it on the render thread.
** A scope is closed only if its Begin was accepted, which is why needsEnd is tracked per command.
** Registers no Lua binding; that is src/presentation/imgui_bindings.cpp.
**
** Authors: @LucasLhomme
*/

#include "presentation/draw_buffer.hpp"
#include "imgui/imgui.h"

#include <cfloat>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <utility>

namespace crabe::presentation {

namespace {

    enum class ScopeKind : std::uint8_t {
        Window,
        Child,
        TabBar,
        TabItem,
    };

    // `needsEnd` records whether the matching ImGui Begin was actually issued.
    // Windows and children always need their End; tab bars and tab items must
    // only be closed when ImGui accepted them.
    struct Scope {
        ScopeKind kind = ScopeKind::Window;
        bool needsEnd = false;
    };

    bool opensScope(DrawOp op)
    {
        switch (op) {
            case DrawOp::Begin:
            case DrawOp::End:
            case DrawOp::BeginChild:
            case DrawOp::EndChild:
            case DrawOp::BeginTabBar:
            case DrawOp::EndTabBar:
            case DrawOp::BeginTabItem:
            case DrawOp::EndTabItem:
                return true;
            default:
                return false;
        }
    }

    void closeScope(const Scope& scope)
    {
        if (!scope.needsEnd)
            return;

        switch (scope.kind) {
            case ScopeKind::Window:  ImGui::End(); break;
            case ScopeKind::Child:   ImGui::EndChild(); break;
            case ScopeKind::TabBar:  ImGui::EndTabBar(); break;
            case ScopeKind::TabItem: ImGui::EndTabItem(); break;
        }
    }

    ImFont* fontFor(int index)
    {
        ImFontAtlas* atlas = ImGui::GetIO().Fonts;
        if (index > 0 && index < atlas->Fonts.Size)
            return atlas->Fonts[index];
        return atlas->Fonts.Size > 0 ? atlas->Fonts[0] : ImGui::GetFont();
    }

    void popCodepoint(std::string& text)
    {
        while (!text.empty() && (static_cast<unsigned char>(text.back()) & 0xC0) == 0x80)
            text.pop_back();
        if (!text.empty())
            text.pop_back();
    }

    // Text in the box that starts at (f0, f1) and is f2 wide, aligned left (0),
    // centred (1) or right (2). Text wider than the box is cut with "...", and
    // a non-zero c1 draws a one-pixel drop shadow in that colour first.
    void drawShapeText(const DrawCommand& command)
    {
        ImFont* font = fontFor(command.i1);
        const float size = command.f3 > 0.0f ? command.f3 : font->FontSize;
        const float boxWidth = command.f2;

        std::string text = command.text;
        float width = font->CalcTextSizeA(size, FLT_MAX, 0.0f, text.c_str()).x;
        if (boxWidth > 0.0f && width > boxWidth) {
            const float ellipsis = font->CalcTextSizeA(size, FLT_MAX, 0.0f, "...").x;
            while (!text.empty() && width + ellipsis > boxWidth) {
                popCodepoint(text);
                width = font->CalcTextSizeA(size, FLT_MAX, 0.0f, text.c_str()).x;
            }
            text += "...";
            width += ellipsis;
        }

        float x = command.f0;
        if (boxWidth > 0.0f && command.i0 == 1)
            x += (boxWidth - width) * 0.5f;
        else if (boxWidth > 0.0f && command.i0 == 2)
            x += boxWidth - width;
        const ImVec2 position(std::floor(x), std::floor(command.f1));

        ImDrawList* list = ImGui::GetBackgroundDrawList();
        if (command.c1 != 0)
            list->AddText(font, size, ImVec2(position.x + 1.0f, position.y + 1.0f), command.c1, text.c_str());
        list->AddText(font, size, position, command.c0, text.c_str());
    }

    void drawShapeRect(const DrawCommand& command)
    {
        const ImVec2 from(command.f0, command.f1);
        const ImVec2 to(command.f0 + command.f2, command.f1 + command.f3);
        ImDrawList* list = ImGui::GetBackgroundDrawList();

        if (command.i0 > 0)
            list->AddRect(from, to, command.c0, command.f4, 0, static_cast<float>(command.i0));
        else
            list->AddRectFilled(from, to, command.c0, command.f4);
    }

    // c0 is the top (or left, when b0 is set) colour and c1 the opposite edge.
    void drawShapeGradient(const DrawCommand& command)
    {
        const ImVec2 from(command.f0, command.f1);
        const ImVec2 to(command.f0 + command.f2, command.f1 + command.f3);
        const ImU32 topRight = command.b0 ? command.c1 : command.c0;
        const ImU32 bottomLeft = command.b0 ? command.c0 : command.c1;
        ImGui::GetBackgroundDrawList()->AddRectFilledMultiColor(from, to, command.c0, topRight,
                                                                command.c1, bottomLeft);
    }

    void drawShapeCircle(const DrawCommand& command)
    {
        const ImVec2 centre(command.f0, command.f1);
        ImDrawList* list = ImGui::GetBackgroundDrawList();

        if (command.f4 > 0.0f)
            list->AddCircle(centre, command.f2, command.c0, 0, command.f4);
        else
            list->AddCircleFilled(centre, command.f2, command.c0);
    }

    // FNV-1a over the label, mixed with the call index. A widget keeps its id
    // only while both its label and its position in the frame hold still, so a
    // menu that changed shape never inherits the previous layout's click.
    std::uint32_t makeWidgetId(const std::string& label, std::uint32_t sequence)
    {
        std::uint32_t hash = 2166136261u;

        for (char character : label) {
            hash ^= static_cast<std::uint8_t>(character);
            hash *= 16777619u;
        }
        hash ^= sequence * 2654435761u;
        return hash ? hash : 1u;
    }

}

DrawBuffer& DrawBuffer::get()
{
    static DrawBuffer instance;
    return instance;
}

void DrawBuffer::beginFrame()
{
    _recording.clear();
    _sequence = 0;
}

std::uint32_t DrawBuffer::record(DrawCommand command)
{
    command.id = makeWidgetId(command.label, _sequence++);

    std::uint32_t id = command.id;
    _recording.push_back(std::move(command));
    return id;
}

bool DrawBuffer::recordScope(DrawCommand command, bool fallback)
{
    command.id = makeWidgetId(command.label, _sequence++);

    bool value = fallback;
    {
        std::lock_guard<std::mutex> lock(_resultsMutex);
        auto found = _results.find(command.id);
        if (found != _results.end())
            value = found->second.flag;
    }

    command.b1 = value;
    _recording.push_back(std::move(command));
    return value;
}

void DrawBuffer::endFrame()
{
    std::lock_guard<std::mutex> lock(_publishMutex);
    _published.swap(_recording);
    _hasPublished = true;
}

bool DrawBuffer::tryResult(std::uint32_t id, WidgetResult& out) const
{
    std::lock_guard<std::mutex> lock(_resultsMutex);

    auto found = _results.find(id);
    if (found == _results.end())
        return false;
    out = found->second;
    return true;
}

// Present can run several times per script tick, so clicks accumulate until the
// next frame is published, and edited values are written back into the playback
// so that replaying a list twice does not undo what the user just did.
void DrawBuffer::replay()
{
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    _displayWidth.store(display.x, std::memory_order_relaxed);
    _displayHeight.store(display.y, std::memory_order_relaxed);

    bool freshList = false;

    {
        std::lock_guard<std::mutex> lock(_publishMutex);
        if (_hasPublished) {
            _playback.swap(_published);
            _hasPublished = false;
            freshList = true;
        }
    }

    if (_playback.empty()) {
        if (freshList) {
            _accumulated.clear();
            std::lock_guard<std::mutex> lock(_resultsMutex);
            _results.clear();
        }
        return;
    }

    if (freshList)
        _accumulated.clear();

    std::unordered_map<std::uint32_t, WidgetResult>& accumulated = _accumulated;
    std::vector<Scope> scopes;
    int skipDepth = -1;

    auto popScope = [&scopes, &skipDepth](ScopeKind expected) {
        if (scopes.empty() || scopes.back().kind != expected)
            return;

        Scope scope = scopes.back();
        scopes.pop_back();
        closeScope(scope);
        if (skipDepth >= 0 && static_cast<int>(scopes.size()) <= skipDepth)
            skipDepth = -1;
    };

    for (DrawCommand& command : _playback) {
        const bool skipping = skipDepth >= 0;
        if (skipping && !opensScope(command.op))
            continue;

        switch (command.op) {
            case DrawOp::Begin: {
                if (skipping) {
                    scopes.push_back(Scope{ ScopeKind::Window, false });
                    break;
                }
                bool open = true;
                bool visible = command.b0
                    ? ImGui::Begin(command.label.c_str(), &open, static_cast<ImGuiWindowFlags>(command.i0))
                    : ImGui::Begin(command.label.c_str(), nullptr, static_cast<ImGuiWindowFlags>(command.i0));

                scopes.push_back(Scope{ ScopeKind::Window, true });
                WidgetResult& slot = accumulated[command.id];
                slot.flag = visible;
                slot.open = open;
                break;
            }
            case DrawOp::End:
                popScope(ScopeKind::Window);
                break;
            case DrawOp::BeginTabBar: {
                if (skipping) {
                    if (command.b1)
                        scopes.push_back(Scope{ ScopeKind::TabBar, false });
                    break;
                }
                bool opened = ImGui::BeginTabBar(command.label.c_str(),
                                                 static_cast<ImGuiTabBarFlags>(command.i0));
                accumulated[command.id].flag = opened;

                if (command.b1) {
                    scopes.push_back(Scope{ ScopeKind::TabBar, opened });
                    if (!opened)
                        skipDepth = static_cast<int>(scopes.size()) - 1;
                } else if (opened) {
                    ImGui::EndTabBar();
                }
                break;
            }
            case DrawOp::EndTabBar:
                popScope(ScopeKind::TabBar);
                break;
            case DrawOp::BeginTabItem: {
                if (skipping) {
                    if (command.b1)
                        scopes.push_back(Scope{ ScopeKind::TabItem, false });
                    break;
                }
                bool selected = ImGui::BeginTabItem(command.label.c_str(), nullptr,
                                                    static_cast<ImGuiTabItemFlags>(command.i0));
                accumulated[command.id].flag = selected;

                if (command.b1) {
                    scopes.push_back(Scope{ ScopeKind::TabItem, selected });
                    if (!selected)
                        skipDepth = static_cast<int>(scopes.size()) - 1;
                } else if (selected) {
                    ImGui::EndTabItem();
                }
                break;
            }
            case DrawOp::EndTabItem:
                popScope(ScopeKind::TabItem);
                break;
            case DrawOp::Text:
                ImGui::TextUnformatted(command.text.c_str());
                break;
            case DrawOp::TextColored:
                ImGui::TextColored(ImVec4(command.f0, command.f1, command.f2, command.f3),
                                   "%s", command.text.c_str());
                break;
            case DrawOp::TextDisabled:
                ImGui::TextDisabled("%s", command.text.c_str());
                break;
            case DrawOp::Button: {
                bool clicked = ImGui::Button(command.label.c_str(), ImVec2(command.f0, command.f1));
                accumulated[command.id].flag |= clicked;
                break;
            }
            case DrawOp::Selectable: {
                bool clicked = ImGui::Selectable(command.label.c_str(), command.b0,
                                                 static_cast<ImGuiSelectableFlags>(command.i0));
                accumulated[command.id].flag |= clicked;
                break;
            }
            case DrawOp::SetScrollHereY:
                ImGui::SetScrollHereY(command.f0);
                break;
            case DrawOp::Checkbox: {
                bool value = command.b0;
                ImGui::Checkbox(command.label.c_str(), &value);
                command.b0 = value;
                accumulated[command.id].flag = value;
                break;
            }
            case DrawOp::SliderFloat: {
                float value = command.f0;
                ImGui::SliderFloat(command.label.c_str(), &value, command.f1, command.f2);
                command.f0 = value;
                accumulated[command.id].number = static_cast<double>(value);
                break;
            }
            case DrawOp::SliderInt: {
                int value = command.i0;
                ImGui::SliderInt(command.label.c_str(), &value, static_cast<int>(command.f1),
                                 static_cast<int>(command.f2));
                command.i0 = value;
                accumulated[command.id].number = static_cast<double>(value);
                break;
            }
            case DrawOp::InputText: {
                std::size_t capacity = command.text.size() > 4096 ? command.text.size() : 4096;
                std::vector<char> buffer(capacity + 1, '\0');
                std::memcpy(buffer.data(), command.text.data(), command.text.size());

                bool changed = ImGui::InputText(command.label.c_str(), buffer.data(), buffer.size());
                command.text.assign(buffer.data());

                WidgetResult& slot = accumulated[command.id];
                slot.text = command.text;
                slot.flag |= changed;
                break;
            }
            case DrawOp::SameLine:
                ImGui::SameLine(command.f0, command.f1);
                break;
            case DrawOp::Separator:
                ImGui::Separator();
                break;
            case DrawOp::Spacing:
                ImGui::Spacing();
                break;
            case DrawOp::BeginChild: {
                if (skipping) {
                    scopes.push_back(Scope{ ScopeKind::Child, false });
                    break;
                }
                ImGuiChildFlags childFlags = command.b0 ? ImGuiChildFlags_Border : ImGuiChildFlags_None;
                bool visible = ImGui::BeginChild(command.label.c_str(), ImVec2(command.f0, command.f1),
                                                 childFlags, static_cast<ImGuiWindowFlags>(command.i0));

                scopes.push_back(Scope{ ScopeKind::Child, true });
                accumulated[command.id].flag = visible;
                break;
            }
            case DrawOp::EndChild:
                popScope(ScopeKind::Child);
                break;
            case DrawOp::SetNextWindowPos:
                ImGui::SetNextWindowPos(ImVec2(command.f0, command.f1), static_cast<ImGuiCond>(command.i0));
                break;
            case DrawOp::SetNextWindowSize:
                ImGui::SetNextWindowSize(ImVec2(command.f0, command.f1), static_cast<ImGuiCond>(command.i0));
                break;
            case DrawOp::IsItemClicked:
                accumulated[command.id].flag |= ImGui::IsItemClicked(static_cast<ImGuiMouseButton>(command.i0));
                break;
            case DrawOp::SetKeyboardFocusHere:
                ImGui::SetKeyboardFocusHere(command.i0);
                break;
            case DrawOp::ShapeRect:
                drawShapeRect(command);
                break;
            case DrawOp::ShapeGradient:
                drawShapeGradient(command);
                break;
            case DrawOp::ShapeLine:
                ImGui::GetBackgroundDrawList()->AddLine(ImVec2(command.f0, command.f1), ImVec2(command.f2, command.f3),
                                                        command.c0, command.f4 > 0.0f ? command.f4 : 1.0f);
                break;
            case DrawOp::ShapeCircle:
                drawShapeCircle(command);
                break;
            case DrawOp::ShapeText:
                drawShapeText(command);
                break;
        }
    }

    while (!scopes.empty()) {
        closeScope(scopes.back());
        scopes.pop_back();
    }

    {
        std::lock_guard<std::mutex> lock(_resultsMutex);
        _results = accumulated;
    }
}

void DrawBuffer::displaySize(float& width, float& height) const
{
    width = _displayWidth.load(std::memory_order_relaxed);
    height = _displayHeight.load(std::memory_order_relaxed);
}

} // namespace crabe::presentation
