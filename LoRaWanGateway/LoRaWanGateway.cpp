#include "LoRaWanGateway.h"

LoRaWanGateway::LoRaWanGateway() : Node(NULL, "root") {
	if (SPIFFS.begin()) {
		DEBUG.println("SPIFFS init success");
	} else {
		DEBUG.println("SPIFFS init ERROR !!");
	}

	this->httpServer = new HTTPServer();
	this->webSocketServer = new WebSocketsServer(3498);
	std::function<void(uint8_t, WStype_t, uint8_t*, size_t)> onEvent = std::bind(&LoRaWanGateway::onEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	this->webSocketServer->onEvent(onEvent);

	this->login = new Login(this, "login");
	this->nodes->set(this->login->name, this->login);

	this->system = new System(this, "system");
	this->nodes->set(this->system->name, this->system);

	this->wifi = new WIFI(this, "wifi");
	this->nodes->set(this->wifi->name, this->wifi);

	this->rfm = new RFM(this, "rfm");
	this->nodes->set(this->rfm->name, this->rfm);

	this->wan = new WAN(this, "wan");
	this->nodes->set(this->wan->name, this->wan);
	this->wan->rfm = this->rfm;
}

LoRaWanGateway::~LoRaWanGateway() {
	delete this->httpServer;
	delete this->login;
	delete this->system;
	delete this->wifi;
	delete this->rfm;
	delete this->wan;
}

void LoRaWanGateway::setup() {
	this->httpServer->setup(80);
	this->httpServer->server->on(F("/u"), std::bind(&LoRaWanGateway::user, this));
	this->httpServer->server->on(F("/s"), std::bind(&LoRaWanGateway::session, this));

	this->webSocketServer->begin();

	this->login->setup();
	this->system->setup();
	this->wifi->setup();
	this->rfm->setup();
	this->wan->setup();

	DEBUG.println("Starting LoRaWAN Gateway ... OK");
}

void LoRaWanGateway::loop() {
	this->login->loop();
	this->system->loop();
	this->wifi->loop();
	this->rfm->loop();
	this->wan->loop();
	this->httpServer->loop();
	this->webSocketServer->loop();

	uint64_t now = clock64.mstime();
	uint32_t diff = (uint32_t)(now - this->lping);
	if (diff > this->iping) {
		this->lping = now;
		int connectedClients = this->webSocketServer->connectedClients();
		if (connectedClients) {
			DynamicJsonDocument pongDocument(512);
			JsonObject pongObject = pongDocument.to<JsonObject>();
			this->ping(pongObject);
			this->command(pongObject);
		}
	} 
}

void LoRaWanGateway::session() {
	this->sendHeaders();
	DynamicJsonDocument responseDocument(256);
	JsonObject responseObject = responseDocument.to<JsonObject>();
	this->login->session(responseObject);
	this->httpResponse(responseObject);
}

void LoRaWanGateway::user() {
	this->sendHeaders();
	String edata = this->httpServer->server->arg("d");
	DynamicJsonDocument responseDocument(1024);
	JsonObject responseObject = responseDocument.to<JsonObject>();
	DynamicJsonDocument broadcastDocument(1024);
	JsonObject broadcast = broadcastDocument.to<JsonObject>();

	this->login->user(edata, responseObject, broadcast);
	this->httpResponse(responseObject);
	this->command(broadcast);
}

void LoRaWanGateway::onEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
	// TODO:: handle client messages
	switch(type) {
		case WStype_DISCONNECTED: {
			// Serial.printf("[%u] Disconnected!\n", num);
		} break;
		case WStype_CONNECTED: {
			// IPAddress ip = this->webSocketServer->remoteIP(num);
			for (uint16_t i = 0; i < this->nodes->length; i++) {
				KeyValue<Node>* keyValue = this->nodes->keyValues[i];
				DynamicJsonDocument rootDocument(256);
				JsonObject command = rootDocument.to<JsonObject>();
				JsonObject node = command.createNestedObject(keyValue->key);
				node.createNestedObject("state");

				DynamicJsonDocument responseDocument(1024);
				JsonObject response = responseDocument.to<JsonObject>();

				DynamicJsonDocument broadcastDocument(1024);
				JsonObject broadcast = broadcastDocument.to<JsonObject>();

				this->oncommand(command, response, broadcast);
				this->command(response);
			}
		} break;
		case WStype_TEXT: {
			if (0 < length) {
				// Serial.printf("[%u] get Text: %s\n", num, payload);
				// this->command(num, payload, length);
			}
		} break;
	}
}

void LoRaWanGateway::sendHeaders() {
	this->httpServer->server->sendHeader(F("Access-Control-Allow-Origin"), F("*"));
	this->httpServer->server->sendHeader(F("Access-Control-Allow-Headers"), F("Origin, X-Requested-With, Content-Type, Accept"));
}

void LoRaWanGateway::httpResponse(JsonObject& responseObject) {
	String response = "";
	serializeJson(responseObject, response);
	this->httpServer->server->sendHeader(F("Content-Length"), String(response.length()));
	this->httpServer->server->send(200, F("text/plain"), response);
	yield();
}

JsonObject LoRaWanGateway::rootIT(JsonObject& root) {
	return root;
}

// <<--
void LoRaWanGateway::command(JsonObject& command) {
	String commandSTR = "";
	serializeJson(command, commandSTR);
	// DEBUG.println("::>> " + commandSTR);
	DynamicJsonDocument responseDocument(1024);
	JsonObject response = responseDocument.to<JsonObject>();
	String edata = this->login->aesm->encrypt((byte*) commandSTR.c_str(), commandSTR.length());
	response["data"] = edata;
	String responseSTR = "";
	serializeJson(response, responseSTR);
	this->webSocketServer->broadcastTXT(responseSTR);
	yield();
}



