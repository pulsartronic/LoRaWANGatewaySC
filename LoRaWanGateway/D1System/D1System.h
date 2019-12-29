#ifndef __D1System__
#define __D1System__

#include <ESP8266httpUpdate.h>

#define VERSION 1
#define MHZ 80
#define SPIFS 1


class D1System {
	public:
	

	virtual ~D1System() {
		
	}

	D1System() {

	}

	void setup() {
		if (SPIFFS.begin()) {
			Serial.println(F("SPIFFS init success"));
		} else {
			Serial.println(F("SPIFFS init ERROR !!"));
		}
	}

	void loop() {

	}

	String upgrade() {
		String response = "{}";

		Serial.println("UPGRADING ::::::");
		String host = F("http://pulsartronic.com/firmware/lorawan/version.php");
		WiFiClient client;
		HTTPClient http;
		if (http.begin(client, host)) {
			int httpCode = http.GET();
			if (httpCode > 0) {
				if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
					String payload = http.getString();
					int last = payload.toInt();
					Serial.println("payload: " + payload);
					if (last > VERSION) {
						Serial.println("Last is greather than actual, UPGRADING !!!");
						WiFiClient client;
						ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
						ESPhttpUpdate.rebootOnUpdate(true);
						t_httpUpdate_return ret = ESPhttpUpdate.update(client, "http://pulsartronic.com/firmware/lorawan/d1_80mhz_4mb_1mbspifss_latest.bin");
						switch (ret) {
							case HTTP_UPDATE_FAILED:
								response = "{\"error\":9}";
								Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
							break;
							case HTTP_UPDATE_NO_UPDATES:
								response = "{\"error\":10}";
								Serial.println("HTTP_UPDATE_NO_UPDATES");
							break;
							case HTTP_UPDATE_OK:
								response = "{\"error\":false}";
								Serial.println("HTTP_UPDATE_OK");
							break;
							default:
								response = "{\"error\":15}";
							break;
						}
					} else {
						response = "{\"error\":14}";
					}
				} else {
					response = "{\"error\":16}";
				}
			} else {
				response = "{\"error\":13}";
				Serial.println(String(httpCode));
				Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
			}
			http.end();
		} else {
			response = "{\"error\":12}";
			Serial.printf("[HTTP} Unable to connect\n");
		}

		return response;
	}
};

#endif
