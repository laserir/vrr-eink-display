#include "api_client.h"
#include "config.h"
#include "runtime_config.h"
#include "time_sync.h"
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <math.h>

static time_t depTime(const Departure& d) {
    return d.estimatedTime ? d.estimatedTime : d.plannedTime;
}

// Insert-sort `d` into `arr` keeping the array sorted by departure time,
// bounded to `max` entries. Drops the latest entry if the array is full.
static void insertSorted(Departure* arr, int& count, int max, const Departure& d) {
    time_t t = depTime(d);
    int pos = count;
    while (pos > 0 && depTime(arr[pos - 1]) > t) pos--;
    if (pos >= max) return; // later than anything we'd keep
    int end = count < max ? count : max - 1;
    for (int i = end; i > pos; i--) arr[i] = arr[i - 1];
    arr[pos] = d;
    if (count < max) count++;
}

static String urlEncode(const char* src) {
    String out;
    out.reserve(strlen(src) * 3);
    for (const char* p = src; *p; p++) {
        char c = *p;
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
            out += c;
        } else {
            char buf[4];
            snprintf(buf, sizeof(buf), "%%%02X", (unsigned char)c);
            out += buf;
        }
    }
    return out;
}

bool fetchDepartures(Departure* deps, int& count, int maxDepartures) {
    count = 0;

    String url = String("https://") + API_HOST + API_BASE
        + "&place_dm=" + urlEncode(g_stop_city)
        + "&name_dm=" + urlEncode(g_stop_name);
    Serial.printf("Fetching %s\n", url.c_str());

    BearSSL::WiFiClientSecure client;
    client.setInsecure();
    // Shrink BearSSL buffers — default is 16 KB input which OOMs after
    // WiFiManager's web portal is running. 4 KB input is the smallest value
    // that reliably handles VRR's TLS cert chain without hitting NoMemory.
    client.setBufferSizes(4096, 512);

    HTTPClient http;
    if (!http.begin(client, url)) {
        Serial.println("HTTP begin failed");
        return false;
    }

    int httpCode = http.GET();
    if (httpCode != 200) {
        Serial.printf("HTTP error: %d\n", httpCode);
        http.end();
        return false;
    }

    JsonDocument filter;
    filter["stopEvents"][0]["departureTimePlanned"] = true;
    filter["stopEvents"][0]["departureTimeEstimated"] = true;
    filter["stopEvents"][0]["transportation"]["number"] = true;
    filter["stopEvents"][0]["transportation"]["destination"]["name"] = true;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, http.getStream(),
        DeserializationOption::Filter(filter));
    http.end();

    if (err) {
        Serial.printf("JSON parse error: %s\n", err.c_str());
        return false;
    }

    JsonArray events = doc["stopEvents"].as<JsonArray>();
    if (events.isNull()) {
        Serial.println("No stopEvents in response");
        return false;
    }

    time_t now;
    time(&now);
    String cityPrefix = String(g_stop_city) + " ";
    String commaPrefix = String(g_stop_city) + ", ";

    for (JsonObject ev : events) {
        const char* plannedStr = ev["departureTimePlanned"];
        const char* estimatedStr = ev["departureTimeEstimated"];
        JsonObject transport = ev["transportation"];
        if (!transport) continue;
        const char* lineNum = transport["number"];
        const char* dest = transport["destination"]["name"];

        if (!plannedStr || !lineNum || !dest) continue;

        time_t planned = parseISO8601(plannedStr);
        time_t estimated = estimatedStr ? parseISO8601(estimatedStr) : 0;
        time_t t = estimated ? estimated : planned;

        if (difftime(t, now) < -60) continue; // skip past

        Departure d;
        d.lineNumber = lineNum;
        String destStr = dest;
        if (destStr.startsWith(cityPrefix)) {
            destStr = destStr.substring(cityPrefix.length());
        } else {
            if (destStr.startsWith(commaPrefix)) {
                destStr = destStr.substring(commaPrefix.length());
            }
        }
        d.destination = destStr;
        d.plannedTime = planned;
        d.estimatedTime = estimated;
        d.delayMinutes = estimated ? (int)floor(difftime(estimated, planned) / 60.0) : 0;

        insertSorted(deps, count, maxDepartures, d);
    }

    Serial.printf("Departures: %d\n", count);
    return true;
}
