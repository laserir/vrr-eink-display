#pragma once

// Runtime-editable config, persisted to LittleFS as JSON and set via the
// captive portal. Values are copied into the globals below at boot.

extern char g_stop_city[32];
extern char g_stop_name[32];
extern char g_device_title[64];
extern int  g_refresh_min;

// Clamp an arbitrary int to the nearest allowed refresh value.
int refreshMinSanitize(int v);

void runtimeConfigInitDefaults();
bool runtimeConfigLoad();
bool runtimeConfigSave();
