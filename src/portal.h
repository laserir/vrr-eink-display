#pragma once

// Runs WiFiManager. If stored WiFi connects within timeout, returns true
// immediately. Otherwise opens a captive portal at PORTAL_AP_NAME where the
// user can enter WiFi + stop_city / stop_name / device_title. On save, the
// custom fields are copied into runtime_config globals and persisted.
bool portalRun(bool forceConfig);

// Start the WiFiManager web portal on the STA IP so users can reach the same
// config UI at http://<device-ip>/. Must be pumped from loop() via
// portalProcess().
void portalStartWebPortal();
void portalProcess();
