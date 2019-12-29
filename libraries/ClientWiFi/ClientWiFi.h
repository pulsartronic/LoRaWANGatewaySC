#ifndef __ClientWiFi__
#define __ClientWiFi__

// https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

class ClientWiFi {
	public:
	Logger* logger;

	uint32_t iphase = 2ul * 60ul * 1000ul; // 2 minutes
	uint64_t lphase = 0ull;
	bool tryConnection = false;

	String filename = "wifi.json";
	String ssid = "";
	String pass = "";
	int mode = PHY_MODE_11B;
	String hostname = "";
	int dhcp = 1;
	String ip = "";
	String gateway = "";
	String netmask = "";
	String dns1 = "";
	String dns2 = "";

	virtual ~ClientWiFi() {
		
	}

	ClientWiFi(Logger* logger) {
		this->logger = logger;
	}

	void setup() {
		Serial.println(F("Starting WiFi Client ..."));
		
		this->hostname = WiFi.hostname();
		this->readFile();

		wifi_set_phy_mode((phy_mode_t) this->mode);
		WiFi.hostname(this->hostname);

		WiFi.setAutoConnect(true);
		WiFi.setAutoReconnect(true);

		this->applyDHCP();

		if (this->ssid.equals("")) {
			WiFi.begin();
		} else {
			Serial.println("Connecting to: " + this->ssid);
			WiFi.begin(this->ssid.c_str(), this->pass.c_str());
		}
	}

	void loop() {
		uint64_t now = sysclock->mstime();
		uint32_t diff = (uint32_t) (now - this->lphase);
		if (this->iphase <= diff) {
			this->lphase = now;
			if (!this->ssid.equals("")) {				
				bool connected = (WL_CONNECTED == WiFi.status());
				// Serial.println("Checking connection: " + String(connected));
				if (!connected) {
					this->tryConnection = !this->tryConnection;
					if (this->tryConnection) {
						this->Log("{\"n\":\"log\",\"p\":\"Trying to connect to " + this->ssid + "\"}");
						WiFi.setAutoConnect(true);
						WiFi.setAutoReconnect(true);
						WiFi.begin(this->ssid.c_str(), this->pass.c_str());
					} else if (!tryConnection) {
						this->Log("{\"n\":\"log\",\"p\":\"WiFi client disconnected ...\"}");
						WiFi.disconnect();
					}
				}
			}
		}
	}

	void applyDHCP() {
		if (this->dhcp) {
			WiFi.config(0u, 0u, 0u);
		} else {
			IPAddress ip;
			ip.fromString(this->ip);
			IPAddress gateway;
			gateway.fromString(this->gateway);
			IPAddress netmask;
			netmask.fromString(this->netmask);
			IPAddress dns1;
			dns1.fromString(this->dns1);
			IPAddress dns2;
			dns2.fromString(this->dns2);
			WiFi.config(ip, gateway, netmask, dns1, dns2);
		}
	}

	void saveFile() {
		DynamicJsonDocument doc = this->JSON();
		String json = "";
		serializeJson(doc, json);
		File file = SPIFFS.open(this->filename, "w");
		file.println(json);
		file.close();
	}

	void readFile() {
		if (SPIFFS.exists(this->filename)) {
			File file = SPIFFS.open(this->filename, "r");
			String json = file.readStringUntil('\n');
			file.close();
			DynamicJsonDocument doc(1024);
			DeserializationError error = deserializeJson(doc, json);
			if (!error) {
				JsonObject login = doc.as<JsonObject>();
				this->fromJSON(login);
			} else {
				Serial.println("Login.readFile() : DeserializationError");
				Serial.println(json);
			}
		}
	}

	void fromJSON(JsonObject params) {
		if (params.containsKey("s")) { this->ssid = params["s"].as<String>(); }
		if (params.containsKey("p")) { this->pass = params["p"].as<String>(); }
		if (params.containsKey("mode")) { this->mode = params["mode"]; }
		if (params.containsKey("hn")) { this->hostname = params["hn"].as<String>(); }
		if (params.containsKey("dhcp")) { this->dhcp = params["dhcp"]; }
		if (params.containsKey("ip")) { this->ip = params["ip"].as<String>(); }
		if (params.containsKey("nm")) { this->netmask = params["nm"].as<String>(); }
		if (params.containsKey("gw")) { this->gateway = params["gw"].as<String>(); }
		if (params.containsKey("dns1")) { this->dns1 = params["dns1"].as<String>(); }
		if (params.containsKey("dns2")) { this->dns2 = params["dns2"].as<String>(); }

		Serial.println("this->ssid: " + this->ssid);
		Serial.println("this->pass: " + this->pass);
		Serial.println("this->mode: " + String(this->mode));
		Serial.println("this->hn: " + this->hostname);
		Serial.println("this->dhcp: " + String(this->dhcp));
		Serial.println("this->ip: " + this->ip);
		Serial.println("this->nm: " + this->netmask);
		Serial.println("this->gw: " + this->gateway);
		Serial.println("this->dns1: " + this->dns1);
	}

	DynamicJsonDocument JSON() {
		DynamicJsonDocument doc(1024);
		JsonObject wifi = doc.to<JsonObject>();
		wifi["s"] = this->ssid;
		wifi["p"] = this->pass;
		wifi["mode"] = this->mode;
		wifi["hn"] = this->hostname;
		wifi["dhcp"] = this->dhcp;
		wifi["ip"] = this->ip;
		wifi["nm"] = this->netmask;
		wifi["gw"] = this->gateway;
		wifi["dns1"] = this->dns1;
		wifi["dns2"] = this->dns2;
		return doc;
	}

	void Log(String text) {
		if (NULL != this->logger) {
			String log = "{\"n\":\"wifi\",\"p\":" + text + "}";
			this->logger->log(log);
		}
	}
};

#endif
