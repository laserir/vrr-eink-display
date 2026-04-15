#pragma once
#include "departure.h"

// Fetches the next upcoming departures for the configured stop, sorted by
// time. Returns true on success. `count` is set to the number of filled rows.
bool fetchDepartures(Departure* deps, int& count, int maxDepartures);
