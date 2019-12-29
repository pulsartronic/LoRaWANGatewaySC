#ifndef __ClientWiFiConfigurable__
#define __ClientWiFiConfigurable__

class ClientWiFiConfigurable : public Configurable {
	public:
	String name = "wifi";
	ClientWiFi* clientWiFi = NULL;

	bool scaning = false;
	unsigned long last = 0;

	bool schedule = false;
	bool credentials = true;
	//String ssid = "";
	//String pass = "";

	~ClientWiFiConfigurable() {
		
	}

	ClientWiFiConfigurable(ClientWiFi* clientWiFi) {
		this->clientWiFi = clientWiFi;
	}

	void loop() {
		if (this->schedule) {
			this->schedule = false;

			this->clientWiFi->applyDHCP();

			wifi_set_phy_mode((phy_mode_t) this->clientWiFi->mode);
			WiFi.hostname(this->clientWiFi->hostname);

			if (credentials) {
				WiFi.disconnect();
				delay(100);
				WiFi.begin(this->clientWiFi->ssid.c_str(), this->clientWiFi->pass.c_str());
			}
		}
	}

	String getName() {
		return this->name;
	}

	String hrdwr() {
		String hrdwr = "{}";
		return hrdwr;
	}

	String ping() {
		dhcp_status dhcpstatus = wifi_station_dhcpc_status();
		int dhcp = DHCP_STOPPED == dhcpstatus ? 0 : 1;
		int phymode = wifi_get_phy_mode();

		String wifi = "{";
		wifi += "\"status\":" + String(WiFi.status());
		wifi += ",\"ssid\":\"" + this->clientWiFi->ssid + "\"";
		wifi += ",\"bssid\":\"" + WiFi.BSSIDstr() + "\"";
		wifi += ",\"mac\":\"" + WiFi.macAddress() + "\"";
		wifi += ",\"ip\":\"" + WiFi.localIP().toString() + "\"";
		wifi += ",\"sm\":\"" + WiFi.subnetMask().toString() + "\"";
		wifi += ",\"gw\":\"" + WiFi.gatewayIP().toString() + "\"";
		wifi += ",\"dns1\":\"" + WiFi.dnsIP(0).toString() + "\"";
		wifi += ",\"dns2\":\"" + WiFi.dnsIP(1).toString() + "\"";
		wifi += ",\"hn\":\"" + WiFi.hostname() + "\"";
		wifi += ",\"mode\":" + String(phymode);
		wifi += ",\"ss\":\"" + String(WiFi.RSSI()) + "\"";
		wifi += ",\"dhcp\":" + String(dhcp);
		wifi += "}";
		return wifi;
	}

	String command(JsonObject command) {
		String name = command["n"];
		JsonObject params = command["p"];
		String response = "{\"error\":2}";
		if (String(F("save")).equals(name)) {
			response = this->save(params);
		} else if (String(F("scan")).equals(name)) {
			response = this->scan(params);
		} else if (String(F("network")).equals(name)) {
			response = this->network(params);
		}
		return response;
	}

	String save(JsonObject params) {
		this->clientWiFi->fromJSON(params);
		this->clientWiFi->saveFile();
		this->schedule = true;
		bool hasSSID = params.containsKey("s");
		bool hasPASS = params.containsKey("p");
		// Serial.println("hasSSID: " + String(hasSSID) + " hasPASS: " + String(hasPASS));
		this->credentials = hasSSID && hasPASS;
		if (this->credentials) {
			this->clientWiFi->lphase = sysclock->mstime();
			this->clientWiFi->Log("{\"n\":\"log\",\"p\":\"WiFi saved, connecting to " + this->clientWiFi->ssid + "\"}");
		}
		String response = this->ping();
		return response;
	}

	String scan(JsonObject params) {
		WiFi.scanDelete();
		int networksFound = WiFi.scanNetworks();
		String response = "{\"value\":" + String(networksFound) + "}";
		return response;
	}

	String network(JsonObject params) {
		String ii = params["i"];
		int index = ii.toInt();

		String network = "{\"ssid\":\"" + WiFi.SSID(index) + "\"";
		network += ",\"chn\":" + String(WiFi.channel(index));
		network += ",\"rssi\":" + String(WiFi.RSSI(index));
		network += ",\"opn\":" + String(WiFi.encryptionType(index) == ENC_TYPE_NONE);
		network += "}";

		return network;
	}
};

#endif
