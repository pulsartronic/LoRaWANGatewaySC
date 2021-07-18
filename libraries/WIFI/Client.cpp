#include <WIFI.h>

WIFI::Client::Client(Node* parent, const char* name) : Node(parent, name) {
	Method* scan = new Method(std::bind(&WIFI::Client::scan, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	this->methods->set("scan", scan);

	Method* network = new Method(std::bind(&WIFI::Client::network, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	this->methods->set("network", network);

	Method* disconnect = new Method(std::bind(&WIFI::Client::disconnect, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	this->methods->set("disconnect", disconnect);
}

WIFI::Client::~Client() {
	
}

void WIFI::Client::setup() {
	this->settings.hostname = WiFi.hostname();
	this->readFile();

	WiFi.hostname(this->settings.hostname);

	WiFi.setAutoConnect(true);
	WiFi.setAutoReconnect(true);

	this->applyDHCP();

	if (this->settings.ssid.equals("")) {
		WiFi.begin();
	} else {
		WiFi.begin(this->settings.ssid.c_str(), this->settings.pass.c_str());
	}
}

void WIFI::Client::loop() {
	uint64_t now = clock64.mstime();
	uint32_t diff = (uint32_t) (now - this->lphase);

	if (this->iphase <= diff) {
		this->lphase = now;
		if (!this->settings.ssid.equals("")) {				
			bool connected = (WL_CONNECTED == WiFi.status());
			if (!connected) {
				this->tryConnection = !this->tryConnection;
				if (this->tryConnection) {
					this->log("Trying to connect to " + this->settings.ssid);
					WiFi.setAutoConnect(true);
					WiFi.setAutoReconnect(true);
					WiFi.begin(this->settings.ssid.c_str(), this->settings.pass.c_str());
				} else if (!tryConnection) {
					String log = "WiFi client disconnected ...";
					this->log(log);
					WiFi.disconnect();
				}
			}
		}
	}

	if (this->schedule) {
		uint32_t sdiff = (uint32_t) (now - this->scheduled);
		if (2000ul < sdiff) {
			this->schedule = false;
			yield();
			this->applyDHCP();
			WiFi.hostname(this->settings.hostname);
			yield();

			if (this->credentials) {
				WiFi.disconnect();
				delay(100);
				WiFi.begin(this->settings.ssid.c_str(), this->settings.pass.c_str());
			}

			DynamicJsonDocument commandDocument(512);
			JsonObject command = commandDocument.to<JsonObject>();
			JsonObject object = this->rootIT(command);
			JsonObject mparams = object.createNestedObject("state");
			this->getState(mparams);
			this->command(command);
		}
	}

	if ((WL_CONNECTED == WiFi.status()) && !ipshown) {
		ipshown = true;
		Serial.println("Connected to : " + WiFi.BSSIDstr() + " ip: " + WiFi.localIP().toString());
	}
}

void WIFI::Client::applyDHCP() {
	if (this->settings.dhcp) {
		WiFi.config(0u, 0u, 0u);
	} else {
		IPAddress ip;
		ip.fromString(this->settings.ip);
		IPAddress gateway;
		gateway.fromString(this->settings.gateway);
		IPAddress netmask;
		netmask.fromString(this->settings.netmask);
		IPAddress dns1;
		dns1.fromString(this->settings.dns1);
		IPAddress dns2;
		dns2.fromString(this->settings.dns2);
		WiFi.config(ip, gateway, netmask, dns1, dns2);
	}
	yield();
	
	//struct station_config {
	//		uint8 ssid[32];
	//		uint8 password[64];
	//		uint8 bssid_set;
	//		// Note: If bssid_set is 1, station will just connect to the router
	//		// with both ssid[] and bssid[] matched. Please check about this.
	//		uint8 bssid[6];
	//		wifi_fast_scan_threshold_t threshold;
	//};
	// struct station_config config;
	// wifi_station_get_config(&config);

	this->ipshown = false;
}

void WIFI::Client::state(JsonObject& params, JsonObject& response, JsonObject& broadcast) {
	JsonObject object = this->rootIT(response);
	JsonObject state = object.createNestedObject("state");
	this->JSON(state);

	dhcp_status dhcpstatus = wifi_station_dhcpc_status();
	int dhcp = (DHCP_STOPPED == dhcpstatus) ? 0 : 1;

	state["status"] = (int) WiFi.status();
	state["ssid"] = WiFi.SSID();
	state["cssid"] = this->settings.ssid;
	state["bssid"] = WiFi.BSSIDstr();
	state["mac"] = WiFi.macAddress();
	state["ip"] = WiFi.localIP().toString();
	state["sm"] = WiFi.subnetMask().toString();
	state["gw"] = WiFi.gatewayIP().toString();
	state["dns1"] = WiFi.dnsIP(0).toString();
	state["dns2"] = WiFi.dnsIP(1).toString();
	state["hn"] = WiFi.hostname();
	state["ss"] = WiFi.RSSI();
	state["dhcp"] = dhcp;
}

void WIFI::Client::getPing(JsonObject& response) {
	JsonObject object = this->rootIT(response);
	JsonObject mparams = object.createNestedObject("state");
	mparams["status"] = (int) WiFi.status();
	mparams["ssid"] = WiFi.SSID();
	mparams["cssid"] = this->settings.ssid;
	mparams["ss"] = WiFi.RSSI();
}

void WIFI::Client::fromJSON(JsonObject& params) {
	if (params.containsKey("ssid")) { this->settings.ssid = params["ssid"].as<String>(); }
	if (params.containsKey("pass")) { this->settings.pass = params["pass"].as<String>(); }

	if (params.containsKey("hn")) { this->settings.hostname = params["hn"].as<String>(); }
	if (params.containsKey("dhcp")) { this->settings.dhcp = params["dhcp"]; }
	if (params.containsKey("ip")) { this->settings.ip = params["ip"].as<String>(); }
	if (params.containsKey("nm")) { this->settings.netmask = params["nm"].as<String>(); }
	if (params.containsKey("gw")) { this->settings.gateway = params["gw"].as<String>(); }
	if (params.containsKey("dns1")) { this->settings.dns1 = params["dns1"].as<String>(); }
	if (params.containsKey("dns2")) { this->settings.dns2 = params["dns2"].as<String>(); }
}

void WIFI::Client::JSON(JsonObject& wifi) {
	wifi["ssid"] = this->settings.ssid;
	wifi["pass"] = this->settings.pass;
	wifi["hn"] = this->settings.hostname;
	wifi["dhcp"] = this->settings.dhcp;
	wifi["ip"] = this->settings.ip;
	wifi["nm"] = this->settings.netmask;
	wifi["gw"] = this->settings.gateway;
	wifi["dns1"] = this->settings.dns1;
	wifi["dns2"] = this->settings.dns2;
}

void WIFI::Client::disconnect(JsonObject& params, JsonObject& response, JsonObject& broadcast) {
	WiFi.disconnect();
	yield();
	this->settings.ssid = "";
	this->settings.pass = "";
	this->saveFile();
}

void WIFI::Client::save(JsonObject& params, JsonObject& response, JsonObject& broadcast) {
	this->fromJSON(params);
	this->saveFile();

	this->schedule = true;
	this->scheduled = clock64.mstime();

	bool hasSSID = params.containsKey("ssid");
	bool hasPASS = params.containsKey("pass");
	this->credentials = hasSSID && hasPASS;
	if (this->credentials) {
		this->lphase = clock64.mstime();
		this->log("WiFi saved, connecting to " + this->settings.ssid);
	}
}

void WIFI::Client::scan(JsonObject& params, JsonObject& response, JsonObject& broadcast) {
	JsonObject object = this->rootIT(response);
	JsonObject mparams = object.createNestedObject("scanned");
	WiFi.scanDelete();
	int networksFound = WiFi.scanNetworks();
	mparams["value"] = networksFound;
}

void WIFI::Client::network(JsonObject& params, JsonObject& response, JsonObject& broadcast) {
	JsonObject object = this->rootIT(response);
	JsonObject mparams = object.createNestedObject("network");
	String ii = params["i"];
	int index = ii.toInt();
	mparams["ssid"] = WiFi.SSID(index);
	mparams["chn"] = String(WiFi.channel(index));
	mparams["rssi"] = String(WiFi.RSSI(index));
	mparams["opn"] = String(WiFi.encryptionType(index) == ENC_TYPE_NONE);
	mparams["index"] = index;
}

