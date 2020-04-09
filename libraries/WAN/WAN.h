/**
 * This LoRa WAN protocol implementation follows the following specification:
 * https://github.com/Lora-net/packet_forwarder/blob/master/PROTOCOL.TXT
 *
 */

#define ARDUINOJSON_USE_DOUBLE 1

#include <SystemClock.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <System.h>
#include <RFM.h>
#include <ArduinoJson.h>
#include <Base64.h>
#include <Node.h>

#ifndef __WAN__
#define __WAN__

// Every packet sent to TTN has a header of 12 bytes
#define HEADER_LENGTH 12

#define PROTOCOL_VERSION 0x02
#define PUSH_DATA 0x00
#define PUSH_ACK  0x01
#define PULL_DATA 0x02
#define PULL_RESP 0x03
#define PULL_ACK  0x04
#define TX_ACK    0x05

class WAN : public RFM::Handler, public Node {
	public:

	class Settings {
		public:
		uint8_t id[8] = {0};
		String host = "router.eu.thethings.network";//"192.168.1.134";
		uint16_t port = 1700u; // 1700 Standard port for TTN
		String desc = "no description"; // Name of the gateway, used for free form description 
		String mail = ""; // Owner, used for contact email
		String platform = "ESP8266"; // platform definition
		double lon = 0.0;
		double lat = 0.0;
		float alt = 25.0f; // Altitude
	};

	class Statistics {
		public:
		uint32_t rxnb = 0ul; // Number of radio packets received
		uint32_t rxok = 0ul; // Number of radio packets received with a valid PHY CRC
		uint32_t rxfw = 0ul; // Number of radio packets forwarded
		float ackr    = 0ul; // Percentage of upstream datagrams that were acknowledged
		uint32_t dwnb = 0ul; // Number of downlink datagrams received
		uint32_t txnb = 0ul; // Number of packets emitted	
	};

	class RFData {
		public:
		Data::Packet* packet = NULL;
		RFM::Settings settings;
		int rssi = 0;
		float snr = 0.0;
	};

	class Message {
		public:

		class Up {
			public:
			uint8_t header[HEADER_LENGTH] = {0};
			DynamicJsonDocument* json = NULL;
			Up(WAN* wan, uint16_t jsonLength);
			virtual ~Up();
		};

		class Stat : public Up {
			public:
			Stat(WAN* wan);
		};

		class RxPk : public Up {
			public:
			RxPk(WAN* wan);
			void add(WAN::RFData* data);
		};

		class Pull : public Up {
			public:
			Pull(WAN* wan);
		};

		class TxAck : public Up {
			public:
			TxAck(WAN* wan, String error);
		};
	};


	WiFiUDP* udp = NULL;
	RFM* rfm = NULL;
	Statistics statistics;
	Settings settings;

	uint32_t istat = 180ul * 1000ul; // stat message interval in milliseconds
	uint64_t lstat = 0ull;

	uint32_t ipull = 57ul * 1000ul; // pull message interval in milliseconds
	uint64_t lpull = 0ull;

	uint64_t lastACK = 0ull;

	WAN(Node* parent, const char* name);
	virtual ~WAN();
	void setup();
	void loop();
	void read();
	void stat();
	void pull();
	void send(WAN::Message::Up* up);
	virtual void onRFMPacket(Data::Packet* packet);
	void resp(uint8_t* buffer, uint16_t size);

	virtual void getState(JsonObject& state);
	virtual void getPing(JsonObject& response);
	virtual void fromJSON(JsonObject& params);
	virtual void JSON(JsonObject& params);
	virtual void save(JsonObject& params, JsonObject& response, JsonObject& broadcast);
};

#endif
