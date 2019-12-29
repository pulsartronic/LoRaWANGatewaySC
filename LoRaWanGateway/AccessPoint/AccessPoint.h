#ifndef __AccessPoint__
#define __AccessPoint__

#include <DNSServer.h>

class AccessPoint {
	public:
	DNSServer* dnsServer = NULL;
	String filename = "ap.json";
	String ssid = "";
	String pass = "";

	virtual ~AccessPoint() {
		
	}

	AccessPoint() {

	}

	void setup() {
		Serial.println(F("Starting Access Point ..."));

		// uint32_t id = ESP.getChipId();
		this->ssid = "LoRa WAN Gateway";
		this->pass = "12345678";

		this->readFile();

		WiFiMode_t mode = WiFi.getMode();
		WiFi.mode(WIFI_AP_STA);
		WiFi.softAP(this->ssid, this->pass);//, 1, true);

		Serial.println("SSID: " + this->ssid);

		delay(500); // Without delay I've seen the IP address blank
		IPAddress APIP = WiFi.softAPIP();
		Serial.println("WiFi.softAPIP(): " + APIP.toString());

		this->dnsServer = new DNSServer();
		// Setup the DNS server redirecting all the domains to the apIP
		this->dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
		this->dnsServer->start(53, "*", APIP);
	}

	void loop() {
		this->dnsServer->processNextRequest();
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
			DynamicJsonDocument doc(512);
			DeserializationError error = deserializeJson(doc, json);
			if (!error) {
				JsonObject ap = doc.as<JsonObject>();
				this->fromJSON(ap);
			} else {
				Serial.println("AccessPoint.readFile() : DeserializationError");
				Serial.println(json);
			}
		}
	}

	void fromJSON(JsonObject params) {
		if (params.containsKey("ssid")) {
			this->ssid = params["ssid"].as<String>();
			Serial.println("AP: ssid " + this->ssid);
		}

		if (params.containsKey("pass")) {
			this->pass = params["pass"].as<String>();
			Serial.println("AP: pass " + this->pass);
		}
	}

	DynamicJsonDocument JSON() {
		DynamicJsonDocument doc(512);
		JsonObject ap = doc.to<JsonObject>();
		ap["ssid"] = this->ssid;
		ap["pass"] = this->pass;
		return doc;
	}

};

#endif
