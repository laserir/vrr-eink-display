#include "time_sync.h"

void syncTime(const char* tzInfo, const char* ntpServer) {
    Serial.print("Syncing NTP...");
    configTime(tzInfo, ntpServer);

    time_t now = 0;
    int attempts = 0;
    while (now < 100000 && attempts < 20) {
        delay(500);
        time(&now);
        attempts++;
    }
    if (now < 100000) {
        Serial.println(" FAILED");
        return;
    }
    struct tm ti;
    localtime_r(&now, &ti);
    char buf[20];
    strftime(buf, sizeof(buf), "%H:%M:%S", &ti);
    Serial.printf(" OK (%s)\n", buf);
}

time_t parseISO8601(const char* str) {
    if (!str || strlen(str) < 19) return 0;
    // Format: "2026-04-01T08:53:00Z" — timestamps are always UTC.
    struct tm t = {};
    t.tm_year = atoi(str) - 1900;
    t.tm_mon  = atoi(str + 5) - 1;
    t.tm_mday = atoi(str + 8);
    t.tm_hour = atoi(str + 11);
    t.tm_min  = atoi(str + 14);
    t.tm_sec  = atoi(str + 17);

    // mktime() interprets `t` as *local* time — but the fields are UTC.
    // Compute the current TZ offset and compensate so the returned
    // time_t is the correct UTC epoch.
    //
    //   asLocal          = mktime(utc_fields as local)          [wrong by -offset]
    //   gmtime(asLocal)  = utc_fields - offset (as UTC)
    //   mktime(gmtime(asLocal) as local) = asLocal - offset
    //   tzOffset         = asLocal - doubleConverted = offset
    //   return           = asLocal + offset                     [correct]
    time_t asLocal = mktime(&t);
    struct tm utcOfLocal;
    gmtime_r(&asLocal, &utcOfLocal);
    time_t doubleConverted = mktime(&utcOfLocal);
    time_t tzOffset = asLocal - doubleConverted;
    return asLocal + tzOffset;
}
