#include <WIFI.h>

WIFI::WIFI(Node* parent, const char* name) : Node(parent, name) {
	this->client = new WIFI::Client(this, "client");
	this->nodes->set(this->client->name, this->client);

	this->ap = new WIFI::AP(this, "ap");
	this->nodes->set(this->ap->name, this->ap);
}

WIFI::~WIFI() {
	delete this->client;
	delete this->ap;
}

void WIFI::setup() {
	WiFi.mode(WIFI_AP_STA);
	this->applySettings();

	this->client->setup();
	this->ap->setup();
}

void WIFI::applySettings() {
	wifi_set_phy_mode((phy_mode_t) this->settings.bgn);
};

void WIFI::loop() {
	this->client->loop();
	this->ap->loop();
}

void WIFI::state(JsonObject& params, JsonObject& response, JsonObject& broadcast) {
	JsonObject object = this->rootIT(response);
	JsonObject state = object.createNestedObject("state");
	this->JSON(state);
	state["bgn"] = (int) wifi_get_phy_mode();
	state["mode"] = WiFi.getMode();
}

void WIFI::fromJSON(JsonObject& params) {
	if (params.containsKey("bgn")) { this->settings.bgn = params["bgn"]; }
}

void WIFI::JSON(JsonObject& wifi) {
	wifi["bgn"] = this->settings.bgn;
	wifi["mode"] = this->settings.mode;
}

