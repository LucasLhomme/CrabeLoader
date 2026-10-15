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

    enum class DrawFont : std::uint8_t {
        Default = 0,
        Body = 1,
        Display = 2,
    };

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

    struct WidgetResult {
        bool flag = false;
        bool open = true;
        double number = 0.0;
        std::string text;
    };

    using ImageResolver = void* (*)(const std::string& path);

    class DrawBuffer final {
    public:
        // Returns the singleton instance of DrawBuffer.
        static DrawBuffer& get();

        // Opens a new recording on the script thread, dropping any unpublished frame.
        void beginFrame();

        // Appends a command on the script thread and returns its generated widget id.
        std::uint32_t record(DrawCommand command);

        // Appends a scope-opening command and returns previous frame state for it.
        bool recordScope(DrawCommand command, bool fallback);

        // Publishes the recorded frame for the render thread to replay.
        void endFrame();

        // Reads back measured widget state for the given widget id.
        bool tryResult(std::uint32_t id, WidgetResult& out) const;

        // Replays the published frame through native ImGui on the render thread.
        void replay();

        // Configures the texture resolver callback for image drawing operations.
        static void setImageResolver(ImageResolver resolver);

        // Retrieves the last recorded display size from the render thread.
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
