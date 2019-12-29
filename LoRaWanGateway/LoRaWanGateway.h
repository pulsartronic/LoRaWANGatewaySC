#ifndef __LoRaWanGateway__
#define __LoRaWanGateway__

#include <CDOS.h>

// This value of DEBUG determines whether some parts of code get compiled.
// Also this is the initial value of debug parameter. 
// The value can be changed using the admin webserver
// For operational use, set initial DEBUG vaulue 0
#define DEBUG 1

// Debug message will be put on Serial is this one is set.
// If set to 0, not USB Serial prints are done
// Set to 1 it will print all user level messages (with correct debug set)
// If set to 2 it will also print interrupt messages (not recommended)
#define DUSB 1

// Define the LoRa Frequncy band that is used. TTN Supported values are 915MHz, 868MHz and 433MHz.
// So supported values are: 433 868 915
#define _LFREQ 868

// The spreading factor is the most important parameter to set for a single channel
// gateway. It specifies the speed/datarate in which the gateway and node communicate.
// As the name says, in principle the single channel gateway listens to one channel/frequency
// and to one spreading factor only.
// This parameters contains the default value of SF, the actual version can be set with
// the webserver and it will be stored in SPIFF
// NOTE: The frequency is set in the loraModem.h file and is default 868.100000 MHz.
#define _SPREADING SF9


#define _LOCUDPPORT 1700					// UDP port of gateway! Often 1700 or 1701 is used for upstream comms
#define _MSG_INTERVAL 15					// Reset timer in seconds
//#define _TTNPORT 1700						// Standard port for TTN

//#define _DESCRIPTION "PT Gateway"			// Name of the gateway
//#define _EMAIL "admin@pulsartronic.com"		// Owner
//#define _PLATFORM "ESP8266"
//#define _LAT 41.432316
//#define _LON 2.178164
//#define _ALT 25								// Altitude

// ntp
// Please add daylight saving time to NTP_TIMEZONES when desired
// #define NTP_TIMESERVER "nl.pool.ntp.org"	// Country and region specific
// #define NTP_TIMEZONES	2 // How far is our Timezone from UTC (excl daylight saving/summer time)
#define SECS_IN_HOUR	3600
#define NTP_INTR 0 // Do NTP processing with interrupts or in loop();

// Defines whether the gateway will also report sensor/status value on MQTT
// after all, a gateway can be a node to the system as well
// Set its LoRa address and key below in this file
// See spec. para 4.3.2
#define GATEWAYNODE 0
#define _CHECK_MIC 0

// Define the correct radio type that you are using
#define CFG_sx1276_radio		
//#define CFG_sx1272_radio

// We can put the gateway in such a mode that it will (only) recognize
// nodes that are put in a list of trusted nodes 
// Values:
// 0: Do not use names for trusted Nodes
// 1: Use the nodes as a translation tabel for hex codes to names (in TLN)
// 2: Same as 1, but is nodes NOT in the nodes list below they are NOT
//		forwarded or counted! (not yet fully implemented)
#define TRUSTED_NODES 1
#define TRUSTED_DECODE 1


#if defined (ARDUINO_ARCH_ESP32) || defined(ESP32)
	#define ESP32_ARCH 1
#endif

#include <Arduino.h>
#include <Esp.h>								// ESP8266 specific IDE functions
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>
#include <sys/time.h>
#include <cstring>
#include <string> // C++ specific string functions
#include <FS.h> // ESP8266 Specific
#include <SPI.h> // For the RFM95 bus
#include <TimeLib.h> // http://playground.arduino.cc/code/time
#include <DNSServer.h> // Local DNSserver
#define ARDUINOJSON_USE_DOUBLE 1
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <pins_arduino.h>
#include <gBase64.h>							// https://github.com/adamvr/arduino-base64 (changed the name)

extern "C" {
	#include "lwip/err.h"
	#include "lwip/dns.h"
}

// ----------- Specific ESP32 stuff --------------
#if ESP32_ARCH==1 // IF ESP32
	#include "WiFi.h"
	#include <WiFiClient.h>
	#include <ESPmDNS.h>
	#include <SPIFFS.h>
// ----------- Generic ESP8266 stuff --------------
#else
	#include <ESP8266WiFi.h> // Which is specific for ESP8266
	#include <ESP8266mDNS.h>
	extern "C" {
		#include "user_interface.h"
		#include "c_types.h"
	}
#endif//ESP_ARCH

// ----------- Declaration of vars --------------
uint8_t debug = 1;								// Debug level! 0 is no msgs, 1 normal, 2 extensive
uint8_t pdebug = 0xFF;							// Allow all atterns (departments)

#include "utils.h"


#include "app.h"
#include "scripts.h"
#include "styles.h"

#include <HTTPUserServer.h>

#include "Login/Login.h"
#include "Login/LoginConfigurable.h"

#include <ClientWiFi.h>
#include <ClientWiFiConfigurable.h>

#include "LoRaModule/LoRaModuleB.h"
#include "LoRaWAN/LoRaWAN.h"

#include "LoRaModule/Modem/LoRaModem.h"
#include "LoRaModule/StateMachine/LoRaStateMachine.h"
#include "LoRaModule/LoRaModuleB_Methods.h"
#include "LoRaModule/LoRaModuleConfigurable.h"

#include "LoRaWAN/TXRX/TXRX.h"
#include "LoRaWAN/LoRaWAN_Methods.h"
#include "LoRaWAN/LoRaWANConfigurable.h"

#include "AccessPoint/AccessPoint.h"
#include "AccessPoint/AccessPointConfigurable.h"

#include "D1System/D1System.h"
#include "D1System/D1SystemConfigurable.h"


class LoRaWanGateway : public DeviceSet {
	public:
	HTTPUserServer* httpUserServer = NULL;

	Login* login = NULL;
	ClientWiFi* clientWiFi = NULL;
	LoRaWAN* loRaWAN = NULL;
	LoRaModuleB* loRaModule = NULL;
	AccessPoint* accessPoint = NULL;
	D1System* d1System = NULL;

	LoginConfigurable* loginConfigurable = NULL;
	ClientWiFiConfigurable* clientWiFiConfigurable = NULL;
	LoRaWANConfigurable* loRaWANConfigurable = NULL;
	LoRaModuleConfigurable* loRaModuleConfigurable = NULL;
	AccessPointConfigurable* accessPointConfigurable = NULL;
	D1SystemConfigurable* d1SystemConfigurable = NULL;

	Configurable* configurables[6];

	virtual ~LoRaWanGateway() {
		delete this->httpUserServer;

		delete this->login;
		delete this->clientWiFi;
		delete this->loRaWAN;
		delete this->loRaModule;
		delete this->accessPoint;
		delete this->d1System;
		delete this->loginConfigurable;
		delete this->clientWiFiConfigurable;
		delete this->loRaWANConfigurable;
		delete this->loRaModuleConfigurable;
		delete this->accessPointConfigurable;
		delete this->d1SystemConfigurable;
	}

	LoRaWanGateway() {
		Serial.println("");
		Serial.println(F("Starting LoRa WAN Gateway ..."));

		this->d1System = new D1System();
		this->accessPoint = new AccessPoint();
		this->httpUserServer = new HTTPUserServer();
		this->login = new Login(this, this->httpUserServer);

		this->clientWiFi = new ClientWiFi(this->login);
		this->loRaWAN = new LoRaWAN(this->login);
		this->loRaWAN->loRaModule = this->loRaModule = new LoRaModuleB(this->login);
		this->loRaModule->loRaWAN = this->loRaWAN;

		this->d1System->setup();
		this->accessPoint->setup();
		this->clientWiFi->setup();
		this->httpUserServer->setup(80);
		setup(this->loRaWAN);
		setup(this->loRaModule);

		this->configurables[0] = this->loginConfigurable = new LoginConfigurable(this->login);
		this->configurables[1] = this->clientWiFiConfigurable = new ClientWiFiConfigurable(this->clientWiFi);
		this->configurables[2] = this->loRaWANConfigurable = new LoRaWANConfigurable(this->loRaWAN);
		this->configurables[3] = this->loRaModuleConfigurable = new LoRaModuleConfigurable(this->loRaModule);				
		this->configurables[4] = this->accessPointConfigurable = new AccessPointConfigurable(this->accessPoint);
		this->configurables[5] = this->d1SystemConfigurable = new D1SystemConfigurable(this->d1System);

		this->login->setup();
	}

	unsigned int length() {
		return 6;//sizeof(this->configurables);
	}

	Configurable* configurable(unsigned int index) {
		Configurable* configurable = this->configurables[index];
		return configurable;
	}

	void loop() {
		this->httpUserServer->loop();

		this->login->loop();
		this->clientWiFi->loop();
		Loop(this->loRaModule);
		Loop(this->loRaWAN);
		this->accessPoint->loop();
		this->d1System->loop();
		this->loginConfigurable->loop();
		this->clientWiFiConfigurable->loop();
		this->loRaWANConfigurable->loop();
		this->loRaModuleConfigurable->loop();
		this->accessPointConfigurable->loop();
		this->d1SystemConfigurable->loop();
	}
};

#endif
