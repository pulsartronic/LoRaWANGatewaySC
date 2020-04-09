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
	// WiFiMode_t mode = WiFi.getMode();
	WiFi.mode(WIFI_AP_STA);
	this->applySettings();

	this->client->setup();
	this->ap->setup();
}

void WIFI::applySettings() {
	wifi_set_phy_mode((phy_mode_t) this->settings.mode);
};

void WIFI::loop() {
	this->client->loop();
	this->ap->loop();
}

void WIFI::getState(JsonObject& state) {
	this->JSON(state);
	state["mode"] = (int) wifi_get_phy_mode();
}

void WIFI::fromJSON(JsonObject& params) {
	if (params.containsKey("mode")) { this->settings.mode = params["mode"]; }
}

void WIFI::JSON(JsonObject& wifi) {
	wifi["mode"] = this->settings.mode;
}

void WIFI::save(JsonObject& params, JsonObject& response, JsonObject& broadcast) {
	this->fromJSON(params);
	this->saveFile();
	this->applySettings();
	JsonObject object = this->rootIT(broadcast);
	JsonObject mparams = object.createNestedObject("state");
	this->getState(mparams);
}


