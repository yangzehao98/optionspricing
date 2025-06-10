/*
StopWatch.hpp

Lightweight Utility for Measuring Elapsed Execution Time

Overview:
---------
This header defines a simple stopwatch utility for timing code execution using
the C++11 `<chrono>` library. It is useful for profiling Monte Carlo simulations,
numerical solvers, or other performance-critical sections.


Core Features:
--------------
- `StartStopWatch()`: Begins timing from the current system clock.
- `StopStopWatch()`: Stops timing and accumulates the elapsed duration.
- `Reset()`: Resets the stopwatch to zero.
- `GetTime()`: Returns the total elapsed time (in seconds) as a `double`.

Design Notes:
-------------
- Uses `std::chrono::system_clock` for portable high-resolution timing.
- Ensures copy-construction and assignment are disabled (singleton-like usage).
- Internally accumulates time over multiple start/stop intervals.

Usage:
------
```cpp
StopWatch sw;
sw.StartStopWatch();
// ... code to be timed ...
sw.StopStopWatch();
std::cout << "Elapsed time: " << sw.GetTime() << " seconds\n";

*/

#ifndef StopWatch_HPP
#define StopWatch_HPP

#include <iostream>
#include <chrono>


class StopWatch
{
private:
    StopWatch(const StopWatch&) = delete;
    StopWatch& operator=(const StopWatch&) = delete;
    std::chrono::time_point<std::chrono::system_clock> startTime;
	std::chrono::time_point<std::chrono::system_clock> endTime;
    double elapsed;
    bool running;
public:
    StopWatch() : elapsed(0), running(false) {}

    void StartStopWatch() {
		std::cout << "StopWatch is starting." << std::endl;
        startTime = std::chrono::system_clock::now();
        running = true;
    }

    void StopStopWatch() {
        if (running) {
            endTime = std::chrono::system_clock::now();
            elapsed += std::chrono::duration<double>(endTime - startTime).count();
            running = false;
        }
    }

    void Reset() {
        elapsed = 0;
        running = false;
    }

    double GetTime() const {
        return elapsed;
    }
};

#endif