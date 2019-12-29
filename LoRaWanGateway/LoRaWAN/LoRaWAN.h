#ifndef __LoRaWAN__
#define __LoRaWAN__

#include <Logger.h>

class LoRaWAN {
	public:
	unsigned int phase = 0;

	Logger* logger = NULL;
	LoRaModuleB* loRaModule = NULL;

	IPAddress ttnServer; // IP address of NTP_TIMESERVER
	IPAddress ntpServer; // IP address of NTP_TIMESERVER
	WiFiUDP Udp;

	bool UDPConnected = false;

	// The time in seconds since 1970 that the server started
	// be aware that UTP time has to succeed for meaningful values.
	// We use this variable since millis() is reset every 50 days...
	time_t startTime = (time_t) 0;
	time_t lastACK = (time_t) 0;
	uint32_t ntptimer = 0ul;

	// this is for phase_1
	uint32_t iphase = 2ul * 1000ul; // 2 seconds
	uint64_t lphase = 0ull;
	
	String filename = "wan.json";

	uint8_t gid[8] = {0};
	String id = "";
	String server = "router.eu.thethings.network";
	uint16_t port = 1700u; // 1700 Standard port for TTN
	String desc = "no description"; // Name of the gateway, used for free form description 
	String mail = ""; // Owner, used for contact email
	String platform = "ESP8266"; // platform definition
	double lon = 0.0;
	double lat = 0.0;
	double alt = 25.0; // Altitude
	uint16_t istat = 120u; // Send a 'stat' message to server
	uint16_t ipull = 55u; // PULL_DATA messages to server to get downstream in milliseconds
	String ntp = "nl.pool.ntp.org";
	uint16_t tzones = 0u;
	uint16_t intp = 3600u; // How often do we want time NTP synchronization

	LoRaWAN(Logger* logger) {
		this->logger = logger;
	}
};


#endif
