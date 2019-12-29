#ifndef __Login__
#define __Login__

#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <AESM.h>
#include <KeyValueMap.h>
#include <Configurable.h>
#include <DeviceSet.h>
#include <Logger.h>

class Login : public WebSocketEventReceiver, public Logger {
	public:
	DeviceSet* deviceSet = NULL;
	HTTPUserServer* httpUserServer = NULL;
	WebSocketsServer* webSocketServer = NULL;
	KeyValueMap<Configurable>* configurablesMap = NULL;
	AESM* aesm = NULL;
	uint32_t bid = 0ul;
	uint32_t lid = 0ul;

	String filename = "login.json";
	byte* key = NULL;

	virtual ~Login() {
		delete this->aesm;
		delete this->configurablesMap;
		delete[] this->key;
	}

	Login(DeviceSet* deviceSet, HTTPUserServer* httpUserServer) {
		this->deviceSet = deviceSet;
		this->httpUserServer = httpUserServer;
	}

	void setup() {
		Serial.println(F("Starting Login system ..."));

		this->httpUserServer->server->on(F("/u"), std::bind(&Login::user, this));
		this->httpUserServer->server->on(F("/s"), std::bind(&Login::session, this));

		this->bid = RANDOM_REG32;
		this->lid = millis();

		byte defaultKey[] = { // SHA256("adminadmin") "userpass"
			0xd8, 0x24, 0x94, 0xf0, 0x5d, 0x69, 0x17, 0xba, 
			0x02, 0xf7, 0xaa, 0xa2, 0x96, 0x89, 0xcc, 0xb4,
			0x44, 0xbb, 0x73, 0xf2, 0x03, 0x80, 0x87, 0x6c,
			0xb0, 0x5d, 0x1f, 0x37, 0x53, 0x7b, 0x78, 0x92};
		this->key = new byte[32];
		for (int i = 0; i < 32; i++) this->key[i] = defaultKey[i];
		this->readFile();
		this->aesm = new AESM((byte*) key);
		this->configurablesMap = new KeyValueMap<Configurable>();

		this->webSocketServer = new WebSocketsServer(3498);
		this->webSocketServer->webSocketEventReceiver = this; // TODO: fix it
		this->webSocketServer->begin();
		//std::function<void(uint8_t, WStype_t, uint8_t*, size_t)> onEvent = std::bind(&LoRaConfigurable::webSocketEvent, this);
		//webSocket.onEvent(webSocketEvent);

		unsigned int length = this->deviceSet->length();
		for (unsigned int i = 0; i < length; i++) {
			Configurable* configurable = this->deviceSet->configurable(i);
			this->registerConfigurable(configurable);
		}

	}

	void loop() {
		this->webSocketServer->loop();
	}

	/**
	 * WebSocketServer Events
	 */
	void onEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
		// TODO:: handle client messages
	}

	void log(String text) {
		String edata = this->aesm->encrypt((byte*) text.c_str(), text.length());
		String log = "{\"data\":\"" + edata + "\"}";
		this->webSocketServer->broadcastTXT(log);
	}

	void LID() {
		uint32_t m = millis();
		if (this->lid > m) {
			this->bid = RANDOM_REG32;
			this->lid = m;
		}
	}

	String JLID() {
		String jlid = "{\"b\":" + String(this->bid) + ",\"l\":" + String(this->lid) + "}";
		return jlid;
	}

	// from this->server
	void session() {
		this->LID(); // don't forget to lid
		String jlid = this->JLID();
		this->respond(jlid);
	}

	// from this->server
	void user() {
		this->LID(); // don't forget to lid

		String edata = this->httpUserServer->server->arg("d");
		String data = this->aesm->decrypt((byte*) edata.c_str());

		DynamicJsonDocument doc(1024);
		DeserializationError error = deserializeJson(doc, data);
		if (error) {
			Serial.println("ERROR: DeserializationError");
			this->sendLID();
		} else {
			JsonObject object = doc.as<JsonObject>();
			JsonObject id = object["id"];
			uint32_t l = id["l"];
			uint32_t b = id["b"];
			if (this->bid == b && this->lid < l) {
				this->lid = l;
				const char* name = object["n"];
				Configurable* configurable = this->configurablesMap->get((char*) name);
				if (NULL != configurable) {
					JsonObject params = object["p"];
					String data = configurable->command(params);
					String edata = this->aesm->encrypt((byte*) data.c_str(), data.length());
					String response = "{\"data\":\"" + edata + "\"}";
					this->respond(response);
				} else {
					Serial.println("ERROR: NULL == configurable");
					this->sendLID();
				}
			} else {
				Serial.println("this->bid == b && this->lid < l");
				this->sendLID();
			}
		}
	}

	void sendLID() {
		String jlid = this->JLID();
		String response = "{\"error\":1,\"jlid\":" + jlid + "}";
		this->respond(response);
	}

	void registerConfigurable(Configurable* configurable) {
		String name = configurable->getName();
		char* key = (char*) name.c_str();
		this->configurablesMap->set(key, configurable);
	}

	void respond(String response) {
		this->httpUserServer->server->sendHeader(F("Access-Control-Allow-Origin"), F("*"));
		this->httpUserServer->server->sendHeader(F("Access-Control-Allow-Headers"), F("Origin, X-Requested-With, Content-Type, Accept"));
		this->httpUserServer->server->sendHeader(F("Content-Length"), String(response.length()));
		this->httpUserServer->server->send(200, F("text/plain"), response);
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
				JsonObject login = doc.as<JsonObject>();
				this->fromJSON(login);
			} else {
				Serial.println("Login.readFile() : DeserializationError");
				Serial.println(json);
			}
		}
	}

	void fromJSON(JsonObject params) {
		if (params.containsKey("key")) {
			String keySTR = params["key"];
			byte* base64 = (byte*) keySTR.c_str();
			unsigned int binary_length = decode_base64_length(base64);
			if (32 == binary_length) {
				decode_base64(base64, this->key);
			}
		}
	}

	DynamicJsonDocument JSON() {
		DynamicJsonDocument doc(512);
		JsonObject login = doc.to<JsonObject>();
		unsigned int b64Length = (4 * (32 + 2) / 3) + 2; // TODO:: what ?
		unsigned char base64[b64Length];
		for (int i = 0; i < b64Length; i++) base64[i] = '\0';
		unsigned int base64_length = encode_base64(this->key, 32, base64);
		login["key"] = String((char*) base64);
		return doc;
	}
};

#endif
