/*
** CrabeLoader
** File description:
** menu -- the mod menu's shared state, between the render and game threads
*/

#ifndef MENU_HPP_
#define MENU_HPP_

#include <mutex>
#include <string>
#include <vector>

// The menu is declared and navigated in Lua (src/api/15_menu.lua); this only
// carries it across a thread boundary. The overlay runs on the render thread
// and must never touch a lua_State, so it reads the cached view here and
// queues what the player clicked. The game thread drains that queue, calls
// Lua, and stores the view that comes back -- the same pattern the console
// input already uses.
class Menu {
    public:
        static Menu& get();

        // What the overlay draws. A snapshot, so it can be read without
        // holding the lock while ImGui works through it.
        struct View {
            std::string title;
            std::vector<std::string> items;
            std::string status;
        };

        View view() const;

        // Called from the render thread. The action is applied on the game
        // thread, at the next drain.
        void requestActivate(int index);
        void requestBack();
        void requestRefresh();

        // Called from the game thread with a live Lua state: applies whatever
        // the player queued and refreshes the cached view. Cheap and does
        // nothing when nothing was queued.
        void drain(void* L);

    private:
        Menu() = default;
        ~Menu() = default;
        Menu(const Menu&) = delete;
        Menu& operator=(const Menu&) = delete;

        // "T=<title>", "L=<label>" per item, "S=<status>" -- what
        // Crabe.Menu._describe() returns.
        void storeView(const std::string& described);

        enum class Action { Refresh, Activate, Back };

        struct Request {
            Action action = Action::Refresh;
            int index = 0;
        };

        mutable std::mutex _mutex;
        View _view;
        std::vector<Request> _pending;
};

#endif /* !MENU_HPP_ */
