#include <Node.h>
#include <HTTPServer.h>
#include <WebSocketsServer.h>
#include <Login.h>

#ifndef __NetworkNode__
#define __NetworkNode__

class NetworkNode : public Node {
	public:

	class Settings {
		public:
		int httpPort = 80;
		int wsPort = 3498;
	};

	HTTPServer* httpServer = NULL;
	WebSocketsServer* webSocketServer = NULL;
	Login* login = NULL;
	Settings settings;

	NetworkNode(Node* parent, const char* name);
	virtual ~NetworkNode();

	virtual void setup();
	virtual void loop();

	void session();
	void user();
	void onEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length);

	void sendHeaders(); // -->>
	void httpResponse(JsonObject& responseObject); // -->>

	virtual JsonObject rootIT(JsonObject& root) {
		return root;
	}

	virtual void command(JsonObject& command) {
		String commandSTR = "";
		serializeJson(command, commandSTR);
		DynamicJsonDocument responseDocument(1024);
		JsonObject response = responseDocument.to<JsonObject>();
		String edata = this->login->aesm->encrypt((byte*) commandSTR.c_str(), commandSTR.length());
		response["data"] = edata;
		String responseSTR = "";
		serializeJson(response, responseSTR);
		this->webSocketServer->broadcastTXT(responseSTR);
		yield();
	}
};

#endif

