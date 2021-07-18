#include <WIFI.h>

WIFI::AP::AP(Node* parent, const char* name) : Node(parent, name) {

}

WIFI::AP::~AP() {
	
}

void WIFI::AP::setup() {
	this->readFile();
	this->applySettings();
	//DEBUG.println("Starting Access Point ... OK");
	//DEBUG.println("SSID: " + this->settings.ssid);
	//IPAddress APIP = WiFi.softAPIP();
	//DEBUG.println("IP: " + APIP.toString());
}

void WIFI::AP::applySettings() {
	WiFi.softAP(this->settings.ssid, this->settings.pass);//, 1, true);
	struct softap_config conf;
	wifi_softap_get_config(&conf);
	conf.beacon_interval = this->settings.beacon;
	conf.channel = this->settings.channel;
	conf.ssid_hidden = this->settings.hidden;
	conf.max_connection = this->settings.maxconn;
	// TODO:: add more configurations
	wifi_softap_set_config(&conf);

	struct ip_info info;
	wifi_get_ip_info(SOFTAP_IF, &info);
	// TODO:: add more configurations
	wifi_set_ip_info(SOFTAP_IF, &info);
	yield();

	if (NULL != this->dnsServer) {
		this->dnsServer->stop();
		delete this->dnsServer;
	}

	this->dnsServer = new DNSServer();
	// Setup the DNS server redirecting all the domains to the apIP
	IPAddress APIP = WiFi.softAPIP();
	this->dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
	this->dnsServer->start(53, "*", APIP);
};


void WIFI::AP::loop() {
	this->dnsServer->processNextRequest();
}

void WIFI::AP::state(JsonObject& params, JsonObject& response, JsonObject& broadcast) {
	JsonObject object = this->rootIT(response);
	JsonObject state = object.createNestedObject("state");

	this->JSON(state);

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

	state["ip"] = WiFi.softAPIP().toString();
	state["mac"] = WiFi.softAPmacAddress();
	state["ssid"] = String((char*)conf.ssid);
	state["stations"] = WiFi.softAPgetStationNum();
	state["channel"] = conf.channel;
	state["hidden"] = conf.ssid_hidden;
	state["beacon"] = conf.beacon_interval;
	state["gw"] = IPAddress(info.gw.addr).toString();
	state["netmask"] = IPAddress(info.netmask.addr).toString();
}

void WIFI::AP::fromJSON(JsonObject& params) {
	if (params.containsKey("ssid")) {
		this->settings.ssid = params["ssid"].as<String>();
		//DEBUG.println("AP: ssid " + this->settings.ssid);
	}

	if (params.containsKey("pass")) {
		this->settings.pass = params["pass"].as<String>();
		//DEBUG.println("AP: pass " + this->settings.pass);
	}

	if (params.containsKey("hidden")) { this->settings.hidden = params["hidden"]; }
	if (params.containsKey("beacon")) { this->settings.beacon = params["beacon"]; }
	if (params.containsKey("maxconn")) { this->settings.maxconn = params["maxconn"]; }
	if (params.containsKey("channel")) { this->settings.channel = params["channel"]; }
	if (params.containsKey("ip")) { this->settings.ip = params["ip"].as<String>(); }
	if (params.containsKey("gateway")) { this->settings.gateway = params["gateway"].as<String>(); }
	if (params.containsKey("netmask")) { this->settings.netmask = params["netmask"].as<String>(); }
}

void WIFI::AP::JSON(JsonObject& ap) {
	ap["ssid"] = this->settings.ssid;
	ap["pass"] = this->settings.pass;
	ap["hidden"] = this->settings.hidden;
	ap["beacon"] = this->settings.beacon;
	ap["maxconn"] = this->settings.maxconn;
	ap["channel"] = this->settings.channel;
	ap["ip"] = this->settings.ip;
	ap["gateway"] = this->settings.gateway;
	ap["netmask"] = this->settings.netmask;
}


