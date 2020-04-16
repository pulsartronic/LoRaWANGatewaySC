#include <WAN.h>

WAN::WAN(Node* parent, const char* name) : Node(parent, name) {
	this->udp = new WiFiUDP();
	this->schedules = new DS::List<Scheduled*>();
}

WAN::~WAN() {
	delete this->udp;
}

void WAN::setup() {
	// Default ID : MAC address
	uint8_t MAC_array[6] = {0};
	WiFi.macAddress(MAC_array);		
	this->settings.id[0] = MAC_array[0];
	this->settings.id[1] = MAC_array[1];
	this->settings.id[2] = MAC_array[2];
	this->settings.id[3] = 0xFF;
	this->settings.id[4] = 0xFF;
	this->settings.id[5] = MAC_array[3];
	this->settings.id[6] = MAC_array[4];
	this->settings.id[7] = MAC_array[5];
/*
	this->settings.id[0] = 0x84;
	this->settings.id[1] = 0x0d;
	this->settings.id[2] = 0x8e;
	this->settings.id[3] = 0xFF;
	this->settings.id[4] = 0xFF;
	this->settings.id[5] = 0xab;
	this->settings.id[6] = 0xac;
	this->settings.id[7] = 0x40;
*/
	this->readFile();
	// TODO:: use bound somewhere ... should we check if it is already bound ???
	uint8_t bound = this->udp->begin(this->settings.port);

	DEBUG.println("Starting WAN system ... OK");
}

void WAN::loop() {
	bool connected = (WL_CONNECTED == WiFi.status());
	if (connected) {
		uint64_t now = clock64.mstime();

		uint32_t diff = (uint32_t) (now - this->lstat);
		if (this->istat <= diff) {
			this->lstat = now;
			this->stat();
		}

		diff = (uint32_t) (now - this->lpull);
		if (this->ipull <= diff) {
			this->lpull = now;
			this->pull();
		}

		this->read();
	}

	this->rfm->read(this);

	this->emitDownlinks();
}

void WAN::emitDownlinks() {
	DS::List<uint32_t>* indices = new DS::List<uint32_t>();

	for (uint32_t i = 0ul; i < this->schedules->length; i++) {
		Scheduled* scheduled = this->schedules->get(i);
		uint32_t now = micros();
		if (now <= scheduled->tmst) {
			indices->add(i);
			this->rfm->apply(&scheduled->rfData->settings);
			this->rfm->send(scheduled->rfData->packet);
			this->rfm->apply(&this->rfm->settings);
			this->statistics.txnb += 1u;
		}
	}

	for (uint32_t i = 0ul; i < indices->length; i++) {
		uint32_t index = indices->get(i);
		Scheduled* scheduled = this->schedules->get(index);
		this->schedules->removeAt(index);
		delete scheduled;
	}

	if (0ul < indices->length) {
		String scheduleLog = "Sent " + String(indices->length) + " DOWNLINKS, left: " + String(this->schedules->length);
		this->log(scheduleLog);
	}

	delete indices;
}

void WAN::read() {
	int size = this->udp->parsePacket();
	yield();
	if (4 <= size) { // 4 bytes: minimum packet size
		// Remote Address should be known
		// IPAddress remoteIpNo = this->udp->remoteIP();
		// uint16_t remotePortNo = this->udp->remotePort();
		uint8_t buffer[size + 1]; buffer[size] = '\0'; // add triling 0 to later treat it as a char array
		int readSize = this->udp->read(buffer, size);
		yield();

		if (size == readSize) {
			uint8_t protocol = buffer[0];
			uint16_t token = buffer[2] * 256 + buffer[1];
			uint8_t identifier = buffer[3];
			switch (identifier) {
				case PUSH_ACK: {
					//Serial.println("PUSH_ACK");
					this->lastACK = clock64.mstime();
					// TODO:: wan->statistics->ackr
				} break;
				case PULL_ACK: {
					//Serial.println("PULL_ACK");
					this->lastACK = clock64.mstime();
				} break;
				case PULL_RESP: {
					//Serial.println("PULL_RESP");
					this->resp(buffer, size);
					this->lastACK = clock64.mstime();
				} break;
				default: {
					String pullAckLog = "ERROR readUdp :: identifier not recognized " + String(identifier, HEX);
					this->log(pullAckLog);
				} break;
			}
		} else {
			String errorLog = "ERROR: size != readSize : " + String(size) + ":" + String(readSize);
			this->log(errorLog);
		}
	}
}

void WAN::stat() {
	WAN::Message::Stat* statMessage = new WAN::Message::Stat(this);
	this->send(statMessage);
	delete statMessage;
}

void WAN::pull() {
	WAN::Message::Pull* pullMessage = new WAN::Message::Pull(this);
	this->send(pullMessage);
	delete pullMessage;
}

/**
 * This is called from LoRaModule when a radio packet is received
 */
void WAN::onRFMPacket(Data::Packet* packet) {
	this->statistics.rxnb += 1u;
	this->statistics.rxok += 1u; // TODO:: check CRC ????

	WAN::RFData* data = new WAN::RFData();
	data->packet = packet;
	data->settings = this->rfm->settings;
	data->rssi = LoRa.packetRssi();
	data->snr = LoRa.packetSnr();

	WAN::Message::RxPk* rxpkMessage = new WAN::Message::RxPk(this);
	rxpkMessage->add(data);
	this->send(rxpkMessage);

	bool connected = (WL_CONNECTED == WiFi.status());
	if (connected) {
		this->statistics.rxfw += 1u;
	}

	String logMessage = "UPLINK :: received:" + String(this->statistics.rxnb) + " forwarded:" + String(this->statistics.rxfw);
	this->log(logMessage);

	delete rxpkMessage;
	delete data;
}

void WAN::send(WAN::Message::Up* up) {
	bool connected = (WL_CONNECTED == WiFi.status());
	if (connected) {
		const char* host = this->settings.host.c_str();
		int begin = this->udp->beginPacket(host, this->settings.port);
		yield();

		size_t write = this->udp->write(up->header, HEADER_LENGTH);
		yield();

		if (NULL != up->json) {
			String jsonstr = "";
			serializeJson(*up->json, jsonstr);
			uint16_t jsonlength = jsonstr.length();
			const char* jsonchar = jsonstr.c_str();
			write += this->udp->write(jsonchar, jsonlength);
			yield();
		}

		int end = this->udp->endPacket();
		yield();
	}
}

/*
Example:
	{
		"txpk":{
			"imme":false,
			"tmst":176681117,
			"freq":868.3,
			"rfch":0,
			"powe":14,
			"modu":"LORA",
			"datr":"SF7BW125",
			"codr":"4/5",
			"ipol":true,
			"size":29,
			"ncrc":true,
			"data":"YEUqASYAAgABTcboOsOjmOFBwTfPlbBp1jxnXSU="
		}
	}

	imme | bool   | Send packet immediately (will ignore tmst & time)
	tmst | number | Send packet on a certain timestamp value (will ignore time)
	tmms | number | Send packet at a certain GPS time (GPS synchronization required)
	freq | number | TX central frequency in MHz (unsigned float, Hz precision)
	rfch | number | Concentrator "RF chain" used for TX (unsigned integer)
	powe | number | TX output power in dBm (unsigned integer, dBm precision)
	modu | string | Modulation identifier "LORA" or "FSK"
	datr | string | LoRa datarate identifier (eg. SF12BW500)
	datr | number | FSK datarate (unsigned, in bits per second)
	codr | string | LoRa ECC coding rate identifier
	fdev | number | FSK frequency deviation (unsigned integer, in Hz) 
	ipol | bool   | Lora modulation polarization inversion
	prea | number | RF preamble size (unsigned integer)
	size | number | RF packet payload size in bytes (unsigned integer)
	data | string | Base64 encoded RF packet payload, padding optional
	ncrc | bool   | If true, disable the CRC of the physical layer (optional)
*/
void WAN::resp(uint8_t* buffer, uint16_t bsize) {
	uint8_t* chardata = (uint8_t*) (buffer + 4);

	DynamicJsonDocument doc(512);
	DeserializationError error = deserializeJson(doc, chardata);
	if (!error) {
		this->statistics.dwnb += 1u;

		JsonObject json = doc.as<JsonObject>();
		JsonObject txpk = json["txpk"];

		bool        imme    = !txpk.containsKey("imme") ? false      : txpk["imme"];
		uint32_t    tmst    = !txpk.containsKey("tmst") ? micros()   : txpk["tmst"].as<uint32_t>();
		// uint32_t    tmms    = !txpk.containsKey("tmms") ? 0ul        : txpk["tmms"].as<uint32_t>();
		double      freq    = !txpk.containsKey("freq") ? 0.0        : txpk["freq"].as<double>();
		uint16_t    powe    = !txpk.containsKey("powe") ? 0u         : txpk["powe"].as<uint16_t>();
		String      modu    = !txpk.containsKey("modu") ? "LORA"     : txpk["modu"].as<String>();
		const char* datr    = !txpk.containsKey("datr") ? "SF7BW125" : txpk["datr"].as<const char*>();
		const char* codr    = !txpk.containsKey("codr") ? "4/5"      : txpk["codr"].as<const char*>();
		bool        ipol    = !txpk.containsKey("ipol") ? false      : txpk["ipol"];
		uint16_t    plength = !txpk.containsKey("prea") ? 8u         : txpk["prea"];
		uint16_t    size    = !txpk.containsKey("size") ? 0u         : txpk["size"].as<uint16_t>();
		const char* data    = !txpk.containsKey("data") ? ""         : txpk["data"].as<const char*>();
		bool        ncrc    = !txpk.containsKey("ncrc") ? false      : txpk["ncrc"];

		String error = "NONE";
		uint32_t now = micros();
		bool tooearly = false; // TODO:: unhardcode it
		if (!tooearly) {
			// TODO:: i've seen that TTN takes too long to send a DOWNLINK to a gateway resulting
			// in a TOO_LATE error almost a 100% of the times ... i decided to emit them no matter what
			// it is up to you if you want to prevent this behaviour
			bool toolate = false;// (tmst < now);
			if (!toolate) {
				bool loraModulation = modu.equals("LORA");
				if (loraModulation) {
					// TODO:: find a better way to do it because of float artifacts
					uint32_t HZ = (uint32_t) (1000000u * freq);
					uint32_t min = this->rfm->settings.freq.min;
					uint32_t max = this->rfm->settings.freq.max;
					if (min <= HZ && HZ <= max) {
						if (2u <= powe && powe <= 20u) { // TODO:: unhardcode it
							uint16_t sfac = atoi(datr + 2);
							if (6u <= sfac && sfac <= 12u) { // TODO:: unhardcode it
								uint32_t sbw = 125u * 1000u; // TODO:: unhardcode it
								uint16_t crat = atoi(codr + 2);
								if (5u <= crat && crat <= 8u) { // TODO:: unhardcode it
									// TODO:: check sbw
									// TODO:: check plength

									WAN::RFData* rfdata = new WAN::RFData();
									rfdata->settings.freq.curr = HZ;
									rfdata->settings.txpw = powe;
									rfdata->settings.sfac = sfac;
									rfdata->settings.sbw = sbw;
									rfdata->settings.crat = crat;
									rfdata->settings.plength = plength;
									rfdata->settings.sw = this->rfm->settings.sw;
									rfdata->settings.crc = !ncrc;
									rfdata->settings.iiq = ipol;

									Data::Packet* packet = new Data::Packet(size);
									rfdata->packet = packet;
									// TODO:: check packet size
									Base64::decode((unsigned char*) data, (unsigned char*) packet->buffer);

									if (imme) {
										this->rfm->apply(&rfdata->settings);
										this->rfm->send(rfdata->packet);
										this->rfm->apply(&this->rfm->settings);
										this->statistics.txnb += 1u;
										delete rfdata->packet;
										delete rfdata;
									} else {
										Scheduled* scheduled = new Scheduled(rfdata, tmst);
										this->schedules->add(scheduled);
									}
								} else {
									error = "TOO_LATE"; // bad coding rate
								}
							} else {
								error = "TOO_LATE"; // bad SF
							}
						} else {
							error = "TX_POWER";
						}
					} else {
						error = "TX_FREQ";
					}
				} else {
					error = "TX_FREQ"; // bad modulation
				}
			} else {
				error = "TOO_LATE";
			}
		} else {
			error = "TOO_EARLY";
		}

		String logMessage = "DOWNLINK -> freq:" + String(freq);
		logMessage += " txpw:" + String(powe);
		logMessage += " datr:" + String(datr);
		logMessage += " codr:" + String(codr);
		logMessage += " prea:" + String(plength);
		logMessage += " ipol:" + String(ipol);
		logMessage += " imme:" + String(imme);
		// logMessage += " tmms:" + String(tmms);
		logMessage += " tmst:" + String(tmst);
		logMessage += " now:" + String(now);
		logMessage += " error:" + error;
		this->log(logMessage);

		WAN::Message::TxAck* txAckMessage = new WAN::Message::TxAck(this, error);
		this->send(txAckMessage);
		delete txAckMessage;	
	}
}

void WAN::getState(JsonObject& wan) {
	this->JSON(wan);
	//rfm["status"] = this->active; // TODO::
}

void WAN::getPing(JsonObject& response) {
	JsonObject object = this->rootIT(response);
	JsonObject mparams = object.createNestedObject("state");
	mparams["now"] = clock64.mstime();
	mparams["ack"] = this->lastACK;

	JsonObject stats = mparams.createNestedObject("stats");
	stats["rxnb"] = this->statistics.rxnb;
	stats["rxok"] = this->statistics.rxok;
	stats["rxfw"] = this->statistics.rxfw;
	stats["ackr"] = this->statistics.ackr;
	stats["dwnb"] = this->statistics.dwnb;
	stats["txnb"] = this->statistics.txnb;
}

void WAN::JSON(JsonObject& wan) {
	char id_char[17] = {0};
	sprintf(id_char, "%02X%02X%02X%02X%02X%02X%02X%02X",
		this->settings.id[0], this->settings.id[1], this->settings.id[2], this->settings.id[3],
		this->settings.id[4], this->settings.id[5], this->settings.id[6], this->settings.id[7]);

	wan["id"] = String(id_char);
	wan["host"] = this->settings.host;
	wan["port"] = this->settings.port;
	wan["desc"] = this->settings.desc;
	wan["mail"] = this->settings.mail;
	wan["platform"] = this->settings.platform;
	wan["lon"] = this->settings.lon;
	wan["lat"] = this->settings.lat;
	wan["alt"] = this->settings.alt;

	wan["istat"] = this->istat;
	wan["ipull"] = this->ipull;
}

byte strtob(const char* str) {
  byte value = (byte) 0;
  for (int i = 0; i < 2; i++) {
    char c = str[i];
    if ('0' <= c && c <= '9') {
      value |= (c - '0') << 4 * (1 - i);
    } else if ('a' <= c && c <= 'f') {
      value |= (c - 'a' + 10) << c * (1 - i);
    } else if ('A' <= c && c <= 'F') {
      value |= (c - 'A' + 10) << 4 * (1 - i);
    }
  }
  return value;
}

void WAN::fromJSON(JsonObject& params) {
	if (params.containsKey("id")) {
		String id = params["id"].as<String>();
		int length = id.length();
		if (16 == length) {
			id.toUpperCase();
			const char* idSTR = id.c_str();
			for (int i = 0; i < 8; i++) {
				this->settings.id[i] = strtob(idSTR + 2 * i);
			}
		}
	}

	if (params.containsKey("host")) { this->settings.host = params["host"].as<String>(); }
	if (params.containsKey("port")) { this->settings.port = params["port"]; }
	if (params.containsKey("desc")) { this->settings.desc = params["desc"].as<String>(); }
	if (params.containsKey("mail")) { this->settings.mail = params["mail"].as<String>(); }
	if (params.containsKey("platform")) { this->settings.platform = params["platform"].as<String>(); }
	if (params.containsKey("lon")) { this->settings.lon = params["lon"].as<double>(); }
	if (params.containsKey("lat")) { this->settings.lat = params["lat"].as<double>(); }
	if (params.containsKey("alt")) { this->settings.alt = params["alt"].as<double>(); }
	if (params.containsKey("istat")) { this->istat = params["istat"].as<uint32_t>(); }
	if (params.containsKey("ipull")) { this->ipull = params["ipull"].as<uint32_t>(); }
}

void WAN::save(JsonObject& params, JsonObject& response, JsonObject& broadcast) {
	this->fromJSON(params);
	this->saveFile();
	this->setup();
	yield();
	JsonObject object = this->rootIT(broadcast);
	JsonObject mparams = object.createNestedObject("state");
	this->getState(mparams);
}

