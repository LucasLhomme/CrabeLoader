/*
** CrabeLoader
** File description:
** Declares the mouse wheel counter: accumulates mouse wheel movements into discrete notches.
** Lock-free: the input hook accumulates on polling threads, Lua consumes at tick time.
** Header-only and free of Win32 and Lua, allowing isolated offline testing.
**
** Authors: @LucasLhomme
*/

#ifndef WHEEL_COUNTER_HPP_
#define WHEEL_COUNTER_HPP_

#include <atomic>
#include <cstdint>

namespace crabe::presentation {

class WheelCounter final {
    public:
        static constexpr int32_t kUnitsPerNotch = 120;

        // Adds one reading of the wheel axis: positive is up (away from the player), negative is down.
        void add(int32_t units)
        {
            if (units == 0) return;
            _units.fetch_add(units, std::memory_order_relaxed);
            _readings.fetch_add(1, std::memory_order_relaxed);
        }

        // Removes and returns the whole notches counted so far, signed like add(); a fraction stays.
        int32_t takeNotches()
        {
            int32_t units = _units.load(std::memory_order_relaxed);
            for (;;) {
                int32_t notches = units / kUnitsPerNotch;
                if (notches == 0) return 0;
                if (_units.compare_exchange_weak(units, units - notches * kUnitsPerNotch, std::memory_order_relaxed))
                    return notches;
            }
        }

        // How many times the wheel moved since the start, for telling a dead wheel from a dead menu.
        uint32_t readings() const { return _readings.load(std::memory_order_relaxed); }

    private:
        std::atomic<int32_t> _units{ 0 };
        std::atomic<uint32_t> _readings{ 0 };
};

} // namespace crabe::presentation

#endif /* !WHEEL_COUNTER_HPP_ */
