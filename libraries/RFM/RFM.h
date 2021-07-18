#define ARDUINOJSON_USE_DOUBLE 1
#include <ArduinoJson.h>
#include <Node.h>
#include <SPI.h>
#include <LoRa.h>
#include <System.h>

#ifndef __RFM__
#define __RFM__

class RFM : public Node {
	public:

	class Handler {
		public:
		virtual void onRFMPacket(Data::Packet* packet) = 0;
	};

	class Pins {
		public:
		int miso = 12;
		int mosi = 13;
		int sck = 14;
		int nss = 16;
		int rst = 15;
		int dio[6] = {4, -1, -1, -1, -1, -1};
	};

	// DEFAULT RFM CONFIGURATION, YOU SHOULD CHANGE IT BASED IN YOUR HARDWARE
	class Settings {
		public:

		class Frequency {
			public:
			long curr = 868300000l; // current frequency in Hz
			long min  = 858000000l; // min frequency in Hz
			long max  = 878000000l; // max frequency in Hz
		};

		Frequency freq;
		int txpw     = 17;    // tx power
		int sfac     = 7;     // spreading factor
		long sbw     = 125E3; // SignalBandwidth
		int crat     = 5;     // coding rate
		long plength = 8l;    // preamble length
		int sw       = 0x39;  // syncword default:0x39
		int cad      = 0;
		int crc      = 1;
		int iiq      = 0;     // InvertIQ
	};

	RFM::Settings settings;
	RFM::Pins pins;
	bool active = false;

	virtual ~RFM();
	RFM(Node* parent, const char* name);
	void setup();
	void loop();
	void apply(RFM::Settings* settings);
	int send(Data::Packet* packet);
	void read(RFM::Handler* handler);

	virtual void applySettings();
	virtual void state(JsonObject& params, JsonObject& response, JsonObject& broadcast);
	virtual void fromJSON(JsonObject& params);
	virtual void JSON(JsonObject& params);
};

#endif
