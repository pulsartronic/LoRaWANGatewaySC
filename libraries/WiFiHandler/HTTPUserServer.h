#ifndef __HTTPUserServer__
#define __HTTPUserServer__

#include <ESP8266WebServer.h> 
#include <ArduinoJson.h>
#include <AESM.h>

class HTTPUserServer {
	public:
	ESP8266WebServer* server = NULL;

	virtual ~HTTPUserServer() {

	}

	HTTPUserServer() {

	}

	void setup(int port) {
		Serial.print(F("Starting HTTP server on port "));
		Serial.println(String(port));
		this->server = new ESP8266WebServer(port);
		this->server->on(F("/"), std::bind(&HTTPUserServer::handleRoot, this));
		
		this->server->on(F("/scripts.js"), std::bind(&HTTPUserServer::scripts, this));		
		this->server->on(F("/styles.css"), std::bind(&HTTPUserServer::styles, this));
		
		this->server->onNotFound (std::bind(&HTTPUserServer::handleNotFound, this));
		// Android Captive Portal
		this->server->on(String(F("/generate_204")), std::bind(&HTTPUserServer::handleRoot, this));
		// Windows Captive Portal
		this->server->on(String(F("/fwlink")), std::bind(&HTTPUserServer::handleRoot, this));
		// iOS Captive Portal ????
		this->server->on(String(F("/hotspot-detect.html")), std::bind(&HTTPUserServer::handleRoot, this));
		this->server->begin(); // Web server start
	}

	void loop() {
		this->server->handleClient();
	}

	void handleRoot() {
		this->server->send_P(200, "text/html", App);
	}

	void scripts() {
		this->server->send_P(200, "application/javascript", Scripts);
	}

	void styles() {
		this->server->send_P(200, "text/css", Styles);
	}

	void handleNotFound() {
		String message = F("File Not Found\n\n");
		//message += "URI: ";
		//message += server->uri();
		//message += "\nMethod: ";
		//message += ( server->method() == HTTP_GET ) ? "GET" : "POST";
		//message += "\nArguments: ";
		//message += server->args();
		//message += "\n";

		//for ( uint8_t i = 0; i < server->args(); i++ ) {
		//	message += " " + server->argName ( i ) + ": " + server->arg ( i ) + "\n";
		//}

		server->sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
		server->sendHeader(F("Pragma"), F("no-cache"));
		server->sendHeader(F("Expires"), F("-1"));
		server->sendHeader(F("Content-Length"), String(message.length()));
		server->send(404, F("text/plain"), message );

		//Serial.println("Not found");
		//Serial.println(message);
	}
};

#endif
