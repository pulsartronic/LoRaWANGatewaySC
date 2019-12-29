#ifndef __APConfigurable__
#define __APConfigurable__

class AccessPointConfigurable : public Configurable {
	public:
	String name = "ap";
	AccessPoint* accessPoint = NULL;

	virtual ~AccessPointConfigurable() {
		
	}

	AccessPointConfigurable(AccessPoint* accessPoint) {
		this->accessPoint = accessPoint;
	}

	void loop() {
		
	}

	String getName() {
		return this->name;
	}

	String ping() {
		//struct softap_config {
		//	  uint8 ssid[32];
		//	  uint8 password[64];
		//	  uint8 ssid_len;
		//	  uint8 channel;
		//	  uint8 authmode;
		//	  uint8 ssid_hidden;
		//	  uint8 max_connection;
		//	  uint16 beacon_interval;
		//};
		struct softap_config conf;
		wifi_softap_get_config(&conf);

		struct ip_info info;
		wifi_get_ip_info(SOFTAP_IF, &info);

		//wifi_country_t country;
		//wifi_get_country(&country);

		String hotspot = "{\"ip\":\"" + WiFi.softAPIP().toString() + "\"";
		hotspot += ",\"mac\":\"" + WiFi.softAPmacAddress() + "\"";
		hotspot += ",\"ssid\":\"" + String((char*)conf.ssid) + "\"";
		hotspot += ",\"stations\":\"" + String(WiFi.softAPgetStationNum()) + "\"";
		hotspot += ",\"channel\":\"" + String(conf.channel) + "\"";
		hotspot += ",\"hidden\":" + String(conf.ssid_hidden);
		hotspot += ",\"gw\":\"" + IPAddress(info.gw.addr).toString() + "\"";
		hotspot += ",\"netmask\":\"" + IPAddress(info.netmask.addr).toString() + "\"}";
		// hotspot += ",\"country\":\"" + String((int)country) + "\"}";

		return hotspot;
	}

	String hrdwr() {
		String hrdwr = "{";
		hrdwr += "}";
		return hrdwr;
	}

	String command(JsonObject command) {
		String name = command["n"];
		JsonObject params = command["p"];
		String response = "{\"error\":2}";
		if (String(F("save")).equals(name)) {
			response = this->save(params);
		}
		return response;
	}

	String save(JsonObject params) {
		this->accessPoint->fromJSON(params);
		this->accessPoint->saveFile();
		//struct softap_config conf;
		//wifi_softap_get_config(&conf);
		//struct ip_info info;
		//wifi_get_ip_info(SOFTAP_IF, &info);
		WiFi.softAP(this->accessPoint->ssid, this->accessPoint->pass);
		String response = this->ping();
		return response;
	}
};

#endif

