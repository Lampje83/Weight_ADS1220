#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_ble.h"
#include <esp_gap_ble_api.h>
//#include "esp_wifi.h"
class wifiSysClass {
public:
	void	begin ();
	void	loop ();
};

extern wifiSysClass wifiSys;
