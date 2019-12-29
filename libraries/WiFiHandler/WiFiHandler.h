#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h> 
//#include <ESP8266WebServerSecure.h>
#include <ESP8266HTTPClient.h>

// #include <HTML.h>
#include <HTTPUserServer.h>

#include <DeviceSet.h>


// https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi/src
// https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/readme.html
class WiFiHandler : public Configurable {
	public:
	DeviceSet* deviceSet = NULL;
	DNSServer* dnsServer = NULL;
	HTTPUserServer* httpUserServer = NULL;
	ESP8266WebServer* server = NULL;

	const byte DNS_PORT = 53;
	String name = String(ESP.getChipId());

	virtual ~WiFiHandler() {
		delete this->dnsServer;
		delete this->httpUserServer;
		delete this->server;
	}

	WiFiHandler(DeviceSet* deviceSet) {
		Serial.println(F("Starting Access Point ..."));

		this->deviceSet = deviceSet;

		wifi_set_phy_mode(PHY_MODE_11B);

		WiFiMode_t mode = WiFi.getMode();
		WiFi.mode(WIFI_AP_STA);
		//if (WIFI_AP_STA != mode) {
		String defaultPassword = "12345678"; // TODO:: change default password generation
		WiFi.softAP(this->name, defaultPassword);//, 1, true);
		//}

		Serial.println("SSID: " + this->name);

		delay(500); // Without delay I've seen the IP address blank
		IPAddress APIP = WiFi.softAPIP();
		Serial.println("WiFi.softAPIP(): " + APIP.toString());

		this->dnsServer = new DNSServer();
		// Setup the DNS server redirecting all the domains to the apIP
		this->dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
		this->dnsServer->start(DNS_PORT, "*", APIP);

		this->server = new ESP8266WebServer(/*APIP,*/ 80);
		this->server->on(F("/"), std::bind(&WiFiHandler::handleRoot, this));
		this->server->on(F("/hotspot/scripts.js"), std::bind(&WiFiHandler::scripts, this, server));
		this->server->on(F("/hotspot/styles.css"), std::bind(&WiFiHandler::styles, this, server));
		//this->server->on(String(F("/ping")), std::bind(&WiFiHandler::onPing, this));
		this->server->onNotFound (std::bind(&WiFiHandler::handleNotFound, this));
		// Android Captive Portal
		this->server->on(String(F("/generate_204")), std::bind(&WiFiHandler::handleRoot, this));
		// Windows Captive Portal
		this->server->on(String(F("/fwlink")), std::bind(&WiFiHandler::handleRoot, this));
		// iOS Captive Portal ????
		this->server->on(String(F("/hotspot-detect.html")), std::bind(&WiFiHandler::handleRoot, this));

		byte key[] = { // SHA256("adminadmin") "userpass"
			0xd8, 0x24, 0x94, 0xf0, 0x5d, 0x69, 0x17, 0xba, 
			0x02, 0xf7, 0xaa, 0xa2, 0x96, 0x89, 0xcc, 0xb4,
			0x44, 0xbb, 0x73, 0xf2, 0x03, 0x80, 0x87, 0x6c,
			0xb0, 0x5d, 0x1f, 0x37, 0x53, 0x7b, 0x78, 0x92};
		//this->httpUserServer = new HTTPUserServer(this->server, key);

		unsigned int length = this->deviceSet->length();
		for (unsigned int i = 0; i < length; i++) {
			Configurable* configurable = this->deviceSet->configurable(i);
			//configurable->registerHandlers(this->server);
			this->httpUserServer->registerConfigurable(configurable);
		}
		this->server->begin(); // Web server start
	}

	String getName() {
		return "gw";
	}

	void handleRoot() {
		//server->send_P(200, "text/html", WiFiHandlerHTML);
	}

	void scripts(ESP8266WebServer* server) {
		//server->send_P(200, "application/javascript", WiFiHandlerScripts);
	}

	void styles(ESP8266WebServer* server) {
		//server->send_P(200, "text/css", WiFiHandlerStyles);
	}

	void onPing() {
		String pong = this->pong();
		this->server->sendHeader(F("Access-Control-Allow-Origin"), F("*"));
		this->server->sendHeader(F("Access-Control-Allow-Headers"), F("Origin, X-Requested-With, Content-Type, Accept"));
		this->server->sendHeader(F("Content-Length"), String(pong.length()));
		this->server->send(200, F("application/json"), pong);
	}

	// TODO::
	// Reset default settings of following APIs:
	// wifi_station_set_auto_connect,
	// wifi_set_phy_mode,
	// wifi_softap_set_config related,
	// wifi_station_set_config related,
	// wifi_set_opmode,
	// and APs’ information recorded by #define AP_CACHE.
	void restoreDefaults() {
		system_restore();
		system_restart();
	}

	String pong() {
		String pong = "{";

		unsigned int length = this->deviceSet->length();
		for (unsigned int i = 0; i < length; i++) {
			Configurable* configurable = this->deviceSet->configurable(i);
			String name = configurable->getName();
			String cpong = configurable->ping();
			pong += "\"" + name + "\":" + cpong + ",";
		}

		String hotspot = this->ping();
		pong += "\"hotspot\":" + hotspot + "}";

		//const char* sdkVersion = system_get_sdk_version();
		//uint32 freeHeap = system_get_free_heap_size();
		//uint8 cpuFreq = system_get_cpu_freq();
		//flash_size_map flashSizeMap = system_get_flash_size_map();
		//Serial.println("SDK: " + String(sdkVersion));
		//Serial.println("cpuFreq: " + String(cpuFreq));
		// FLASH_SIZE_4M_MAP_256_256 = 0,
		// FLASH_SIZE_2M,
		// FLASH_SIZE_8M_MAP_512_512,
		// FLASH_SIZE_16M_MAP_512_512,
		// FLASH_SIZE_32M_MAP_512_512,
		// FLASH_SIZE_16M_MAP_1024_1024,
		// FLASH_SIZE_32M_MAP_1024_1024,
		// FLASH_SIZE_64M_MAP_1024_1024,
		// FLASH_SIZE_128M_MAP_1024_1024

		return pong;
	}

	String cping() {
		
	}

	String ping() {
		struct softap_config conf;
		wifi_softap_get_config(&conf);

		struct ip_info info;
		wifi_get_ip_info(SOFTAP_IF, &info);

		int phymode = wifi_get_phy_mode();

		String hotspot = "{\"ip\":\"" + WiFi.softAPIP().toString() + "\"";
		hotspot += ",\"mode\":" + String(phymode);
		hotspot += ",\"mac\":\"" + WiFi.softAPmacAddress() + "\"";
		hotspot += ",\"ssid\":\"" + String((char*)conf.ssid) + "\"";
		hotspot += ",\"stations\":\"" + String(WiFi.softAPgetStationNum()) + "\"";
		hotspot += ",\"channel\":\"" + String(conf.channel) + "\"";
		hotspot += ",\"hidden\":" + String(conf.ssid_hidden);
		hotspot += ",\"gw\":\"" + IPAddress(info.gw.addr).toString() + "\"";
		hotspot += ",\"netmask\":\"" + IPAddress(info.netmask.addr).toString() + "\"}";

		return hotspot;
	}

	void onHotspotSaved() {
		String name = server->arg("n");
		String password = server->arg("p");
		//Serial.println("Hotspot saved: " + name + " ::: " + password);
		String pong = this->pong();
		this->server->sendHeader(F("Access-Control-Allow-Origin"), F("*"));
		this->server->sendHeader(F("Access-Control-Allow-Headers"), F("Origin, X-Requested-With, Content-Type, Accept"));
		this->server->sendHeader(F("Content-Length"), String(pong.length()));
		this->server->send(200, F("application/json"), pong);
		WiFi.softAP(name, password);
	}
    
	void handleNotFound() {
		String message = F("File Not Found\n\n");
		//message += "URI: ";
		//message += server->uri();
		//message += "\nMethod: ";
		//message += ( server->method() == HTTP_GET ) ? "GET" : "POST";
		//message += "\nArguments: ";
		//message += server->args();
		//message += "\n";

		//for ( uint8_t i = 0; i < server->args(); i++ ) {
		//	message += " " + server->argName ( i ) + ": " + server->arg ( i ) + "\n";
		//}

		server->sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
		server->sendHeader(F("Pragma"), F("no-cache"));
		server->sendHeader(F("Expires"), F("-1"));
		server->sendHeader(F("Content-Length"), String(message.length()));
		server->send(404, F("text/plain"), message );

		//Serial.println("Not found");
		//Serial.println(message);
	}

	void loop() {
		//DNS
		this->dnsServer->processNextRequest();
		//HTTP
		this->server->handleClient();
	}
};

