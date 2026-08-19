/*
** CrabeLoader
** File description:
** menu -- the mod menu's shared state, between the render and game threads
*/

#include "loader/menu.hpp"

#include <format>
#include <string_view>

#include "loader/luacall.hpp"
#include "logger/logger.hpp"

Menu& Menu::get()
{
    static Menu instance;
    return instance;
}

Menu::View Menu::view() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _view;
}

void Menu::requestActivate(int index)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _pending.push_back({Action::Activate, index});
}

void Menu::requestBack()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _pending.push_back({Action::Back, 0});
}

void Menu::requestRefresh()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _pending.push_back({Action::Refresh, 0});
}

void Menu::storeView(const std::string& described)
{
    View parsed;
    std::string_view rest(described);

    while (!rest.empty()) {
        size_t breakAt = rest.find('\n');
        std::string_view line = rest.substr(0, breakAt);
        rest = breakAt == std::string_view::npos ? std::string_view() : rest.substr(breakAt + 1);

        if (line.size() < 2 || line[1] != '=')
            continue;

        std::string value(line.substr(2));
        switch (line[0]) {
            case 'T': parsed.title = std::move(value); break;
            case 'L': parsed.items.push_back(std::move(value)); break;
            case 'S': parsed.status = std::move(value); break;
            default: break;
        }
    }

    std::lock_guard<std::mutex> lock(_mutex);
    _view = std::move(parsed);
}

void Menu::drain(void* L)
{
    std::vector<Request> pending;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_pending.empty())
            return;
        pending.swap(_pending);
    }

    // Only the last view matters, but every request still has to run: each one
    // may flip a toggle or fire an action.
    std::string described;
    for (const Request& request : pending) {
        std::string call;
        switch (request.action) {
            case Action::Activate: call = std::format("return Crabe.Menu._activate({})", request.index); break;
            case Action::Back:     call = "return Crabe.Menu._back()"; break;
            case Action::Refresh:  call = "return Crabe.Menu._describe()"; break;
        }

        std::string result;
        if (!LuaCall::get().runSnippet(L, call, result)) {
            Logger::getInstance().warning("Menu: {} failed: {}", call, result);
            continue;
        }
        described = std::move(result);
    }

    if (!described.empty())
        storeView(described);
}
