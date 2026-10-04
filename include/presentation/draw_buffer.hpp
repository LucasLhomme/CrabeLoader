/*
** CrabeLoader
** File description:
** Declares the queue the script thread records ImGui calls into for the render thread to replay.
** This is the only legal path from Lua to Direct3D: recording a call draws nothing.
** Widget results lag one frame, because a measurement only exists once the replay has run.
**
** Authors: @LucasLhomme
*/

#ifndef DRAW_BUFFER_HPP_
#define DRAW_BUFFER_HPP_

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace crabe::presentation {

    enum class DrawOp : std::uint8_t {
        Begin,
        End,
        Text,
        TextColored,
        TextDisabled,
        Button,
        Selectable,
        SetScrollHereY,
        Checkbox,
        SliderFloat,
        SliderInt,
        InputText,
        SameLine,
        Separator,
        Spacing,
        BeginChild,
        EndChild,
        BeginTabBar,
        EndTabBar,
        BeginTabItem,
        EndTabItem,
        SetNextWindowPos,
        SetNextWindowSize,
        IsItemClicked,
        SetKeyboardFocusHere,
        ShapeRect,
        ShapeGradient,
        ShapeLine,
        ShapeCircle,
        ShapeText,
        ShapeImage,
    };

    // Fonts a draw command can ask for: the atlas default plus the two UI faces
    // the overlay loads at startup. A missing face falls back to Default.
    enum class DrawFont : std::uint8_t {
        Default = 0,
        Body = 1,
        Display = 2,
    };

    // One recorded ImGui call. The numeric slots and `b0` are op-specific, but
    // `b1` always means "the script recorded a matching close for this scope".
    struct DrawCommand {
        DrawOp op = DrawOp::Separator;
        std::uint32_t id = 0;
        std::string label;
        std::string text;
        float f0 = 0.0f;
        float f1 = 0.0f;
        float f2 = 0.0f;
        float f3 = 0.0f;
        float f4 = 0.0f;
        std::uint32_t c0 = 0;
        std::uint32_t c1 = 0;
        int i0 = 0;
        int i1 = 0;
        bool b0 = false;
        bool b1 = false;
    };

    // What the render thread measured for one widget, read back by the script
    // thread on the following frame.
    struct WidgetResult {
        bool flag = false;
        bool open = true;
        double number = 0.0;
        std::string text;
    };

    // Turns an image path into an ImGui texture id, or nullptr when it cannot.
    // Called on the render thread only; the renderer installs it once at startup.
    using ImageResolver = void* (*)(const std::string& path);

    // Carries one frame of Lua-issued ImGui calls from the script thread to the
    // render thread, and the resulting widget states back, so that the Lua VM is
    // never touched outside Loader::runTicks (Architecture Blueprint, Rule 3).
    class DrawBuffer final {
    public:
        static DrawBuffer& get();

        // Script thread: opens a new recording, dropping any unpublished one.
        void beginFrame();

        // Script thread: appends a command and returns its generated widget id.
        std::uint32_t record(DrawCommand command);

        // Script thread: appends a scope-opening command, returns last frame's
        // state for it, and stores that state in the command so the replay knows
        // whether a matching close follows in the stream.
        bool recordScope(DrawCommand command, bool fallback);

        // Script thread: publishes the recording for the render thread to replay.
        void endFrame();

        // Script thread: reads back what the render thread measured for `id`.
        // False when that widget was never drawn, leaving `out` untouched.
        bool tryResult(std::uint32_t id, WidgetResult& out) const;

        // Render thread: replays the published frame through native ImGui and
        // records every widget state for the script thread to read next frame.
        // Publishing an empty frame is what resets stale widget state.
        void replay();

        // Render thread: installs the function ShapeImage uses to find a texture.
        // Without one, an image command draws nothing.
        static void setImageResolver(ImageResolver resolver);

        // Script thread: the display size the render thread saw on its last
        // frame, or 0x0 before the first one.
        void displaySize(float& width, float& height) const;

    private:
        DrawBuffer() = default;
        ~DrawBuffer() = default;
        DrawBuffer(const DrawBuffer&) = delete;
        DrawBuffer& operator=(const DrawBuffer&) = delete;

        std::vector<DrawCommand> _recording;
        std::uint32_t _sequence = 0;

        std::vector<DrawCommand> _published;
        bool _hasPublished = false;
        mutable std::mutex _publishMutex;

        std::vector<DrawCommand> _playback;
        std::unordered_map<std::uint32_t, WidgetResult> _accumulated;

        std::unordered_map<std::uint32_t, WidgetResult> _results;
        mutable std::mutex _resultsMutex;

        std::atomic<float> _displayWidth{ 0.0f };
        std::atomic<float> _displayHeight{ 0.0f };
    };

} // namespace crabe::presentation

#endif /* !DRAW_BUFFER_HPP_ */
