/*
** CrabeLoader
** File description:
** Unit tests for the mod drawing primitives (DrawBuffer Shape* replay) and the gamepad store.
** Replays into a headless Dear ImGui context and inspects the background draw list's vertices.
** Runs offline as an independent C++23 test binary: no GPU, no game process, no Lua VM.
**
** Authors: @LucasLhomme
*/

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <windows.h>
#include <Xinput.h>

#include "imgui/imgui.h"
#include "presentation/draw_buffer.hpp"
#include "presentation/pad_state.hpp"

namespace {

using crabe::presentation::DrawBuffer;
using crabe::presentation::DrawCommand;
using crabe::presentation::DrawOp;
using crabe::presentation::PadState;
using crabe::presentation::PadStateStore;

int g_checks = 0;

void require(bool condition, const std::string& message)
{
    ++g_checks;
    if (!condition) {
        std::cerr << "[FAILED] " << message << std::endl;
        std::exit(1);
    }
}

constexpr ImU32 kRed = IM_COL32(255, 0, 0, 255);
constexpr ImU32 kBlue = IM_COL32(0, 0, 255, 200);
constexpr float kDisplayWidth = 1280.0f;
constexpr float kDisplayHeight = 720.0f;

// ---------------------------------------------------------------------------
// Headless ImGui frame
// ---------------------------------------------------------------------------

void setUpImGui()
{
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.DisplaySize = ImVec2(kDisplayWidth, kDisplayHeight);
    io.DeltaTime = 1.0f / 60.0f;

    ImFontConfig body;
    body.SizePixels = 20.0f;
    ImFontConfig display;
    display.SizePixels = 40.0f;
    io.Fonts->AddFontDefault();
    io.Fonts->AddFontDefault(&body);
    io.Fonts->AddFontDefault(&display);

    unsigned char* pixels = nullptr;
    int width = 0;
    int height = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
}

struct Vertices {
    int count = 0;
    float minX = 1e9f;
    float maxX = -1e9f;
    float minY = 1e9f;
    float maxY = -1e9f;
    std::vector<ImU32> colors;
};

// Records `commands` as one script frame, replays it inside an ImGui frame and
// returns the vertices it added to the background draw list.
Vertices replay(const std::vector<DrawCommand>& commands)
{
    DrawBuffer& buffer = DrawBuffer::get();
    buffer.beginFrame();
    for (const DrawCommand& command : commands)
        buffer.record(command);
    buffer.endFrame();

    ImGui::NewFrame();
    ImDrawList* list = ImGui::GetBackgroundDrawList();
    int before = list->VtxBuffer.Size;
    buffer.replay();

    Vertices out;
    for (int i = before; i < list->VtxBuffer.Size; ++i) {
        const ImDrawVert& vertex = list->VtxBuffer[i];
        ++out.count;
        out.minX = std::min(out.minX, vertex.pos.x);
        out.maxX = std::max(out.maxX, vertex.pos.x);
        out.minY = std::min(out.minY, vertex.pos.y);
        out.maxY = std::max(out.maxY, vertex.pos.y);
        out.colors.push_back(vertex.col);
    }
    ImGui::EndFrame();
    return out;
}

DrawCommand rect(float x, float y, float w, float h, ImU32 color)
{
    DrawCommand command;
    command.op = DrawOp::ShapeRect;
    command.f0 = x;
    command.f1 = y;
    command.f2 = w;
    command.f3 = h;
    command.c0 = color;
    return command;
}

DrawCommand text(const std::string& value, float x, float width, int align, int font = 0, float size = 0.0f)
{
    DrawCommand command;
    command.op = DrawOp::ShapeText;
    command.text = value;
    command.f0 = x;
    command.f1 = 100.0f;
    command.f2 = width;
    command.f3 = size;
    command.c0 = kRed;
    command.i0 = align;
    command.i1 = font;
    return command;
}

bool allColors(const Vertices& v, ImU32 color)
{
    for (ImU32 c : v.colors)
        if (c != color) return false;
    return !v.colors.empty();
}

// ---------------------------------------------------------------------------
// DrawBuffer shape replay
// ---------------------------------------------------------------------------

void testDisplaySize()
{
    float width = -1.0f;
    float height = -1.0f;
    DrawBuffer::get().displaySize(width, height);
    require(width == 0.0f && height == 0.0f, "display size reads 0x0 before the first replay");

    replay({});
    DrawBuffer::get().displaySize(width, height);
    require(width == kDisplayWidth && height == kDisplayHeight, "display size is the last replayed frame's");
}

void testRect()
{
    Vertices filled = replay({ rect(10, 20, 100, 50, kRed) });
    require(filled.count == 4, "a filled rectangle is one quad");
    require(allColors(filled, kRed), "a filled rectangle keeps the packed colour");
    require(filled.minX == 10 && filled.maxX == 110 && filled.minY == 20 && filled.maxY == 70,
            "a filled rectangle covers x, y, x + w, y + h");

    DrawCommand outline = rect(10, 20, 100, 50, kBlue);
    outline.i0 = 2;
    Vertices stroked = replay({ outline });
    require(stroked.count > 4, "a positive thickness draws an outline, not a fill");

    DrawCommand rounded = rect(10, 20, 100, 50, kRed);
    rounded.f4 = 12.0f;
    require(replay({ rounded }).count > 4, "rounding adds corner vertices");
}

void testGradient()
{
    DrawCommand vertical;
    vertical.op = DrawOp::ShapeGradient;
    vertical.f0 = 0;
    vertical.f1 = 0;
    vertical.f2 = 50;
    vertical.f3 = 50;
    vertical.c0 = kRed;
    vertical.c1 = kBlue;

    Vertices v = replay({ vertical });
    require(v.count == 4, "a gradient is one quad");
    require(v.colors[0] == kRed && v.colors[1] == kRed && v.colors[2] == kBlue && v.colors[3] == kBlue,
            "a vertical gradient runs colorA on top to colorB at the bottom");

    vertical.b0 = true;
    Vertices h = replay({ vertical });
    require(h.colors[0] == kRed && h.colors[1] == kBlue && h.colors[2] == kBlue && h.colors[3] == kRed,
            "a horizontal gradient runs colorA on the left to colorB on the right");
}

void* fakeResolver(const std::string& path)
{
    static int texture = 0;
    return path == "ui/banner.png" ? &texture : nullptr;
}

void testImage()
{
    DrawCommand image;
    image.op = DrawOp::ShapeImage;
    image.text = "ui/banner.png";
    image.f0 = 10;
    image.f1 = 20;
    image.f2 = 200;
    image.f3 = 50;
    image.c0 = 0xFFFFFFFFu;

    require(replay({ image }).count == 0, "an image draws nothing without a resolver");

    DrawBuffer::setImageResolver(&fakeResolver);
    Vertices drawn = replay({ image });
    require(drawn.count == 4, "an image is one textured quad");
    require(drawn.minX == 10 && drawn.maxX == 210 && drawn.minY == 20 && drawn.maxY == 70,
            "an image covers x, y, x + w, y + h");

    image.text = "ui/missing.png";
    require(replay({ image }).count == 0, "an image the resolver cannot load is skipped");
    DrawBuffer::setImageResolver(nullptr);
}

void testLineAndCircle()
{
    DrawCommand line;
    line.op = DrawOp::ShapeLine;
    line.f0 = 0;
    line.f1 = 0;
    line.f2 = 100;
    line.f3 = 0;
    line.c0 = kRed;
    require(replay({ line }).count > 0, "a line with no thickness still draws one pixel wide");

    DrawCommand circle;
    circle.op = DrawOp::ShapeCircle;
    circle.f0 = 200;
    circle.f1 = 200;
    circle.f2 = 30;
    circle.c0 = kBlue;
    Vertices filled = replay({ circle });
    require(filled.count > 8 && filled.minX >= 169.0f && filled.maxX <= 231.0f, "a filled circle stays in its radius");

    circle.f4 = 2.0f;
    require(replay({ circle }).count > 0, "a positive thickness draws a ring");
}

void testText()
{
    Vertices abc = replay({ text("ABC", 50, 0, 0) });
    require(abc.count == 12, "three glyphs are three quads");
    require(allColors(abc, kRed), "text keeps the packed colour");
    require(abc.minX >= 50.0f, "left-aligned text starts at x");

    require(replay({ text("A B", 50, 0, 0) }).count == 8, "a space adds no quad");

    Vertices centred = replay({ text("ABC", 50, 400, 1) });
    float middle = (centred.minX + centred.maxX) / 2;
    require(middle > 245.0f && middle < 255.0f, "centred text sits in the middle of its box");

    Vertices right = replay({ text("ABC", 50, 400, 2) });
    require(right.maxX <= 450.0f && right.maxX > 440.0f, "right-aligned text ends at the box edge");

    std::string longText(80, 'W');
    Vertices clipped = replay({ text(longText, 0, 120, 0) });
    require(clipped.count < 80 * 4, "text wider than its box is cut");
    require(clipped.maxX <= 120.5f, "cut text, ellipsis included, fits its box");

    Vertices accents = replay({ text("\xC3\xA9\xC3\xA9\xC3\xA9\xC3\xA9\xC3\xA9\xC3\xA9\xC3\xA9\xC3\xA9", 0, 30, 0) });
    require(accents.count % 4 == 0 && accents.maxX <= 30.5f, "cutting UTF-8 text never splits a character");

    DrawCommand shadowed = text("ABC", 50, 0, 0);
    shadowed.c1 = kBlue;
    Vertices shadow = replay({ shadowed });
    require(shadow.count == 24, "a shadow colour draws the text twice");
    require(shadow.colors.front() == kBlue && shadow.colors.back() == kRed, "the shadow is drawn under the text");
}

void testFonts()
{
    Vertices small = replay({ text("A", 0, 0, 0, 0) });
    Vertices body = replay({ text("A", 0, 0, 0, 1) });
    Vertices large = replay({ text("A", 0, 0, 0, 2) });
    float smallHeight = small.maxY - small.minY;
    require(body.maxY - body.minY > smallHeight, "font 1 is the body face, bigger than the default");
    require(large.maxY - large.minY > body.maxY - body.minY, "font 2 is the display face, bigger than body");

    Vertices fallback = replay({ text("A", 0, 0, 0, 9) });
    require(fallback.maxY - fallback.minY == smallHeight, "an unknown font index falls back to the default");

    Vertices sized = replay({ text("A", 0, 0, 0, 0, 39.0f) });
    require(sized.maxY - sized.minY > 2.5f * smallHeight, "an explicit size scales the glyphs");
}

void testFocusIsHarmless()
{
    DrawCommand begin;
    begin.op = DrawOp::Begin;
    begin.label = "Prompt";
    DrawCommand focus;
    focus.op = DrawOp::SetKeyboardFocusHere;
    DrawCommand input;
    input.op = DrawOp::InputText;
    input.label = "##field";
    input.text = "abc";
    DrawCommand end;
    end.op = DrawOp::End;
    replay({ begin, focus, input, end });
    require(true, "SetKeyboardFocusHere before a widget replays without asserting");
}

// ---------------------------------------------------------------------------
// PadStateStore
// ---------------------------------------------------------------------------

XINPUT_STATE makeState(WORD buttons, BYTE lt, BYTE rt, SHORT lx, SHORT ly, SHORT rx, SHORT ry)
{
    XINPUT_STATE state = {};
    state.dwPacketNumber = 42;
    state.Gamepad.wButtons = buttons;
    state.Gamepad.bLeftTrigger = lt;
    state.Gamepad.bRightTrigger = rt;
    state.Gamepad.sThumbLX = lx;
    state.Gamepad.sThumbLY = ly;
    state.Gamepad.sThumbRX = rx;
    state.Gamepad.sThumbRY = ry;
    return state;
}

void testPadRoundTrip()
{
    PadStateStore store;
    require(!store.pad(0).connected, "a slot is disconnected until it is observed");
    require(!store.pad(7).connected, "an out-of-range slot reads disconnected");

    XINPUT_STATE state = makeState(XINPUT_GAMEPAD_A | XINPUT_GAMEPAD_RIGHT_SHOULDER | XINPUT_GAMEPAD_Y,
                                   255, 128, -32768, 32767, -1, 12345);
    store.observe(0, ERROR_SUCCESS, &state);
    PadState pad = store.pad(0);
    require(pad.connected, "a successful answer connects the slot");
    require(pad.buttons == (XINPUT_GAMEPAD_A | XINPUT_GAMEPAD_RIGHT_SHOULDER | XINPUT_GAMEPAD_Y),
            "buttons, top bit (Y) included, survive the packing");
    require(pad.leftTrigger == 255 && pad.rightTrigger == 128, "both triggers survive the packing");
    require(pad.leftX == -32768 && pad.leftY == 32767, "the left stick keeps its extremes and sign");
    require(pad.rightX == -1 && pad.rightY == 12345, "the right stick keeps its sign");
    require(state.Gamepad.wButtons != 0, "an uncaptured pad is handed back untouched");

    require(!store.pad(1).connected, "observing slot 0 leaves slot 1 alone");
    store.observe(9, ERROR_SUCCESS, &state);
    require(!store.pad(9).connected, "an out-of-range slot is ignored");
}

void testPadDisconnect()
{
    PadStateStore store;
    XINPUT_STATE state = makeState(XINPUT_GAMEPAD_B, 0, 0, 0, 0, 0, 0);
    store.observe(2, ERROR_SUCCESS, &state);
    require(store.pad(2).connected, "slot 2 connected");

    store.observe(2, ERROR_DEVICE_NOT_CONNECTED, &state);
    require(!store.pad(2).connected, "a failed answer disconnects the slot");

    store.observe(2, ERROR_SUCCESS, &state);
    store.observe(2, ERROR_SUCCESS, nullptr);
    require(!store.pad(2).connected, "a null state disconnects the slot");
}

void testPadCapture()
{
    PadStateStore store;
    require(!store.isCaptured(), "a new store is not captured");
    store.setCaptured(true);
    require(store.isCaptured(), "setCaptured(true) captures");

    XINPUT_STATE state = makeState(XINPUT_GAMEPAD_DPAD_LEFT, 10, 20, 1000, -1000, 2000, -2000);
    store.observe(0, ERROR_SUCCESS, &state);
    require(state.Gamepad.wButtons == 0 && state.Gamepad.bLeftTrigger == 0 && state.Gamepad.sThumbLX == 0
            && state.Gamepad.sThumbRY == 0, "while captured the game gets an idle gamepad");
    require(state.dwPacketNumber == 42, "capturing keeps the packet number");

    PadState pad = store.pad(0);
    require(pad.buttons == XINPUT_GAMEPAD_DPAD_LEFT && pad.leftX == 1000 && pad.rightY == -2000,
            "while captured mods still read the real pad");

    store.setCaptured(false);
    XINPUT_STATE again = makeState(XINPUT_GAMEPAD_START, 0, 0, 0, 0, 0, 0);
    store.observe(0, ERROR_SUCCESS, &again);
    require(again.Gamepad.wButtons == XINPUT_GAMEPAD_START, "releasing the capture gives the game its pad back");
}

} // namespace

int main()
{
    testPadRoundTrip();
    testPadDisconnect();
    testPadCapture();

    setUpImGui();
    testDisplaySize();
    testRect();
    testGradient();
    testImage();
    testLineAndCircle();
    testText();
    testFonts();
    testFocusIsHarmless();
    ImGui::DestroyContext();

    std::cout << "[PASSED] draw primitives and pad store: " << g_checks << " checks" << std::endl;
    return 0;
}
