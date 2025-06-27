#pragma once

// System
#include <chrono>
#include <thread>

// Internal
#include "bs.h"

inline bsUs_t
bsGetClockUs()
{
    return (bsUs_t)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

inline void
bsSleep(bsUs_t durationUs)
{
    asserted(durationUs >= 0);
    std::this_thread::sleep_for(std::chrono::microseconds(durationUs));
}
