#include <NetworkNode.h>

NetworkNode::NetworkNode(Node* parent, const char* name) : Node(parent, name) {
	Method* login = new Method(std::bind(&NetworkNode::login, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	this->methods->set("login", login);

	Method* change = new Method(std::bind(&NetworkNode::change, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	this->methods->set("change", change);

	Method* ping = new Method(std::bind(&NetworkNode::ping, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	this->methods->set("ping", ping);
}

NetworkNode::~NetworkNode() {
	delete this->webSocketServer;
	delete this->httpServer;

	delete this->aesm;
	delete[] this->key;
}

void NetworkNode::setup() {
	this->bid = RANDOM_REG32;
	this->lid = millis();

	this->readFile();
	this->applySettings();
}

void NetworkNode::applySettings() {
	delete this->webSocketServer;
	this->webSocketServer = new WebSocketsServer(this->settings.wport);
	std::function<void(uint8_t, WStype_t, uint8_t*, size_t)> onEvent =
		std::bind(&NetworkNode::onEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	this->webSocketServer->onEvent(onEvent);
	this->webSocketServer->begin();

	this->httpServer = new ESP8266WebServer(this->settings.hport);
	this->httpServer->on(F("/"), std::bind(&NetworkNode::html, this));
	this->httpServer->onNotFound (std::bind(&NetworkNode::handleNotFound, this));
	// Android Captive Portal
	this->httpServer->on(String(F("/generate_204")), std::bind(&NetworkNode::html, this));
	// Windows Captive Portal ????
	this->httpServer->on(String(F("/fwlink")), std::bind(&NetworkNode::html, this));
	// iOS Captive Portal ????
	this->httpServer->on(String(F("/hotspot-detect.html")), std::bind(&NetworkNode::html, this));

	this->httpServer->on(F("/u"), std::bind(&NetworkNode::userHTTP, this));
	this->httpServer->on(F("/s"), std::bind(&NetworkNode::sessionHTTP, this));

	this->httpServer->begin(); // Web server start

	delete this->aesm;
	this->aesm = new AESM((byte*) this->key);
}

void NetworkNode::loop() {
	this->httpServer->handleClient();
	this->webSocketServer->loop();
/*
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
			yield();
		}
	}
*/
}

void NetworkNode::html() {
	this->httpServer->send_P(200, "text/html", "");
}

void NetworkNode::handleNotFound() {
	String message = F("File Not Found\n\n");
	this->httpServer->sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
	this->httpServer->sendHeader(F("Pragma"), F("no-cache"));
	this->httpServer->sendHeader(F("Expires"), F("-1"));
	this->httpServer->sendHeader(F("Content-Length"), String(message.length()));
	this->httpServer->send(404, F("text/plain"), message );
}

void NetworkNode::onEvent(uint8_t socket, WStype_t type, uint8_t* payload, size_t length) {
	// TODO:: handle client messages
	switch(type) {
		case WStype_DISCONNECTED: {
			// Serial.printf("[%u] Disconnected!\n", num);
		} break;
		case WStype_CONNECTED: {
			// ...
		} break;
		case WStype_TEXT: {
			if (0 < length) {
				String edata = String((char*)payload);
				DynamicJsonDocument responseDocument(1024);
				JsonObject response = responseDocument.to<JsonObject>();
				DynamicJsonDocument broadcastDocument(1024);
				JsonObject broadcast = broadcastDocument.to<JsonObject>();
				this->user(edata, response, broadcast);
				this->send(socket, response);
				this->broadcast(broadcast);
			}
		} break;
	}
}

void NetworkNode::sendHeaders() {
	this->httpServer->sendHeader(F("Access-Control-Allow-Origin"), F("*"));
	this->httpServer->sendHeader(F("Access-Control-Allow-Headers"), F("Origin, X-Requested-With, Content-Type, Accept"));
}

void NetworkNode::httpResponse(JsonObject& response, JsonObject& broadcast) {
	String plain = "[";

	{
		String responseSTR = "";
		serializeJson(response, responseSTR);
		plain += responseSTR;
	}
	
	{
		String broadcastSTR = "";
		serializeJson(broadcast, broadcastSTR);
		plain += "," + broadcastSTR + "]";
	}
	
	String edata = this->aesm->encrypt((byte*) plain.c_str(), plain.length());
	this->httpServer->sendHeader(F("Content-Length"), String(edata.length()));
	this->httpServer->send(200, F("text/plain"), edata);
	yield();
}




