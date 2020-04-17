#include <NetworkNode.h>

NetworkNode::NetworkNode(Node* parent, const char* name) : Node(parent, name) {
	this->login = new Login(this, "login");
	this->nodes->set(this->login->name, this->login);
}

NetworkNode::~NetworkNode() {
	delete this->login;
	delete this->webSocketServer;
	delete this->httpServer;
}

void NetworkNode::setup() {
	// TODO:: readFile

	this->httpServer = new HTTPServer();
	this->webSocketServer = new WebSocketsServer(this->settings.wsPort);
	std::function<void(uint8_t, WStype_t, uint8_t*, size_t)> onEvent = std::bind(&NetworkNode::onEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	this->webSocketServer->onEvent(onEvent);

	this->httpServer->setup(this->settings.httpPort);
	this->httpServer->server->on(F("/u"), std::bind(&NetworkNode::user, this));
	this->httpServer->server->on(F("/s"), std::bind(&NetworkNode::session, this));

	this->webSocketServer->begin();

	this->login->setup();
}

void NetworkNode::loop() {
	this->login->loop();
	this->httpServer->loop();
	this->webSocketServer->loop();
}

void NetworkNode::session() {
	this->sendHeaders();
	DynamicJsonDocument responseDocument(256);
	JsonObject responseObject = responseDocument.to<JsonObject>();
	this->login->session(responseObject);
	this->httpResponse(responseObject);
}

void NetworkNode::user() {
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

void NetworkNode::onEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
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

void NetworkNode::sendHeaders() {
	this->httpServer->server->sendHeader(F("Access-Control-Allow-Origin"), F("*"));
	this->httpServer->server->sendHeader(F("Access-Control-Allow-Headers"), F("Origin, X-Requested-With, Content-Type, Accept"));
}

void NetworkNode::httpResponse(JsonObject& responseObject) {
	String response = "";
	serializeJson(responseObject, response);
	this->httpServer->server->sendHeader(F("Content-Length"), String(response.length()));
	this->httpServer->server->send(200, F("text/plain"), response);
	yield();
}




