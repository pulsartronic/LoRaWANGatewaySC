#include <Node.h>
#include <WebSocketsServer.h>
#include <SystemClock.h>
#include <ESP8266WebServer.h>
#define ARDUINOJSON_USE_DOUBLE 1
#include <ArduinoJson.h>
#include <AESM.h>

#ifndef __NetworkNode__
#define __NetworkNode__

class NetworkNode : public Node {
	public:

	class Settings {
		public:
		int hport = 80;
		int wport = 3498;
	};

	ESP8266WebServer* httpServer = NULL;
	WebSocketsServer* webSocketServer = NULL;
	Settings settings;

	AESM* aesm = NULL;
	byte* key = NULL;
	uint32_t bid = 0ul;
	uint32_t lid = 0ul;

	NetworkNode(Node* parent, const char* name);
	virtual ~NetworkNode();

	virtual void setup();
	virtual void loop();

	virtual void applySettings();

	public:
	virtual void handleNotFound();
	virtual void html();
	void session();
	void user();
	void onEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length);

	void sendHeaders(); // -->>
	void httpResponse(JsonObject& responseObject, JsonObject& broadcastObject); // -->>

	virtual JsonObject rootIT(JsonObject& root) {
		return root;
	}

	virtual void command(JsonObject& command) {
		this->broadcast(command);
	}

	void broadcast(JsonObject& command) {
		size_t size = command.size();
		if (size) {
			String commandSTR = "";
			serializeJson(command, commandSTR);
			String edata = this->aesm->encrypt((byte*) commandSTR.c_str(), commandSTR.length());
			this->webSocketServer->broadcastTXT(edata);
			yield();
		}
	}

	void send(uint8_t socket, JsonObject& command) {
		size_t size = command.size();
		if (size) {
			String commandSTR = "";
			serializeJson(command, commandSTR);
			String edata = this->aesm->encrypt((byte*) commandSTR.c_str(), commandSTR.length());
			this->webSocketServer->sendTXT(socket, edata);
			yield();
		}
	}

	void LID() {
		uint32_t m = millis();
		if (this->lid > m) {
			this->bid = RANDOM_REG32;
			this->lid = m;
		}
	}

	void JLID(JsonObject& response) {
		response["b"] = this->bid;
		response["l"] = this->lid;
	}

	void session(JsonObject& response) {
		this->LID(); // don't forget to lid
		this->JLID(response);
	}

	void user(String& edata, JsonObject& response, JsonObject& broadcast) {
		this->LID(); // don't forget to lid
		String data = this->aesm->decrypt((byte*) edata.c_str());
		//DEBUG.println(data);
		DynamicJsonDocument requestDocument(1024);
		DeserializationError error = deserializeJson(requestDocument, data);
		if (!error) {
			JsonObject command = requestDocument.as<JsonObject>();
			JsonObject id = command["id"];
			uint32_t l = id["l"];
			uint32_t b = id["b"];
			JsonObject params = command["p"];
			if (this->bid == b && this->lid < l) {
				this->lid = l;
				this->oncommand(params, response, broadcast);
			} else {
				//DEBUG.println("this->bid == b && this->lid < l : " + String(l) + " " + String(b));
				response["c"] = params;
				this->LIDError(response);
			}
		} else {
			//DEBUG.println("ERROR: DeserializationError");
			this->LIDError(response);
		}

	}

	void sessionHTTP() {
		this->sendHeaders();
		DynamicJsonDocument responseDocument(256);
		JsonObject responseObject = responseDocument.to<JsonObject>();
		this->session(responseObject);
		String responseSTR = "";
		serializeJson(responseObject, responseSTR);
		this->httpServer->sendHeader(F("Content-Length"), String(responseSTR.length()));
		this->httpServer->send(200, F("text/plain"), responseSTR);
		yield();
	}

	void userHTTP() {
		this->sendHeaders();
		String encryptedData = this->httpServer->arg("d");

		DynamicJsonDocument responseDocument(1024);
		JsonObject response = responseDocument.to<JsonObject>();

		DynamicJsonDocument broadcastDocument(1024);
		JsonObject broadcast = broadcastDocument.to<JsonObject>();

		this->user(encryptedData, response, broadcast);

		this->httpResponse(response, broadcast);
		this->broadcast(broadcast);
	}

	void LIDError(JsonObject& response) {
		response["error"] = 1;
		JsonObject jlid = response.createNestedObject("jlid");
		this->JLID(jlid);
	}

	void fromJSON(JsonObject& params) {
		if (params.containsKey("key")) {
			String keySTR = params["key"];
			byte* base64 = (byte*) keySTR.c_str();
			unsigned int binary_length = Base64::decode_length(base64);
			if (32 == binary_length) {
				Base64::decode(base64, this->key);
			}
		}
	}

	virtual void JSON(JsonObject& login) {
		unsigned int b64Length = (4 * (32 + 2) / 3) + 2; // TODO:: what ?
		unsigned char base64[b64Length];
		for (int i = 0; i < b64Length; i++) base64[i] = '\0';
		unsigned int base64_length = Base64::encode(this->key, 32, base64);
		login["key"] = String((char*) base64);
	}

	void login(JsonObject& params, JsonObject& response, JsonObject& broadcast) {
		response["error"] = false;
		response["wport"] = this->settings.wport;
	}

	virtual void change(JsonObject& params, JsonObject& response, JsonObject& broadcast) {
		this->fromJSON(params);
		delete this->aesm;
		this->aesm = new AESM(this->key);
		this->saveFile();
		this->login(params, response, broadcast);
	}
};

#endif

