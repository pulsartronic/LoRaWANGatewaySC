#include <System.h>

System::NTP::NTP(Node* parent, const char* name) : Node(parent, name) {
	
}

void System::NTP::setup() {
	this->readFile();
	sntp_stop();
	sntp_setservername(0, (char*)this->settings.host.c_str());
	sntp_set_timezone(this->settings.tz);
	sntp_init();
}

void System::NTP::loop() {
	
}

void System::NTP::getState(JsonObject& state) {
	this->JSON(state);
	state["host"] = sntp_getservername(0);
	state["tz"] = sntp_get_timezone();
	state["now"] = sntp_get_current_timestamp();
}

void System::NTP::getPing(JsonObject& response) {
	JsonObject object = this->rootIT(response);
	JsonObject mparams = object.createNestedObject("state");
	mparams["now"] = sntp_get_current_timestamp();
}

void System::NTP::save(JsonObject& params, JsonObject& response, JsonObject& broadcast) {
	this->fromJSON(params);
	this->saveFile();
	this->setup();

	JsonObject object = this->rootIT(broadcast);
	JsonObject mparams = object.createNestedObject("state");
	this->getState(mparams);
}

void System::NTP::JSON(JsonObject& ntp) {
	ntp["host"] = this->settings.host;
	ntp["tz"] = this->settings.tz;
}

void System::NTP::fromJSON(JsonObject& params) {
	if (params.containsKey("host")) { this->settings.host = params["host"].as<String>(); }
	if (params.containsKey("tz")) { this->settings.tz = params["tz"]; }
}


