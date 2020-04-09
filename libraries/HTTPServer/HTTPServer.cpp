#include <HTTPServer.h>

HTTPServer::HTTPServer() {

}

HTTPServer::~HTTPServer() {

}

void HTTPServer::setup(int port) {
	//Serial.print(F("Starting HTTP server on port "));
	//Serial.println(String(port));
	this->server = new ESP8266WebServer(port);
	this->server->on(F("/"), std::bind(&HTTPServer::handleRoot, this));
	
	this->server->on(F("/scripts.js"), std::bind(&HTTPServer::scripts, this));		
	this->server->on(F("/styles.css"), std::bind(&HTTPServer::styles, this));
	
	this->server->onNotFound (std::bind(&HTTPServer::handleNotFound, this));
	// Android Captive Portal
	this->server->on(String(F("/generate_204")), std::bind(&HTTPServer::handleRoot, this));
	// Windows Captive Portal
	this->server->on(String(F("/fwlink")), std::bind(&HTTPServer::handleRoot, this));
	// iOS Captive Portal ????
	this->server->on(String(F("/hotspot-detect.html")), std::bind(&HTTPServer::handleRoot, this));
	this->server->begin(); // Web server start
}

void HTTPServer::loop() {
	this->server->handleClient();
}

void HTTPServer::handleRoot() {
	this->server->send_P(200, "text/html", App);
}

void HTTPServer::scripts() {
	this->server->send_P(200, "application/javascript", Scripts);
}

void HTTPServer::styles() {
	this->server->send_P(200, "text/css", Styles);
}

void HTTPServer::handleNotFound() {
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
