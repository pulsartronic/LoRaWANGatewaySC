#include <WAN.h>

// //////////////////////////////////////////////////////////////////////////////////////
// Up Message
WAN::Message::Up::Up(WAN* wan, uint16_t jsonLength) {	
	this->header[0]  = PROTOCOL_VERSION; // protocol version
	this->header[1]  = (uint8_t) rand(); // random token
	this->header[2]  = (uint8_t) rand(); // random token
	// this->buffer[3]  = IDENTIFIER;
	for (int i= 0; i < 8; i++) {
		this->header[4 + i]  = wan->settings.id[i];
	}

	if (jsonLength) {
		this->json = new DynamicJsonDocument(jsonLength);
	}
}

WAN::Message::Up::~Up() {	
	delete this->json;
}

// //////////////////////////////////////////////////////////////////////////////////////
// Stat Message
WAN::Message::Stat::Stat(WAN* wan) : WAN::Message::Up(wan, 512u) {
	this->header[3]  = PUSH_DATA;
	JsonObject stat = this->json->createNestedObject("stat");
	// time | string | UTC 'system' time of the gateway, ISO 8601 'expanded' format
	// char timestamp[32] = {0};
	// TODO:: sprintf(timestamp, "%04d-%02d-%02d %02d:%02d:%02d CET", year(), month(), day(), hour(), minute(), second());
	// stat["time"] = String(timestamp);
	// lati | number | GPS latitude of the gateway in degree (float, N is +)
	stat["lati"] = wan->settings.lat;
	// long | number | GPS latitude of the gateway in degree (float, E is +)
	stat["long"] = wan->settings.lon;
	// alti | number | GPS altitude of the gateway in meter RX (integer)
	stat["alti"] = wan->settings.alt;
	// rxnb | number | Number of radio packets received (unsigned integer)
	stat["rxnb"] = wan->statistics.rxnb;
	// rxok | number | Number of radio packets received with a valid PHY CRC
	stat["rxok"] = wan->statistics.rxok;
	// rxfw | number | Number of radio packets forwarded (unsigned integer)
	stat["rxfw"] = wan->statistics.rxfw;
	// ackr | number | Percentage of upstream datagrams that were acknowledged
	stat["ackr"] = wan->statistics.ackr;
	// dwnb | number | Number of downlink datagrams received (unsigned integer)
	stat["dwnb"] = wan->statistics.dwnb;
	// txnb | number | Number of packets emitted (unsigned integer)
	stat["txnb"] = wan->statistics.txnb;
}

// //////////////////////////////////////////////////////////////////////////////////////
// RxPk Message
WAN::Message::RxPk::RxPk(WAN* wan) : WAN::Message::Up(wan, 768u) {
	this->header[3] = PUSH_DATA;
	this->json->createNestedArray("rxpk");
}

void WAN::Message::RxPk::add(WAN::RFData* data) {
	JsonArray rxpk = (*this->json)["rxpk"];
	JsonObject pkdata = rxpk.createNestedObject();
	// tmms | number | GPS time of pkt RX, number of milliseconds since 06.Jan.1980
	// char timestamp[32] = {0};
	// sprintf(timestamp, "%04d-%02d-%02dT%02d:%02d:%02d.000000Z", year(), month(), day(), hour(), minute(), second());
	// time | string | UTC time of pkt RX, us precision, ISO 8601 'compact' format
	// pkdata["time"] = String(timestamp);
	// TODO:: ???
	// tmst | number | Internal timestamp of "RX finished" event (32b unsigned)
	pkdata["tmst"] = micros();
	// freq | number | RX central frequency in MHz (unsigned float, Hz precision)
	// char cfreq[12] = {0};
	// ftoa((double)freq / 1000000, cfreq, 6);
	pkdata["freq"] = (double) data->settings.freq.curr / 1000000.0;
	// chan | number | Concentrator "IF" channel used for RX (unsigned integer)
	pkdata["chan"] = 0;
	// rfch | number | Concentrator "RF chain" used for RX (unsigned integer)
	pkdata["rfch"] = 0u;
	// stat | number | CRC status: 1 = OK, -1 = fail, 0 = no CRC
	pkdata["stat"] = data->settings.crc;
	// modu | string | Modulation identifier "LORA" or "FSK"
	pkdata["modu"] = "LORA";
	// datr | string | LoRa datarate identifier (eg. SF12BW500)
	// datr | number | FSK datarate (unsigned, in bits per second)
	pkdata["datr"] = "SF" + String(data->settings.sfac) + "BW" + String(data->settings.sbw / 1000); // TODO:: find a better way ?
	// codr | string | LoRa ECC coding rate identifier
	pkdata["codr"] = "4/" + String(data->settings.crat);
	// rssi | number | RSSI in dBm (signed integer, 1 dB precision)
	pkdata["rssi"] = data->rssi;
	// lsnr | number | Lora SNR ratio in dB (signed float, 0.1 dB precision)
	pkdata["lsnr"] = data->snr;
	// size | number | RF packet payload size in bytes (unsigned integer)
	pkdata["size"] = data->packet->size;
	// data | string | Base64 encoded RF packet payload, padded
	unsigned int b64Length = Base64::encode_length(data->packet->size);
	unsigned char base64[b64Length + 1]; base64[b64Length] = '\0';
	Base64::encode(data->packet->buffer, data->packet->size, base64);
	pkdata["data"] = String((char*)base64);
}

// //////////////////////////////////////////////////////////////////////////////////////
// Pull Message
WAN::Message::Pull::Pull(WAN* wan) : WAN::Message::Up(wan, 0u) {
	this->header[3]  = PULL_DATA;
}

// //////////////////////////////////////////////////////////////////////////////////////
// TxAck Message
WAN::Message::TxAck::TxAck(WAN* wan, String error) : WAN::Message::Up(wan, 512u) {
	this->header[3]  = TX_ACK;
	JsonObject txpk_ack = this->json->createNestedObject("txpk_ack");
	// NONE              | Packet has been programmed for downlink
	// TOO_LATE          | Rejected because it was already too late to program this packet for downlink
	// TOO_EARLY         | Rejected because downlink packet timestamp is too much in advance
	// COLLISION_PACKET  | Rejected because there was already a packet programmed in requested timeframe
	// COLLISION_BEACON  | Rejected because there was already a beacon planned in requested timeframe
	// TX_FREQ           | Rejected because requested frequency is not supported by TX RF chain
	// TX_POWER          | Rejected because requested power is not supported by gateway
	// GPS_UNLOCKED      | Rejected because GPS is unlocked, so GPS timestamp cannot be used
	txpk_ack["error"] = error;
}

