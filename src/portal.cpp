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
static char s_refreshBuf[4];
static bool s_webPortalActive = false;

static void persistParams() {
    strncpy(g_stop_city, pCity->getValue(), sizeof(g_stop_city) - 1);
    g_stop_city[sizeof(g_stop_city) - 1] = '\0';
    strncpy(g_stop_name, pName->getValue(), sizeof(g_stop_name) - 1);
    g_stop_name[sizeof(g_stop_name) - 1] = '\0';
    strncpy(g_device_title, pTitle->getValue(), sizeof(g_device_title) - 1);
    g_device_title[sizeof(g_device_title) - 1] = '\0';
    int r = atoi(pRefresh->getValue());
    if (r <= 0) r = DEFAULT_REFRESH_MIN;
    g_refresh_min = refreshMinSanitize(r);
    runtimeConfigSave();
    Serial.println("Portal: runtime config saved");
}

static void setupOnce() {
    if (pCity) return;

    pCity = new WiFiManagerParameter("city", "Stop city (exact spelling as on vrr.de)",
        g_stop_city, sizeof(g_stop_city) - 1);
    pName = new WiFiManagerParameter("name", "Stop name (exact spelling as on vrr.de)",
        g_stop_name, sizeof(g_stop_name) - 1);
    pTitle = new WiFiManagerParameter("title", "Device title",
        g_device_title, sizeof(g_device_title) - 1);

    // Dropdown for refresh interval, following WiFiManager custom HTML pattern:
    // 1) Hidden standard param stores the value WiFiManager reads back
    // 2) Custom HTML param renders the visible <select> and syncs via JS
    snprintf(s_refreshBuf, sizeof(s_refreshBuf), "%d", g_refresh_min);
    pRefresh = new WiFiManagerParameter("refresh", "refresh", s_refreshBuf, 3);

    static char selectHtml[640];
    const int opts[] = {1, 3, 5, 10, 30, 60};
    int pos = snprintf(selectHtml, sizeof(selectHtml),
        "<br/><label for='s'>Refresh (min)</label>"
        "<select id='s' onchange=\"document.getElementById('refresh').value=this.value\">");
    for (int i = 0; i < 6; i++) {
        pos += snprintf(selectHtml + pos, sizeof(selectHtml) - pos,
            "<option value='%d'>%d</option>", opts[i], opts[i]);
    }
    snprintf(selectHtml + pos, sizeof(selectHtml) - pos,
        "</select>"
        "<script>"
        "document.getElementById('s').value=document.getElementById('refresh').value;"
        "document.querySelector(\"[for='refresh']\").hidden=true;"
        "document.getElementById('refresh').hidden=true;"
        "</script>");
    static WiFiManagerParameter selectParam(selectHtml);

    static WiFiManagerParameter info(
        "<br/><hr/><p style='font-size:small'>"
        "Find stop names, docs &amp; license info at<br/>"
        "<a href='https://github.com/laserir/vrr-eink-display' target='_blank'>"
        "github.com/laserir/vrr-eink-display</a></p>");

    wm.addParameter(pCity);
    wm.addParameter(pName);
    wm.addParameter(pTitle);
    wm.addParameter(pRefresh);
    wm.addParameter(&selectParam);
    wm.addParameter(&info);

    wm.setConfigPortalTimeout(PORTAL_TIMEOUT_SEC);
    std::vector<const char*> menu = {"param", "wifi", "sep", "restart", "exit"};
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
