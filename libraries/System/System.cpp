#include <System.h>

int System::Pins::length = 9;
//int System::Pins::VALUE[] = {D0, D1, D2, D3, D4, D5, D6, D7, D8};
//int System::Pins::VALUE[] = {16,  5,  4,  0,  2, 14, 12, 13, 15};


Data::Packet::Packet(uint16_t size) {
	this->size = size;
	this->buffer = new uint8_t[size];
}

Data::Packet::~Packet() {
	delete[] this->buffer;
}


System::System(Node* parent, const char* name) : Node(parent, name) {
	this->ntp = new NTP(this, "ntp");
	this->nodes->set(this->ntp->name, this->ntp);

	this->espSystem = new ESPS(this, "esps");
	this->nodes->set(this->espSystem->name, this->espSystem);

	Method* upgrade = new Method(std::bind(&System::upgrade, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	this->methods->set("upgrade", upgrade);
}

System::~System() {
	delete this->ntp;
	delete this->espSystem;
}

void System::setup() {
	this->ntp->setup();
}

void System::loop() {
	this->ntp->loop();
	this->espSystem->loop();
}

void System::state(JsonObject& params, JsonObject& response, JsonObject& broadcast) {
	JsonObject object = this->rootIT(response);
	JsonObject state = object.createNestedObject("state");
	state["version"] = this->settings.version;
	state["vurl"] = this->settings.vurl;
	state["up"] = String(clock64.high()) + String(clock64.low);
}

void System::upgrade(JsonObject& params, JsonObject& response, JsonObject& broadcast) {
	JsonObject object = this->rootIT(response);
	JsonObject upgrading = object.createNestedObject("upgrading");

	//DEBUG.println("UPGRADING ::::::");
	String host = this->settings.vurl + "/version.php";
	WiFiClient client;
	HTTPClient http;
	if (http.begin(client, host)) {
		int httpCode = http.GET();
		if (httpCode > 0) {
			if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
				String payload = http.getString();
				int last = payload.toInt();
				//DEBUG.println("payload: " + payload);
				if (last > VERSION) {
					//DEBUG.println("Last is greather than actual, UPGRADING !!!");
					WiFiClient client;
					ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
					ESPhttpUpdate.rebootOnUpdate(true);
					t_httpUpdate_return ret = ESPhttpUpdate.update(client, this->settings.vurl + "/d1_80mhz_4mb_1mbspifss_latest.bin");
					switch (ret) {
						case HTTP_UPDATE_FAILED:
							upgrading["error"] = 9;
							//DEBUG.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
						break;
						case HTTP_UPDATE_NO_UPDATES:
							upgrading["error"] = 10;
							//DEBUG.println("HTTP_UPDATE_NO_UPDATES");
						break;
						case HTTP_UPDATE_OK:
							upgrading["error"] = false;
							//DEBUG.println("HTTP_UPDATE_OK");
						break;
						default:
							upgrading["error"] = 15;
						break;
					}
				} else {
					upgrading["error"] = 14;
				}
			} else {
				upgrading["error"] = 16;
			}
		} else {
			upgrading["error"] = 13;
			//DEBUG.println(String(httpCode));
			//DEBUG.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
		}
		http.end();
	} else {
		upgrading["error"] = 12;
		//DEBUG.printf("[HTTP} Unable to connect\n");
	}
}
