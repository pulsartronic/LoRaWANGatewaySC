#ifndef __LoRaWAN_Methods__
#define __LoRaWAN_Methods__

using namespace std;

void Log(LoRaWAN* loRaWAN, String text) {
	if (NULL != loRaWAN->logger) {
		String log = "{\"n\":\"wan\",\"p\":" + text + "}";
		loRaWAN->logger->log(log);
	}
}

// ============================================================================
// UDP  FUNCTIONS

// ----------------------------------------------------------------------------
// Send UP an UDP/DGRAM message to the MQTT server
// If we send to more than one host (not sure why) then we need to set sockaddr 
// before sending.
// Parameters:
//	IPAddress
//	port
//	msg *
//	length (of msg)
// return values:
//	0: Error
//	1: Success
// ----------------------------------------------------------------------------
int sendUdp(LoRaWAN* loRaWAN, IPAddress server, int port, uint8_t *msg, int length) {
	if (WL_CONNECTED == WiFi.status()) {
		if (!loRaWAN->Udp.beginPacket(server, (int) port)) {
			Log(loRaWAN, "{\"n\":\"log\",\"p\":\"ERROR sendUdp :: Udp.beginPacket ...\"}");
			return(0);
		}
		
		yield();
		
		if (loRaWAN->Udp.write((unsigned char *) msg, length) != length) {
			Log(loRaWAN, "{\"n\":\"log\",\"p\":\"ERROR sendUdp :: Udp.write ...\"}");
			loRaWAN->Udp.endPacket(); // Close UDP
			return(0); // Return error
		}
		
		yield();
		
		if (!loRaWAN->Udp.endPacket()) {
			Log(loRaWAN, "{\"n\":\"log\",\"p\":\"ERROR sendUdp :: Udp.endPacket ...\"}");
			return(0);
		}
		return(1);
	} else {
		return 0;
	}
} //sendUDP

// ----------------------------------------------------------------------------
// Read DOWN a package from UDP socket, can come from any server
// Messages are received when server responds to gateway requests from LoRa nodes 
// (e.g. JOIN requests etc.) or when server has downstream data.
// We respond only to the server that sent us a message!
// Note: So normally we can forget here about codes that do upstream
// Parameters:
//	Packetsize: size of the buffer to read, as read by loop() calling function
//
// Returns:
//	-1 or false if not read
//	Or number of characters read is success
//
// ----------------------------------------------------------------------------
int readUdp(LoRaWAN* loRaWAN, int packetSize) {
	if (WL_CONNECTED == WiFi.status()) {
		uint16_t token;
		uint8_t ident;
		uint8_t buff[32]; // General buffer to use for UDP, set to 64
		uint8_t buff_down[RX_BUFF_SIZE]; // Buffer for downstream

		if (packetSize > RX_BUFF_SIZE) {
			Log(loRaWAN, "{\"n\":\"log\",\"p\":\"ERROR readUdp :: packetSize:" + String(packetSize) + " max: " + String(RX_BUFF_SIZE) + "\"}");
			loRaWAN->Udp.flush();
			return(-1);
		}
	  
		// We assume here that we know the originator of the message
		// In practice however this can be any sender!
		int readSize = loRaWAN->Udp.read(buff_down, packetSize);
		if (readSize < packetSize) {
			Log(loRaWAN, "{\"n\":\"log\",\"p\":\"ERROR readUdp :: readSize:" + String(readSize) + " packetSize: " + String(packetSize) + "\"}");
			return(-1);
		}

		// Remote Address should be known
		IPAddress remoteIpNo = loRaWAN->Udp.remoteIP();
		// Remote port is either of the remote TTN server or from NTP server (=123)
		unsigned int remotePortNo = loRaWAN->Udp.remotePort();

		if (remotePortNo == 123) {
			// This is an NTP message arriving
			Log(loRaWAN, "{\"n\":\"log\",\"p\":\"NTP msg rcvd\"}");
			//gwayConfig.ntpErr++;
			//gwayConfig.ntpErrTime = now();
			return(0);
		} else {
			// If it is not NTP it must be a LoRa message for gateway or node
			uint8_t *data = (uint8_t*) ((uint8_t*) buff_down + 4);
			uint8_t protocol = buff_down[0];
			token = buff_down[2] * 256 + buff_down[1];
			ident = buff_down[3];

			// now parse the message type from the server (if any)
			switch (ident) {
				// This message is used by the gateway to send sensor data to the
				// server. As this function is used for downstream only, this option
				// will never be selected but is included as a reference only
				case PKT_PUSH_DATA: { // 0x00 UP
					String pushLog = "PKT_PUSH_DATA size " + String(packetSize) + " From " + remoteIpNo.toString() + ", port " + String(remotePortNo);
					Log(loRaWAN, "{\"n\":\"log\",\"p\":\"" + pushLog + "\"}");
				} break;
				// This message is sent by the server to acknoledge receipt of a
				// (sensor) message sent with the code above.
				case PKT_PUSH_ACK: {	// 0x01 DOWN
					String pushLog = "PKT_PUSH_ACK size " + String(packetSize) + " From " + remoteIpNo.toString() + ", port " + String(remotePortNo) + ", token: " + String(token, HEX);
					Log(loRaWAN, "{\"n\":\"ack\",\"p\":\"" + pushLog + "\"}");
					loRaWAN->lastACK = now();
				} break;
				case PKT_PULL_DATA: {	// 0x02 UP
					Log(loRaWAN, "{\"n\":\"log\",\"p\":\"readUdp :: PKT_PULL_DATA received\"}");
				} break;
				// This message type is used to confirm OTAA message to the node
				// XXX This message format may also be used for other downstream communucation
				case PKT_PULL_RESP: {	// 0x03 DOWN
					Log(loRaWAN, "{\"n\":\"log\",\"p\":\"PKT_PULL_RESP received\"}");
					//	lastTmst = micros();	// Store the tmst this package was received
					// Send to the LoRa Node first (timing) and then do reporting to Serial
					_state = S_TX;
					sendTime = micros();	// record when we started sending the message
					if (sendPacket(loRaWAN, data, packetSize - 4) < 0) {
						Log(loRaWAN, "{\"n\":\"log\",\"p\":\"ERROR readUdp :: PKT_PULL_RESP sendPacket failed\"}");
						return(-1);
					}

					// Now respond with an PKT_TX_ACK; 0x04 UP
					buff[0] = buff_down[0];
					buff[1] = buff_down[1];
					buff[2] = buff_down[2];
					//buff[3] = PKT_PULL_ACK; // Pull request/Change of Mogyi
					buff[3] = PKT_TX_ACK;
					buff[4] = loRaWAN->gid[0];
					buff[5] = loRaWAN->gid[1];
					buff[6] = loRaWAN->gid[2];
					buff[7] = loRaWAN->gid[3];
					buff[8] = loRaWAN->gid[4];
					buff[9] = loRaWAN->gid[5];
					buff[10] = loRaWAN->gid[6];
					buff[11] = loRaWAN->gid[7];
					buff[12] = 0;

					// Only send the PKT_PULL_ACK to the UDP socket that just sent the data!!!
					loRaWAN->Udp.beginPacket(remoteIpNo, remotePortNo);
					int writen = loRaWAN->Udp.write((unsigned char*)buff, 12);
					if (writen != 12) {
						Log(loRaWAN, "{\"n\":\"log\",\"p\":\"ERROR readUdp :: PKT_PULL_ACK UDP.write total: 12 writen: " + String(writen) + "\"}");
					} else {
						//#if DUSB>=1
						//	if (( debug>=0 ) && ( pdebug & P_TX )) {
								Serial.print(F("M PKT_TX_ACK:: micros="));
								Serial.println(micros());
						//	}
						//#endif
					}

					if (!loRaWAN->Udp.endPacket()) {
						Log(loRaWAN, "{\"n\":\"log\",\"p\":\"ERROR readUdp :: PKT_PULL_DATALL Udp.endpaket\"}");
					}
					
					yield();

					//#if DUSB>=1
						//if (( debug >=1 ) && (pdebug & P_MAIN )) {
							//Serial.print(F("M PKT_PULL_RESP:: size ")); 
							//Serial.print(packetSize);
							//Serial.print(F(" From ")); 
							//Serial.print(remoteIpNo);
							//Serial.print(F(", port ")); 
							//Serial.print(remotePortNo);	
							//Serial.print(F(", data: "));
							//data = buff_down + 4;
							//data[packetSize] = 0;
							//Serial.print((char *)data);
							//Serial.println(F("..."));
						//}
					//#endif		
				} break;
				
				case PKT_PULL_ACK: { // 0x04 DOWN; the server sends a PULL_ACK to confirm PULL_DATA receipt
					String pullAckLog = "PKT_PULL_ACK size " + String(packetSize) + " From " + remoteIpNo.toString() + ", port " + String(remotePortNo);
					Log(loRaWAN, "{\"n\":\"ack\",\"p\":\"" + pullAckLog + "\"}");
					loRaWAN->lastACK = now();
				} break;
				
				default: {
					String pullAckLog = "ERROR readUdp :: ident not recognized " + String(ident, HEX);
					Log(loRaWAN, "{\"n\":\"log\",\"p\":\"" + pullAckLog + "\"}");
				} break;
			}

			//#if DUSB>=2
			//	if (debug>=1) {
			//		Serial.print(F("readUdp:: returning=")); 
			//		Serial.println(packetSize);
			//	}
			//#endif
			// For downstream messages
			return packetSize;
		}
	} else {
		return 0;
	}

}//readUdp



// ----------------------------------------------------------------------------
// UDPconnect(): connect to UDP (which is a local thing, after all UDP 
// connections do not exist.
// Parameters:
//	<None>
// Returns
//	Boollean indicating success or not
// ----------------------------------------------------------------------------
bool UDPconnect(LoRaWAN* loRaWAN) {
	unsigned int localPort = _LOCUDPPORT; // To listen to return messages from WiFi
	Log(loRaWAN, "{\"n\":\"log\",\"p\":\"Local UDP port: " + String(localPort) + "\"}");
	loRaWAN->UDPConnected = (loRaWAN->Udp.begin(localPort) == 1);
	if (loRaWAN->UDPConnected) {
		Log(loRaWAN, "{\"n\":\"log\",\"p\":\"UDPconnect :: Connection successful\"}");
	} else {
		Log(loRaWAN, "{\"n\":\"log\",\"p\":\"ERROR UDPconnect :: Connection failed\"}");
	}
	return (loRaWAN->UDPConnected);
} //udpConnect


// ----------------------------------------------------------------------------
// Send UP periodic Pull_DATA message to server to keepalive the connection
// and to invite the server to send downstream messages when these are available
// *2, par. 5.2
//	- Protocol Version (1 byte)
//	- Random Token (2 bytes)
//	- PULL_DATA identifier (1 byte) = 0x02
//	- Gateway unique identifier (8 bytes) = MAC address
// ----------------------------------------------------------------------------
void pullData(LoRaWAN* loRaWAN) {
	uint8_t pullDataReq[12]; // status report as a JSON object
	int pullIndex = 0;
	int i;
	
	uint8_t token_h = (uint8_t)rand(); 						// random token
	uint8_t token_l = (uint8_t)rand();						// random token

	// pre-fill the data buffer with fixed fields
	pullDataReq[0]  = PROTOCOL_VERSION;						// 0x01
	pullDataReq[1]  = token_h;
	pullDataReq[2]  = token_l;
	pullDataReq[3]  = PKT_PULL_DATA;						// 0x02
	// READ MAC ADDRESS OF ESP8266, and return unique Gateway ID consisting of MAC address and 2bytes 0xFF
	pullDataReq[4]  = loRaWAN->gid[0];
	pullDataReq[5]  = loRaWAN->gid[1];
	pullDataReq[6]  = loRaWAN->gid[2];
	pullDataReq[7]  = loRaWAN->gid[3];
	pullDataReq[8]  = loRaWAN->gid[4];
	pullDataReq[9]  = loRaWAN->gid[5];
	pullDataReq[10] = loRaWAN->gid[6];
	pullDataReq[11] = loRaWAN->gid[7];
	//pullDataReq[12] = 0/00; 								// add string terminator, for safety
	
	pullIndex = 12;											// 12-byte header
	
	//send the update
	uint8_t *pullPtr;
	pullPtr = pullDataReq;
	sendUdp(loRaWAN, loRaWAN->ttnServer, loRaWAN->port, pullDataReq, pullIndex);
	yield();

	if (pullPtr != pullDataReq) {
		Log(loRaWAN, "{\"n\":\"log\",\"p\":\"pullPtr != pullDatReq\"}");
	}

	//#if DUSB>=1
	//	if (( debug>=2 ) && ( pdebug & P_MAIN )) {
			yield();
			//Serial.print(F("M PKT_PULL_DATA request, len=<"));
			//Serial.print(pullIndex);
			//Serial.print(F("> "));
			//for (i=0; i<pullIndex; i++) {
			//	Serial.print(pullDataReq[i],HEX); // debug: display JSON stat
			//	Serial.print(':');
			//}
			//Serial.println();
			//if (debug>=2) Serial.flush();
	//	}
	//#endif
	return;
}//pullData


// ----------------------------------------------------------------------------
// Send UP periodic status message to server even when we do not receive any
// data. 
// Parameters:
//	- <none>
// ----------------------------------------------------------------------------
void sendstat(LoRaWAN* loRaWAN) {
	uint8_t status_report[STATUS_SIZE]; // status report as a JSON object
	char stat_timestamp[32]; // XXX was 24
	time_t t;
	char clat[10] = {0};
	char clon[10] = {0};

	int stat_index=0;
	uint8_t token_h   = (uint8_t)rand(); // random token
	uint8_t token_l   = (uint8_t)rand(); // random token
	
	// pre-fill the data buffer with fixed fields
	status_report[0]  = PROTOCOL_VERSION; // 0x01
	status_report[1]  = token_h;
	status_report[2]  = token_l;
	status_report[3]  = PKT_PUSH_DATA; // 0x00
	
	// READ MAC ADDRESS OF ESP8266, and return unique Gateway ID consisting of MAC address and 2bytes 0xFF
	status_report[4]  = loRaWAN->gid[0];
	status_report[5]  = loRaWAN->gid[1];
	status_report[6]  = loRaWAN->gid[2];
	status_report[7]  = loRaWAN->gid[3];
	status_report[8]  = loRaWAN->gid[4];
	status_report[9]  = loRaWAN->gid[5];
	status_report[10] = loRaWAN->gid[6];
	status_report[11] = loRaWAN->gid[7];

    stat_index = 12;										// 12-byte header
	
    t = now();												// get timestamp for statistics
	
	// XXX Using CET as the current timezone. Change to your timezone	
	sprintf(stat_timestamp, "%04d-%02d-%02d %02d:%02d:%02d CET", year(),month(),day(),hour(),minute(),second());
	yield();
	
	ftoa(loRaWAN->lat, clat, 5); // Convert lat to char array with 5 decimals
	ftoa(loRaWAN->lon, clon, 5); // As IDE CANNOT prints floats
	
	// Build the Status message in JSON format, XXX Split this one up...
	delay(1);
	
    int j = snprintf((char *)(status_report + stat_index), STATUS_SIZE-stat_index, 
		"{\"stat\":{\"time\":\"%s\",\"lati\":%s,\"long\":%s,\"alti\":%i,\"rxnb\":%u,\"rxok\":%u,\"rxfw\":%u,\"ackr\":%u.0,\"dwnb\":%u,\"txnb\":%u,\"pfrm\":\"%s\",\"mail\":\"%s\",\"desc\":\"%s\"}}", 
		stat_timestamp, clat, clon, (int)loRaWAN->alt, cp_nb_rx_rcv, cp_nb_rx_ok, cp_up_pkt_fwd, 0, 0, 0, loRaWAN->platform.c_str(), loRaWAN->mail.c_str(), loRaWAN->desc.c_str());
		
	yield(); // Give way to the internal housekeeping of the ESP8266

	stat_index += j;
	status_report[stat_index] = 0; // add string terminator, for safety

	//#if DUSB>=1
	//	 if (( debug>=2 ) && ( pdebug & P_MAIN )) {
	Log(loRaWAN, "{\"n\":\"stat\",\"p\":" + String((char *)(status_report + 12)) + "}");
	//Serial.print(F("M stat update: <"));
	//Serial.print(stat_index);
	//Serial.print(F("> "));
	//Serial.println((char *)(status_report + 12)); // DEBUG: display JSON stat
	//	}
	//#endif

	if (stat_index > STATUS_SIZE) {
		Log(loRaWAN, "{\"n\":\"log\",\"p\":\"ERROR sendstat :: buffer too big, size: " + String(stat_index) + " max: " + String(STATUS_SIZE) + "\"}");
		return;
	}
	
	//send the update
	sendUdp(loRaWAN, loRaWAN->ttnServer, loRaWAN->port, status_report, stat_index);
	yield();

	return;
} //sendstat


// ----------------------------------------------------------------------------
// Send the request packet to the NTP server.
//
// ----------------------------------------------------------------------------
int sendNtpRequest(LoRaWAN* loRaWAN, IPAddress timeServerIP) {
	const int NTP_PACKET_SIZE = 48;				// Fixed size of NTP record
	byte packetBuffer[NTP_PACKET_SIZE];

	memset(packetBuffer, 0, NTP_PACKET_SIZE);	// Zeroise the buffer.
	
	packetBuffer[0]  = 0b11100011;   			// LI, Version, Mode
	packetBuffer[1]  = 0;						// Stratum, or type of clock
	packetBuffer[2]  = 6;						// Polling Interval
	packetBuffer[3]  = 0xEC;					// Peer Clock Precision
	// 8 bytes of zero for Root Delay & Root Dispersion
	packetBuffer[12] = 49;
	packetBuffer[13] = 0x4E;
	packetBuffer[14] = 49;
	packetBuffer[15] = 52;	

	if (!sendUdp(loRaWAN, (IPAddress) timeServerIP, (int) 123, packetBuffer, NTP_PACKET_SIZE)) {
		//gwayConfig.ntpErr++;
		//gwayConfig.ntpErrTime = now();
		return(0);	
	}
	return(1);
}


// ----------------------------------------------------------------------------
// Get the NTP time from one of the time servers
// Note: As this function is called from SyncINterval in the background
//	make sure we have no blocking calls in this function
// ----------------------------------------------------------------------------
time_t getNtpTime(LoRaWAN* loRaWAN) {
	if (!sendNtpRequest(loRaWAN, loRaWAN->ntpServer))	{ // Send the request for new time
		Log(loRaWAN, "{\"n\":\"log\",\"p\":\"ERROR getNtpTime :: sendNtpRequest failed\"}");
		// Serial.println(F("M sendNtpRequest failed"));
		return(0);
	}
	
	const int NTP_PACKET_SIZE = 48; // Fixed size of NTP record
	byte packetBuffer[NTP_PACKET_SIZE];
	memset(packetBuffer, 0, NTP_PACKET_SIZE); // Set buffer cntents to zero

	uint32_t beginWait = millis();
	delay(10);

	while (millis() - beginWait < 1500)  {
		int size = loRaWAN->Udp.parsePacket();
		if ( size >= NTP_PACKET_SIZE ) {
			if (loRaWAN->Udp.read(packetBuffer, NTP_PACKET_SIZE) < NTP_PACKET_SIZE) {
				break;
			} else {
				// Extract seconds portion.
				unsigned long secs;
				secs  = packetBuffer[40] << 24;
				secs |= packetBuffer[41] << 16;
				secs |= packetBuffer[42] <<  8;
				secs |= packetBuffer[43];
				// UTC is 1 TimeZone correction when no daylight saving time
				return(secs - 2208988800UL + loRaWAN->tzones * SECS_IN_HOUR);
			}
			loRaWAN->Udp.flush();	
		}
		delay(100); // Wait 100 millisecs, allow kernel to act when necessary
	}

	loRaWAN->Udp.flush();

	// If we are here, we could not read the time from internet
	// So increase the counter
	//gwayConfig.ntpErr++;
	//gwayConfig.ntpErrTime = now();

	Log(loRaWAN, "{\"n\":\"log\",\"p\":\"ERROR getNtpTime :: read failed\"}");
	Serial.println(F("M getNtpTime:: read failed"));

	return(0); // return 0 if unable to get the time
}

bool updateTime(LoRaWAN* loRaWAN) {
	time_t newTime = (time_t) getNtpTime(loRaWAN);
	bool updated = (newTime != 0);
	if (updated) {
		setTime(newTime);
		loRaWAN->ntptimer = newTime;
		Log(loRaWAN, "{\"n\":\"log\",\"p\":\"Time updated: " + String(now()) + "\"}");
	} else {
		Log(loRaWAN, "{\"n\":\"log\",\"p\":\"ERROR :: time not updated " + String(now()) + "\"}");
	}
	return updated;
}

void fromJSON(LoRaWAN* loRaWAN, JsonObject wan) {
	if (wan.containsKey("id")) {
		String id = wan["id"].as<String>();
		int length = id.length();
		if (16 == length) {
			id.toUpperCase();
			const char* idSTR = id.c_str();
			for (int i = 0; i < 8; i++) {
				loRaWAN->gid[i] = strtob(idSTR + 2 * i);
			}
		}
	}

	if (wan.containsKey("server")) loRaWAN->server = wan["server"].as<String>();
	if (wan.containsKey("port")) loRaWAN->port = wan["port"];
	if (wan.containsKey("desc")) loRaWAN->desc = wan["desc"].as<String>();
	if (wan.containsKey("mail")) loRaWAN->mail = wan["mail"].as<String>();
	if (wan.containsKey("platform")) loRaWAN->platform = wan["platform"].as<String>();
	if (wan.containsKey("lon")) loRaWAN->lon = wan["lon"];
	if (wan.containsKey("lat")) loRaWAN->lat = wan["lat"];
	if (wan.containsKey("alt")) loRaWAN->alt = wan["alt"];
	if (wan.containsKey("istat")) loRaWAN->istat = wan["istat"];
	if (wan.containsKey("ipull")) loRaWAN->ipull = wan["ipull"];
	if (wan.containsKey("ntp")) loRaWAN->ntp = wan["ntp"].as<String>();
	if (wan.containsKey("tzones")) loRaWAN->tzones = wan["tzones"];
	if (wan.containsKey("intp")) loRaWAN->intp = wan["intp"];
}

DynamicJsonDocument JSON(LoRaWAN* loRaWAN) {
	DynamicJsonDocument doc(1024);
	JsonObject wan = doc.to<JsonObject>();
	String id = "";
	for (int i = 0; i < 8; i++) {
		id += btostr(loRaWAN->gid[i]);
	}
	id.toUpperCase();
	wan["id"] = id;
	wan["server"] = loRaWAN->server;
	wan["port"] = loRaWAN->port;
	wan["desc"] = loRaWAN->desc;
	wan["mail"] = loRaWAN->mail;
	wan["platform"] = loRaWAN->platform;
	wan["lon"] = loRaWAN->lon;
	wan["lat"] = loRaWAN->lat;
	wan["alt"] = loRaWAN->alt;
	wan["istat"] = loRaWAN->istat;
	wan["ipull"] = loRaWAN->ipull;
	wan["ntp"] = loRaWAN->ntp;
	wan["tzones"] = loRaWAN->tzones;
	wan["intp"] = loRaWAN->intp;
	return doc;
}

String Hrdwr(LoRaWAN* loRaWAN) {
	DynamicJsonDocument doc = JSON(loRaWAN);
	doc["sip"] = loRaWAN->ttnServer.toString();
	doc["nip"] = loRaWAN->ntpServer.toString();
	doc["now"] = now();
	doc["ack"] = loRaWAN->lastACK;
	String hrdwr = "";
	serializeJson(doc, hrdwr);
	return hrdwr;
}

void saveFile(LoRaWAN* loRaWAN) {
	DynamicJsonDocument doc = JSON(loRaWAN);
	String json = "";
	serializeJson(doc, json);
	File file = SPIFFS.open(loRaWAN->filename, "w");
	file.println(json);
	file.close();
}

void readFile(LoRaWAN* loRaWAN) {
	if (SPIFFS.exists(loRaWAN->filename)) {
		File file = SPIFFS.open(loRaWAN->filename, "r");
		String json = file.readStringUntil('\n');
		file.close();
		DynamicJsonDocument doc(1024);
		DeserializationError error = deserializeJson(doc, json);
		if (!error) {
			JsonObject wan = doc.as<JsonObject>();
			fromJSON(loRaWAN, wan);
		}
	}
}

void updateTTNAddress(LoRaWAN* loRaWAN) {
	IPAddress ttnServer;
	const char* server = loRaWAN->server.c_str();
	if (!WiFi.hostByName(server, ttnServer)) { // Use DNS to get server IP once
		Log(loRaWAN, "{\"n\":\"log\",\"p\":\"ERROR phase_0 :: hostByName TTN\"}");
	} else {
		Log(loRaWAN, "{\"n\":\"log\",\"p\":\"DNS for: " + loRaWAN->server + " : " + ttnServer.toString() + "\"}");
	}
	loRaWAN->ttnServer = ttnServer;
	yield();
}

void updateNTPAddress(LoRaWAN* loRaWAN) {
	IPAddress ntpServer;
	const char* ntp = loRaWAN->ntp.c_str();
	if (!WiFi.hostByName(ntp, ntpServer)) {	// Get IP address of Timeserver
		Log(loRaWAN, "{\"n\":\"log\",\"p\":\"ERROR updateNtpAddress :: hostByName NTP\"}");
	} else {
		Log(loRaWAN, "{\"n\":\"log\",\"p\":\"DNS for: " + loRaWAN->ntp + " : " + ntpServer.toString() + "\"}");
	}
	loRaWAN->ntpServer = ntpServer;
	yield();
}

void setup(LoRaWAN* loRaWAN) {
	Serial.println(F("Starting LoRaWAN system ..."));

	// We choose the Gateway ID to be the MAC Address of our Gateway card
	// display results of getting hardware address
	uint8_t MAC_array[6] = {0};
	WiFi.macAddress(MAC_array);
	
	loRaWAN->gid[0] = MAC_array[0];
	loRaWAN->gid[1] = MAC_array[1];
	loRaWAN->gid[2] = MAC_array[2];
	loRaWAN->gid[3] = 0xFF;
	loRaWAN->gid[4] = 0xFF;
	loRaWAN->gid[5] = MAC_array[3];
	loRaWAN->gid[6] = MAC_array[4];
	loRaWAN->gid[7] = MAC_array[5];

	char MAC_char[19]; // XXX Unbelievable
	MAC_char[18] = 0;
	sprintf(MAC_char,"%02X%02X%02XFFFF%02X%02X%02X", MAC_array[0], MAC_array[1], MAC_array[2], MAC_array[3], MAC_array[4], MAC_array[5]);
	loRaWAN->id = String(MAC_char);

	if (SPIFFS.exists(loRaWAN->filename)) {
		readFile(loRaWAN);
	}
}

void phase_0(LoRaWAN* loRaWAN) {
	bool connected = (WL_CONNECTED == WiFi.status());
	if (connected) {
		dns_getserver(0);
		loRaWAN->phase += 1;
		yield();

		Serial.print(F("WiFi connected to: "));
		Serial.println(WiFi.SSID());
		Serial.println("ip: " + WiFi.localIP().toString());
		Serial.println(F("--------------------------------------"));
	}
}

void phase_1(LoRaWAN* loRaWAN) {
	bool connected = (WL_CONNECTED == WiFi.status());
	if (connected) {
		uint64_t now = sysclock->mstime();
		uint32_t diff = (uint32_t) (now - loRaWAN->lphase);
		if (loRaWAN->iphase <= diff) {
			loRaWAN->lphase = now;

			bool done = true;

			if (!loRaWAN->UDPConnected) {
				UDPconnect(loRaWAN);
			}
			done = done && loRaWAN->UDPConnected;
			yield();

			bool ntpServerSet = loRaWAN->ntpServer.isSet();
			if (!ntpServerSet) {
				updateNTPAddress(loRaWAN);
			}
			done = done && loRaWAN->ntpServer.isSet();
			yield();

			bool ttnServerSet = loRaWAN->ttnServer.isSet();
			if (!ttnServerSet) {
				updateTTNAddress(loRaWAN);
			}
			done = done && loRaWAN->ttnServer.isSet();
			yield();

			if (0ul >= loRaWAN->ntptimer) {
				updateTime(loRaWAN);
			}
			done = done && (0ul < loRaWAN->ntptimer) ;
			yield();

			loRaWAN->phase += done ? 1 : 0;

			if (!done) {
				String log = "{\"n\":\"log\",\"p\":\"phase_1 incomplete :: ";
				log += "udp:" + String(loRaWAN->UDPConnected) + " ";
				log += "ntp:" + String(loRaWAN->ntpServer.isSet()) + " ";
				log += "ttn:" + String(loRaWAN->ttnServer.isSet()) + " ";
				log += "time:" + String(0ul < loRaWAN->ntptimer) + "\"}";
				Log(loRaWAN,  log);
			} else {
				String log = "{\"n\":\"hrdwr\",\"p\":" + Hrdwr(loRaWAN) + "}";
				Log(loRaWAN,  log);
			}
		}
	}
}

void phase_2(LoRaWAN* loRaWAN) {
	uint32_t uSeconds; // micro seconds
	int packetSize;
	uint32_t nowSeconds = now();
	
	if (WL_CONNECTED == WiFi.status()) {
		// So if we are connected 
		// Receive UDP PUSH_ACK messages from server. (*2, par. 3.3)
		// This is important since the TTN broker will return confirmation
		// messages on UDP for every message sent by the gateway. So we have to consume them.
		// As we do not know when the server will respond, we test in every loop.
		while( (packetSize = loRaWAN->Udp.parsePacket()) > 0) {
			//#if DUSB>=2
				//Serial.println(F("loop:: readUdp calling"));
			//#endif
			// DOWNSTREAM
			// Packet may be PKT_PUSH_ACK (0x01), PKT_PULL_ACK (0x03) or PKT_PULL_RESP (0x04)
			// This command is found in byte 4 (buffer[3])
			if (readUdp(loRaWAN, packetSize) <= 0) {
				Log(loRaWAN, "{\"n\":\"log\",\"p\":\"ERROR phase_1 :: readUDP error\"}");
				//Serial.println(F("M readUDP error"));
				break;
			} else {
				// Now we know we succesfully received message from host
				//_event=1;	// Could be done double if more messages received
			}
		}

		yield();	// XXX 26/12/2017

		// stat PUSH_DATA message (*2, par. 4)
		if ((nowSeconds - statTime) >= loRaWAN->istat) { // Wake up every xx seconds
			// Log(loRaWAN, "{\"n\":\"pull\",\"p\":\"Sending STAT ...\"}");
			sendstat(loRaWAN); // Show the status message and send to server
			statTime = nowSeconds;
		}
		
		yield();

		// send PULL_DATA message (*2, par. 4)
		nowSeconds = now();
		if ((nowSeconds - pulltime) >= loRaWAN->ipull) { // Wake up every xx seconds
			Log(loRaWAN, "{\"n\":\"pull\",\"p\":\"PULLING data ...\"}");
			pullData(loRaWAN); // Send PULL_DATA message to server
			startReceiver(loRaWAN->loRaModule);
			pulltime = nowSeconds;
		}
	}

	// Set the time in a manual way. Do not use setSyncProvider
	// as this function may collide with SPI and other interrupts
	yield(); // 26/12/2017
	nowSeconds = now();
	if (nowSeconds - loRaWAN->ntptimer >= loRaWAN->intp) {
		yield();
		updateTime(loRaWAN);
		loRaWAN->ntptimer = nowSeconds;
	}
} //loop


void Loop(LoRaWAN* loRaWAN) {
	switch(loRaWAN->phase) {
		case 0:
			phase_0(loRaWAN);
			break;
		case 1:
			phase_1(loRaWAN);
			break;
		case 2:
			phase_2(loRaWAN);
			break;
	}
}


#endif
