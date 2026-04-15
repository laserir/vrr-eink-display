#pragma once
#include <Arduino.h>
#include <time.h>

struct Departure {
    String lineNumber;
    String destination;
    time_t plannedTime;
    time_t estimatedTime;
    int delayMinutes;

    int minutesUntil() const {
        time_t now;
        time(&now);
        time_t dep = estimatedTime ? estimatedTime : plannedTime;
        int diff = (int)difftime(dep, now) / 60;
        return diff < 0 ? 0 : diff;
    }
};
