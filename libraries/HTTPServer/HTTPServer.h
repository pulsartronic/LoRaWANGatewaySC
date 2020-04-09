#ifndef __HTTPServer__
#define __HTTPServer__

#include <ESP8266WebServer.h>
#define ARDUINOJSON_USE_DOUBLE 1
#include <ArduinoJson.h>
#include <AESM.h>

#include <app.h>
#include <scripts.h>
#include <styles.h>

class HTTPServer {
	public:
	ESP8266WebServer* server = NULL;

	HTTPServer();
	virtual ~HTTPServer();
	void setup(int port);
	void loop();
	void handleRoot();
	void scripts();
	void styles();
	void handleNotFound();
};

#endif
