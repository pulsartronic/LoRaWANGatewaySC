#include "LoRaWANGateway.h"

/**
 * Contructor. Here's the program entry point
 */
LoRaWANGateway::LoRaWANGateway() : NetworkNode(NULL, "root") {
	Serial.begin(9600);

	// ////////////////////////////////////////////////////////////////////////////////
	// DEFAULT CONFIGURATION //////////////////////////////////////////////////////////
	// Change these values as needed, most of them can be modified from the web interface
	
	// TODO:: make checks against this
	bool filesystemInitilized = LittleFS.begin();

	this->system = new System(this, "system");
	this->nodes->set(this->system->name, this->system);

	this->wifi = new WIFI(this, "wifi");
	this->nodes->set(this->wifi->name, this->wifi);

	this->rfm = new RFM(this, "rfm");
	this->nodes->set(this->rfm->name, this->rfm);

	this->wan = new WAN(this, "wan");
	this->nodes->set(this->wan->name, this->wan);
	this->wan->rfm = this->rfm;
	
	// ////////////////////////////////////////////////////////////////
	// Http and WebSockets ports this app is going to listen
	// these values cannot be changed from the configuration interface
	this->settings.hport = 80;
	this->settings.wport = 3498;

	// Default credentials for the WiFi Access Point
	this->wifi->ap->settings.ssid = "LoRa TTN Gateway " + String(ESP.getChipId());
	this->wifi->ap->settings.pass = "12345678";

	// ////////////////////////////////////////////////////////////////
	// WiFi Client, Maybe you need to set it here
	// this->wifi->client->settings.ssid = "";
	// this->wifi->client->settings.password = "";
	
	// ////////////////////////////////////////////////////////////////
	// NTP, Network Time Protocol
	this->system->ntp->settings.host = "pool.ntp.org";
	this->system->ntp->settings.tz = 341; // TZ_Europe_London

	// ////////////////////////////////////////////////////////////////
	// Default password for the configuration interface
	// SHA256("adminadmin") change "adminadmin" by "user+pass"
	this->key = new byte[32] {
		0xd8, 0x24, 0x94, 0xf0, 0x5d, 0x69, 0x17, 0xba, 
		0x02, 0xf7, 0xaa, 0xa2, 0x96, 0x89, 0xcc, 0xb4,
		0x44, 0xbb, 0x73, 0xf2, 0x03, 0x80, 0x87, 0x6c,
		0xb0, 0x5d, 0x1f, 0x37, 0x53, 0x7b, 0x78, 0x92
	};
	
	// ////////////////////////////////////////////////////////////////
	// TTN configuration
	this->wan->settings.host     = "router.eu.thethings.network";
	this->wan->settings.port     = 1700u; // 1700 Standard port for TTN
	this->wan->settings.desc     = "no description"; // Name of the gateway, used for free form description 
	this->wan->settings.mail     = ""; // Owner, used for contact email
	this->wan->settings.platform = "ESP8266"; // platform definition, could be anything
	this->wan->settings.lon      = 0.0;
	this->wan->settings.lat      = 0.0;
	this->wan->settings.alt      = 25.0f; // Altitude
	
	// ////////////////////////////////////////////////////////////////
	// RFM configuration
	this->rfm->settings.freq.curr = 868300000l; // current frequency in Hz
	this->rfm->settings.freq.min  = 858000000l; // min frequency in Hz
	this->rfm->settings.freq.max  = 878000000l; // max frequency in Hz;
	this->rfm->settings.txpw      = 17;    // tx power
	this->rfm->settings.sfac      = 7;     // spreading factor
	this->rfm->settings.sbw       = 125E3; // SignalBandwidth
	this->rfm->settings.crat      = 5;     // coding rate
	this->rfm->settings.plength   = 8l;    // preamble length
	this->rfm->settings.sw        = 0x39;  // syncword default:0x39
	this->rfm->settings.cad       = 0;
	this->rfm->settings.crc       = 1;
	this->rfm->settings.iiq       = 0;  // InvertIQ
	// RFM Pins
	this->rfm->pins.miso          = 12; // GPIO12
	this->rfm->pins.mosi          = 13; // GPIO13
	this->rfm->pins.sck           = 14; // GPIO14
	this->rfm->pins.nss           = 16; // GPIO16
	this->rfm->pins.rst           = 15; // GPIO15
	this->rfm->pins.dio[0]        = 4;  // GPIO4
}

LoRaWANGateway::~LoRaWANGateway() {
	delete this->system;
	delete this->wifi;
	delete this->wan;
	delete this->rfm;
}

void LoRaWANGateway::setup() {
	NetworkNode::setup();
	this->system->setup();
	this->wifi->setup();
	this->wan->setup();
	this->rfm->setup();
}

void LoRaWANGateway::loop() {
	NetworkNode::loop();
	this->system->loop();
	this->wifi->loop();
	this->wan->loop();
	this->rfm->loop();
}

void LoRaWANGateway::html() {
	this->httpServer->send_P(200, "text/html", HTML);
}


