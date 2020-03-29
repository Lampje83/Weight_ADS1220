#include "wifiSys.h"

wifi_prov_mgr_config_t config = { 
	.scheme = wifi_prov_scheme_ble,
	.scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM
};

wifiSysClass wifiSys;

void wifiSysClass::begin () {
	bool	isProvisioned;
	const char	*serviceName = "MiniScale";
	const char	*pop = "abcd123";
	
	//esp_netif_create_default_wifi_sta ();
	WiFi.begin ();
	
	wifi_prov_mgr_init (config);
	wifi_prov_mgr_is_provisioned (&isProvisioned);
	
	if (!isProvisioned) {
		Serial.println ("Starting provisioning");

		wifi_prov_mgr_start_provisioning (WIFI_PROV_SECURITY_1,
			pop, serviceName, NULL);		
	}
}
	
void wifiSysClass::loop () {	}