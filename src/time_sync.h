#pragma once
#include <Arduino.h>
#include <time.h>

bool syncTime(const char* tzInfo, const char* ntpServer);
time_t parseISO8601(const char* str);
