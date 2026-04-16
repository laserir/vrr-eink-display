#include "portal.h"
#include "config.h"
#include "runtime_config.h"
#include <WiFiManager.h>

// File-scope WiFiManager so it survives portalRun() and can continue
// serving the web portal on the STA IP after WiFi connects.
static WiFiManager wm;
static WiFiManagerParameter* pCity = nullptr;
static WiFiManagerParameter* pName = nullptr;
static WiFiManagerParameter* pTitle = nullptr;
static WiFiManagerParameter* pRefresh = nullptr;
static bool s_webPortalActive = false;

static void persistParams() {
    strncpy(g_stop_city, pCity->getValue(), sizeof(g_stop_city) - 1);
    g_stop_city[sizeof(g_stop_city) - 1] = '\0';
    strncpy(g_stop_name, pName->getValue(), sizeof(g_stop_name) - 1);
    g_stop_name[sizeof(g_stop_name) - 1] = '\0';
    strncpy(g_device_title, pTitle->getValue(), sizeof(g_device_title) - 1);
    g_device_title[sizeof(g_device_title) - 1] = '\0';
    int r = atoi(wm.server->arg("refresh").c_str());
    if (r <= 0) r = DEFAULT_REFRESH_MIN;
    g_refresh_min = refreshMinSanitize(r);
    runtimeConfigSave();
    Serial.println("Portal: runtime config saved");
}

static void setupOnce() {
    if (pCity) return;

    pCity = new WiFiManagerParameter("city", "Stop city",
        g_stop_city, sizeof(g_stop_city) - 1);
    pName = new WiFiManagerParameter("name", "Stop name",
        g_stop_name, sizeof(g_stop_name) - 1);
    pTitle = new WiFiManagerParameter("title", "Device title",
        g_device_title, sizeof(g_device_title) - 1);

    // Build <select> dropdown with current value pre-selected
    static char refreshHtml[512];
    const int opts[] = {1, 3, 5, 10, 30, 60};
    int pos = snprintf(refreshHtml, sizeof(refreshHtml),
        "<br/><label for='refresh'>Refresh interval (minutes)</label>"
        "<select id='refresh' name='refresh'>");
    for (int i = 0; i < 6; i++) {
        pos += snprintf(refreshHtml + pos, sizeof(refreshHtml) - pos,
            "<option value='%d'%s>%d</option>",
            opts[i], (opts[i] == g_refresh_min) ? " selected" : "", opts[i]);
    }
    snprintf(refreshHtml + pos, sizeof(refreshHtml) - pos, "</select>");
    pRefresh = new WiFiManagerParameter(refreshHtml);

    wm.addParameter(pCity);
    wm.addParameter(pName);
    wm.addParameter(pTitle);
    wm.addParameter(pRefresh);

    wm.setConfigPortalTimeout(PORTAL_TIMEOUT_SEC);
    std::vector<const char*> menu = {"wifi", "param", "sep", "restart", "exit"};
    wm.setMenu(menu);
    wm.setSaveConfigCallback(persistParams);
    wm.setSaveParamsCallback(persistParams);
}

bool portalRun(bool forceConfig) {
    setupOnce();
    if (forceConfig) {
        return wm.startConfigPortal(PORTAL_AP_NAME);
    }
    return wm.autoConnect(PORTAL_AP_NAME);
}

void portalStartWebPortal() {
    setupOnce();
    wm.startWebPortal();
    s_webPortalActive = true;
    Serial.printf("Web portal on http://%s/\n",
        WiFi.localIP().toString().c_str());
}

void portalProcess() {
    if (s_webPortalActive) wm.process();
}
