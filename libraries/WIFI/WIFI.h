#include <Node.h>
#include <DNSServer.h>
#include <SystemClock.h>
// https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <FS.h>

#ifndef __WIFI__
#define __WIFI__

class WIFI : public Node {
	public:

	class Client : public Node {
		public:
		class Settings {
			public:
			String ssid = "";
			String pass = "";
			String hostname = "";
			int dhcp = 1;
			String ip = "";
			String gateway = "";
			String netmask = "";
			String dns1 = "";
			String dns2 = "";
		};

		public:
		Settings settings;

		uint32_t iphase = 1ul * 60ul * 1000ul; // 1 minute TODO:: 
		uint64_t lphase = 0ull;

		bool tryConnection = false;

		bool schedule = false;
		uint64_t scheduled = 0ull;
		bool credentials = false;
		bool ipshown = false;

		Client(Node* parent, const char* name);
		virtual ~Client();
		void setup();
		void loop();
		void applyDHCP();

		void scan(JsonObject& params, JsonObject& response, JsonObject& broadcast);
		void network(JsonObject& params, JsonObject& response, JsonObject& broadcast);

		virtual void getPing(JsonObject& state);
		virtual void state(JsonObject& params, JsonObject& response, JsonObject& broadcast);
		virtual void fromJSON(JsonObject& params);
		virtual void JSON(JsonObject& params);
		virtual void save(JsonObject& params, JsonObject& response, JsonObject& broadcast);
		virtual void disconnect(JsonObject& params, JsonObject& response, JsonObject& broadcast);
	};

	class AP : public Node {
		public:
		class Settings {
			public:
			String ssid = "";
			String pass = "";
			bool hidden = false;
			uint16_t beacon = 500u;
			uint16_t maxconn = 1u;
			uint16_t channel = 1u;
			String ip = "";
			String gateway = "";
			String netmask = "";
		};

		DNSServer* dnsServer = NULL;
		Settings settings;

		AP(Node* parent, const char* name);
		virtual ~AP();
		void setup();
		void loop();

		virtual void applySettings();
		virtual void state(JsonObject& params, JsonObject& response, JsonObject& broadcast);
		virtual void fromJSON(JsonObject& params);
		virtual void JSON(JsonObject& params);
	};

	class Settings {
		public:
		int mode = WIFI_AP_STA;
		int bgn = PHY_MODE_11B;
	};

	Client* client = NULL;
	AP* ap = NULL;
	Settings settings;

	WIFI(Node* parent, const char* name);
	virtual ~WIFI();
	void setup();
	void loop();

	virtual void applySettings();
	virtual void state(JsonObject& params, JsonObject& response, JsonObject& broadcast);
	virtual void fromJSON(JsonObject& params);
	virtual void JSON(JsonObject& params);
};

#endif
