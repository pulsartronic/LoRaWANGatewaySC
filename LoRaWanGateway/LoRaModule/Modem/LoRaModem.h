// 1-channel LoRa Gateway for ESP8266
// Copyright (c) 2016, 2017, 2018 Maarten Westenberg version for ESP8266
// Version 5.3.3
// Date: 2018-08-25
//
// 	based on work done by Thomas Telkamp for Raspberry PI 1ch gateway
//	and many other contributors.
//
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the MIT License
// which accompanies this distribution, and is available at
// https://opensource.org/licenses/mit-license.php
//
// NO WARRANTY OF ANY KIND IS PROVIDED
//
// Author: Maarten Westenberg (mw12554@hotmail.com)
//
// This file contains a number of compile-time settings and declarations that are
// specific to the LoRa rfm95, sx1276, sx1272 radio of the gateway.
//
//
// ------------------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// gway_failed is a function called by ASSERT in ESP-sc-gway.h
//
// ----------------------------------------------------------------------------

#define ASSERT(cond) /**/

// ----------------------------------------
// Used by REG_PAYLOAD_LENGTH to set receive payload length
#define PAYLOAD_LENGTH              0x40		// 64 bytes
#define MAX_PAYLOAD_LENGTH          0x80		// 128 bytes

// In order to make the CAD behaviour dynamic we set a variable
// when the CAD functions are defined. Value of 3 is minimum frequencies a
// gateway should support to be fully LoRa compliant.
// For performance reasons, 3 is the maximum as well!
//
#define NUM_HOPS 3

// Do not change these setting for RSSI detection. They are used for CAD
// Given the correction factor of 157, we can get to -122dB with this rating
// 
#define RSSI_LIMIT	35							// 

// How long to wait in LoRa mode before using the RSSI value.
// This period should be as short as possible, yet sufficient
// 
#define RSSI_WAIT	6							// was 25

// How long will it take when hopping before a CDONE or CDETD value
// is present and can be measured.
//
#define EVENT_WAIT 15000						// XXX 180520 was 25 milliseconds before CDDETD timeout
#define DONE_WAIT 1950							// 2000 microseconds (1/500) sec between CDDONE events


// Our code should correct the server Tramission delay settings
long txDelay = 0x00; // tx delay time on top of server TMST

// SPI setting. 8MHz seems to be the max
#define SPISPEED 8000000						// Set to 8 * 10E6

// Frequencies
// Set center frequency. If in doubt, choose the first one, comment all others
// Each "real" gateway should support the first 3 frequencies according to LoRa spec.
// NOTE: This means you have to specify at least 3 frequencies here for the single
//	channel gateway to work.

// This the the EU868 format as used in most of Europe
// It is also the default for most of the single channel gateway work.
int EU868 [] = { 
	868100000, // Channel 0, 868.1 MHz/125 primary
	868300000, // Channel 1, 868.3 MHz mandatory
	868500000, // Channel 2, 868.5 MHz mandatory
	867100000, // Channel 3, 867.1 MHz Optional
	867300000, // Channel 4, 867.3 MHz Optional
	867500000, // Channel 5, 867.5 MHz Optional
	867700000, // Channel 6, 867.7 MHz Optional 
	867900000, // Channel 7, 867.9 MHz Optional 
	868800000, // Channel 8, 868.9 MHz/125 Optional
	869525000  // Channel 9, 869.5 MHz/125 for RX2 responses SF9(10%)
	// TTN defines an additional channel at 869.525Mhz using SF9 for class B. Not used
};

// The following 3 frequencies should be defined/used in an EU433 
// environment.
int EU433 [] = {
	433175000, // Channel 0, 433.175 MHz/125 primary
	433375000, // Channel 1, 433.375 MHz primary
	433575000, // Channel 2, 433.575 MHz primary
	433775000, // Channel 3, 433.775 MHz primary
	433975000, // Channel 4, 433.975 MHz primary
	434175000, // Channel 5, 434.175 MHz primary
	434375000, // Channel 6, 434.375 MHz primary
	434575000, // Channel 7, 434.575 MHz primary
	434775000  // Channel 8, 434.775 MHz primary
};

// US902=928
// AU915-928
int US902 [] = {
	// Uplink
	903900000, // Channel 0, SF7BW125 to SF10BW125 primary
	904100000, // Ch 1, SF7BW125 to SF10BW125
	904300000, // Ch 2, SF7BW125 to SF10BW125
	904500000, // Ch 3, SF7BW125 to SF10BW125
	904700000, // Ch 4, SF7BW125 to SF10BW125
	904900000, // Ch 5, SF7BW125 to SF10BW125
	905100000, // Ch 6, SF7BW125 to SF10BW125
	905300000, // Ch 7, SF7BW125 to SF10BW125
	904600000  // Ch 8, SF8BW500
	// Downlink
	// 923.3 - SF7BW500 to SF12BW500
	// 923.9 - SF7BW500 to SF12BW500
	// 924.5 - SF7BW500 to SF12BW500
	// 925.1 - SF7BW500 to SF12BW500
	// 925.7 - SF7BW500 to SF12BW500
	// 926.3 - SF7BW500 to SF12BW500
	// 926.9 - SF7BW500 to SF12BW500
	// 927.5 - SF7BW500 to SF12BW500
	// We should specify downlink frequencies here	// SFxxxBW500
};

int* FREQS[] = {0, 0, 0, 0, EU868, 0, 0, 0, US902};

// The state of the receiver. See Semtech Datasheet (rev 4, March 2015) page 43
// The _state is of the enum type (and should be cast when used as a number)
enum state_t { S_INIT=0, S_SCAN, S_CAD, S_RX, S_TX, S_TXDONE};

volatile state_t _state=S_INIT;
volatile uint8_t _event=0;

// rssi is measured at specific moments and reported on others
// so we need to store the current value we like to work with
uint8_t _rssi;	

bool sx1272 = true;

unsigned long nowTime=0;
unsigned long msgTime=0;
unsigned long hopTime=0;
unsigned long detTime=0;

// ----------------------------------------------------------------------------
// For ESP32/Wemos based board
// SCK  == GPIO5/ PIN5
// SS   == GPIO18/PIN18
// MISO == GPIO19/ PIN19
// MOSI == GPIO27/ PIN27
// RST  == GPIO14/ PIN14
struct pins {
	uint8_t dio0=D2;		// GPIO26 / Dio0 used for one frequency and one SF
	uint8_t dio1=D2;		// GPIO26 / Used for CAD, may or not be shared with DIO0
	uint8_t dio2=D4;		// GPI2O6 / Used for frequency hopping, don't care
	uint8_t ss=D0;			// GPIO18 / Dx. Select pin connected to GPIO18
	uint8_t rst=D4;			// GPIO0  / D3. Reset pin not used	
} pins;

// STATR contains the statictis that are kept by message. 
// Ech time a message is received or sent the statistics are updated.
// In case STATISTICS==1 we define the last MAX_STAT messages as statistics
struct stat_t {
	unsigned long tmst;						// Time since 1970 in seconds		
	unsigned long node;						// 4-byte DEVaddr (the only one known to gateway)
	uint8_t ch;								// Channel index to freqs array
	uint8_t sf;
#if RSSI==1
	int8_t		rssi;						// XXX Can be < -128
#endif
	int8_t		prssi;						// XXX Can be < -128
#if _LOCALSERVER==1
	uint8_t data[23];						// For memory purposes, only 23 chars
	uint8_t datal;							// Length of decoded message 1 char
#endif
} stat_t;


#if STATISTICS >= 1
// STATC contains the statistic that are gateway related and not per
// message. Example: Number of messages received on SF7 or number of (re) boots
// So where statr contains the statistics gathered per packet the statc
// contains general statics of the node
#if STATISTICS >= 2							// Only if we explicitely set it higher
struct stat_c {
	unsigned long sf7;						// Spreading factor 7 statistics/Count
	unsigned long sf8;						// Spreading factor 8
	unsigned long sf9;						// Spreading factor 9
	unsigned long sf10;						// Spreading factor 10
	unsigned long sf11;						// Spreading factor 11
	unsigned long sf12;						// Spreading factor 12

	// If STATISTICS is 3, we add statistics about the channel 
	// When only one changgel is used, we normally know the spread of
	// statistics, but when HOP mode is selected we migth want to add this info
#if STATISTICS >=3
	unsigned long sf7_0, sf7_1, sf7_2;
	unsigned long sf8_0, sf8_1, sf8_2;
	unsigned long sf9_0, sf9_1, sf9_2;
	unsigned long sf10_0, sf10_1, sf10_2;
	unsigned long sf11_0, sf11_1, sf11_2;
	unsigned long sf12_0, sf12_1, sf12_2;
#endif
	
	uint16_t boots;							// Number of boots
	uint16_t resets;
} stat_c;
struct stat_c statc;

#endif

// History of received uplink messages from nodes
struct stat_t statr[MAX_STAT];




#else // STATISTICS==0
struct stat_t	statr[1];					// Always have at least one element to store in
#endif

// Define the payload structure used to separate interrupt ans SPI
// processing from the loop() part
uint8_t payLoad[128];						// Payload i
struct LoraBuffer {
	uint8_t	* 	payLoad;
	uint8_t		payLength;
	uint32_t	tmst;						// in millis()
	uint8_t		sfTx;
	uint8_t		powe;
	uint32_t	fff;
	uint8_t		crc;
	uint8_t		iiq;
} LoraDown;

// Up buffer (from Lora sensor to UDP)
//

struct LoraUp {
	uint8_t		payLoad[128];
	uint8_t		payLength;
	int			prssi; 
	long		snr;
	int			rssicorr;
	uint8_t		sf;
} LoraUp;

uint32_t cp_nb_rx_rcv;							// Number of messages received by gateway
uint32_t cp_nb_rx_ok;							// Number of messages received OK
uint32_t cp_nb_rx_bad;							// Number of messages received bad
uint32_t cp_nb_rx_nocrc;						// Number of messages without CRC
uint32_t cp_up_pkt_fwd;



// ============================================================================
// Set all definitions for Gateway
// ============================================================================	
// Register definitions. These are the addresses of the TFM95, SX1276 that we 
// need to set in the program.

#define REG_FIFO                    0x00		// rw FIFO address
#define REG_OPMODE                  0x01
// Register 2 to 5 are unused for LoRa
#define REG_FRF_MSB					0x06
#define REG_FRF_MID					0x07
#define REG_FRF_LSB					0x08
#define REG_PAC                     0x09
#define REG_PARAMP                  0x0A
#define REG_LNA                     0x0C
#define REG_FIFO_ADDR_PTR           0x0D		// rw SPI interface address pointer in FIFO data buffer
#define REG_FIFO_TX_BASE_AD         0x0E		// rw write base address in FIFO data buffer for TX modulator
#define REG_FIFO_RX_BASE_AD         0x0F		// rw read base address in FIFO data buffer for RX demodulator (0x00)

#define REG_FIFO_RX_CURRENT_ADDR    0x10		// r  Address of last packet received
#define REG_IRQ_FLAGS_MASK          0x11
#define REG_IRQ_FLAGS               0x12
#define REG_RX_NB_BYTES             0x13
#define REG_PKT_SNR_VALUE			0x19
#define REG_PKT_RSSI				0x1A		// latest package
#define REG_RSSI					0x1B		// Current RSSI, section 6.4, or  5.5.5
#define REG_HOP_CHANNEL				0x1C
#define REG_MODEM_CONFIG1           0x1D
#define REG_MODEM_CONFIG2           0x1E
#define REG_SYMB_TIMEOUT_LSB  		0x1F

#define REG_PAYLOAD_LENGTH          0x22
#define REG_MAX_PAYLOAD_LENGTH 		0x23
#define REG_HOP_PERIOD              0x24
#define REG_MODEM_CONFIG3           0x26
#define REG_RSSI_WIDEBAND			0x2C

#define REG_INVERTIQ				0x33
#define REG_DET_TRESH				0x37		// SF6
#define REG_SYNC_WORD				0x39
#define REG_TEMP					0x3C

#define REG_DIO_MAPPING_1           0x40
#define REG_DIO_MAPPING_2           0x41
#define REG_VERSION	  				0x42

#define REG_PADAC					0x5A
#define REG_PADAC_SX1272			0x5A
#define REG_PADAC_SX1276			0x4D


// ----------------------------------------
// opModes
#define SX72_MODE_SLEEP             0x80
#define SX72_MODE_STANDBY           0x81
#define SX72_MODE_FSTX              0x82
#define SX72_MODE_TX                0x83		// 0x80 | 0x03
#define SX72_MODE_RX_CONTINUOS      0x85

// ----------------------------------------
// LMIC Constants for radio registers
#define OPMODE_LORA      			0x80
#define OPMODE_MASK      			0x07
#define OPMODE_SLEEP     			0x00
#define OPMODE_STANDBY   			0x01
#define OPMODE_FSTX      			0x02
#define OPMODE_TX        			0x03
#define OPMODE_FSRX      			0x04
#define OPMODE_RX        			0x05
#define OPMODE_RX_SINGLE 			0x06
#define OPMODE_CAD       			0x07



// ----------------------------------------
// LOW NOISE AMPLIFIER

#define LNA_MAX_GAIN                0x23		// Max gain 0x20 | Boost 0x03
#define LNA_OFF_GAIN                0x00
#define LNA_LOW_GAIN		    	0x20

// CONF REG
#define REG1                        0x0A
#define REG2                        0x84

// ----------------------------------------
// MC1 sx1276 RegModemConfig1
#define SX1276_MC1_BW_125           0x70
#define SX1276_MC1_BW_250           0x80
#define SX1276_MC1_BW_500           0x90
#define SX1276_MC1_CR_4_5           0x02
#define SX1276_MC1_CR_4_6           0x04
#define SX1276_MC1_CR_4_7           0x06
#define SX1276_MC1_CR_4_8           0x08
#define SX1276_MC1_IMPLICIT_HEADER_MODE_ON  0x01

#define SX72_MC1_LOW_DATA_RATE_OPTIMIZE     0x01 	// mandated for SF11 and SF12

// ----------------------------------------
// MC2 definitions
#define SX72_MC2_FSK                0x00
#define SX72_MC2_SF7                0x70		// SF7 == 0x07, so (SF7<<4) == SX7_MC2_SF7
#define SX72_MC2_SF8                0x80
#define SX72_MC2_SF9                0x90
#define SX72_MC2_SF10               0xA0
#define SX72_MC2_SF11               0xB0
#define SX72_MC2_SF12               0xC0

// ----------------------------------------
// MC3
#define SX1276_MC3_LOW_DATA_RATE_OPTIMIZE  0x08
#define SX1276_MC3_AGCAUTO                 0x04

// ----------------------------------------
// FRF
#define FRF_MSB						0xD9		// 868.1 Mhz
#define FRF_MID						0x06
#define FRF_LSB						0x66

// ----------------------------------------
// DIO function mappings           		     D0D1D2D3
#define MAP_DIO0_LORA_RXDONE   		0x00  // 00------ bit 7 and 6
#define MAP_DIO0_LORA_TXDONE   		0x40  // 01------
#define MAP_DIO0_LORA_CADDONE  		0x80  // 10------
#define MAP_DIO0_LORA_NOP   		0xC0  // 11------

#define MAP_DIO1_LORA_RXTOUT   		0x00  // --00---- bit 5 and 4
#define MAP_DIO1_LORA_FCC			0x10  // --01----
#define MAP_DIO1_LORA_CADDETECT		0x20  // --10----
#define MAP_DIO1_LORA_NOP      		0x30  // --11----

#define MAP_DIO2_LORA_FCC0      	0x00  // ----00-- bit 3 and 2
#define MAP_DIO2_LORA_FCC1      	0x04  // ----01-- bit 3 and 2
#define MAP_DIO2_LORA_FCC2      	0x08  // ----10-- bit 3 and 2
#define MAP_DIO2_LORA_NOP      		0x0C  // ----11-- bit 3 and 2

#define MAP_DIO3_LORA_CADDONE  		0x00  // ------00 bit 1 and 0
#define MAP_DIO3_LORA_HEADER		0x01  // ------01
#define MAP_DIO3_LORA_CRC			0x02  // ------10
#define MAP_DIO3_LORA_NOP      		0x03  // ------11

// FSK specific
#define MAP_DIO0_FSK_READY     		0x00  // 00------ (packet sent / payload ready)
#define MAP_DIO1_FSK_NOP       		0x30  // --11----
#define MAP_DIO2_FSK_TXNOP     		0x04  // ----01--
#define MAP_DIO2_FSK_TIMEOUT   		0x08  // ----10--

// ----------------------------------------
// Bits masking the corresponding IRQs from the radio
#define IRQ_LORA_RXTOUT_MASK 		0x80	// RXTOUT
#define IRQ_LORA_RXDONE_MASK 		0x40	// RXDONE after receiving the header and CRC, we receive payload part
#define IRQ_LORA_CRCERR_MASK 		0x20	// CRC error detected. Note that RXDONE will also be set
#define IRQ_LORA_HEADER_MASK 		0x10	// valid HEADER mask. This interrupt is first when receiving a message
#define IRQ_LORA_TXDONE_MASK 		0x08	// End of TRansmission
#define IRQ_LORA_CDDONE_MASK 		0x04	// CDDONE
#define IRQ_LORA_FHSSCH_MASK 		0x02
#define IRQ_LORA_CDDETD_MASK 		0x01	// Detect preamble channel


// ----------------------------------------
// Definitions for UDP message arriving from server
#define PROTOCOL_VERSION			0x01
#define PKT_PUSH_DATA				0x00
#define PKT_PUSH_ACK				0x01
#define PKT_PULL_DATA				0x02
#define PKT_PULL_RESP				0x03
#define PKT_PULL_ACK				0x04
#define PKT_TX_ACK                  0x05

#define MGT_RESET					0x15		// Not a LoRa Gateway Spec message
#define MGT_SET_SF					0x16
#define MGT_SET_FREQ				0x17




uint32_t eventTime = 0;					// Timing of _event to change value (or not).
uint32_t sendTime = 0;					// Time that the last message transmitted
uint32_t doneTime = 0;					// Time to expire when CDDONE takes too long
uint32_t statTime = 0;					// last time we sent a stat message to server
uint32_t pulltime = 0;					// last time we sent a pull_data request to server
//uint32_t lastTmst = 0;				// Last activity Timer




String SerialStat(LoRaModuleB* loRaModule, uint8_t intr)  {
	String stat = "I=";

	if (intr & IRQ_LORA_RXTOUT_MASK) stat += "RXTOUT ";		// 0x80
	if (intr & IRQ_LORA_RXDONE_MASK) stat += "RXDONE ";		// 0x40
	if (intr & IRQ_LORA_CRCERR_MASK) stat += "CRCERR ";		// 0x20
	if (intr & IRQ_LORA_HEADER_MASK) stat += "HEADER ";		// 0x10
	if (intr & IRQ_LORA_TXDONE_MASK) stat += "TXDONE ";		// 0x08
	if (intr & IRQ_LORA_CDDONE_MASK) stat += "CDDONE ";		// 0x04
	if (intr & IRQ_LORA_FHSSCH_MASK) stat += "FHSSCH ";		// 0x02
	if (intr & IRQ_LORA_CDDETD_MASK) stat += "CDDETD ";		// 0x01

	if (intr == 0x00) stat += "  --  ";
	
	stat += ", F=";
	stat += String(loRaModule->ch);
	stat += ", SF=";
	stat += String(loRaModule->sf);
	stat += ", E=";
	stat += String(_event);
		
	stat += ", S=";
	//Serial.print(_state);
	switch (_state) {
		case S_INIT:
			stat += "INIT ";
		break;
		case S_SCAN:
			stat += "SCAN ";
		break;
		case S_CAD:
			stat += "CAD  ";
		break;
		case S_RX:
			stat += "RX   ";
		break;
		case S_TX:
			stat += "TX   ";
		break;
		case S_TXDONE:
			stat += "TXDONE";
		break;
		default:
			stat += " -- ";
	}
	stat += ", eT=";
	stat += String( micros() - eventTime );
	stat += ", dT=";
	stat += String( micros() - doneTime );

	return stat;
}














// 1-channel LoRa Gateway for ESP8266
// Copyright (c) 2016, 2017, 2018 Maarten Westenberg version for ESP8266
// Version 5.3.3
// Date: 2018-08-25
//
// 	based on work done by Thomas Telkamp for Raspberry PI 1ch gateway
//	and many others.
//
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the MIT License
// which accompanies this distribution, and is available at
// https://opensource.org/licenses/mit-license.php
//
// NO WARRANTY OF ANY KIND IS PROVIDED
//
// Author: Maarten Westenberg (mw12554@hotmail.com)
//
// This file contains the LoRa modem specific code enabling to receive
// and transmit packages/messages.
// ========================================================================================
//
//
//
// SPI AND INTERRUPTS
// The RFM96/SX1276 communicaties with the ESP8266 by means of interrupts 
// and SPI interface. The SPI interface is bidirectional and allows both
// parties to simultaneous write and read to registers.
// Major drawback is that access is not protected for interrupt and non-
// interrupt access. This means that when a program in loop() and a program
// in interrupt do access the readregister and writeRegister() function
// at teh same time that probably an error will occur.
// Therefore it is best to Either not use interrupts AT all (like LMIC)
// or only use these functions in inteerupts and to further processing
// in the main loop() program.
//
// ============================================================================


// ----------------------------------------------------------------------------
// Mutex definitiona
//
// ----------------------------------------------------------------------------
#if MUTEX==1
	void CreateMutux(int *mutex) {
		*mutex=1;
	}

#define LIB_MUTEX 1
#if LIB_MUTEX==1
	bool GetMutex(int *mutex) {
		//noInterrupts();
		if (*mutex==1) { 
			*mutex=0; 
			//interrupts(); 
			return(true); 
		}
		//interrupts();
		return(false);
	}
#else	
	bool GetMutex(int *mutex) {

	int iOld = 1, iNew = 0;

	asm volatile (
		"rsil a15, 1\n"    // read and set interrupt level to 1
		"l32i %0, %1, 0\n" // load value of mutex
		"bne %0, %2, 1f\n" // compare with iOld, branch if not equal
		"s32i %3, %1, 0\n" // store iNew in mutex
		"1:\n"             // branch target
		"wsr.ps a15\n"     // restore program state
		"rsync\n"
		: "=&r" (iOld)
		: "r" (mutex), "r" (iOld), "r" (iNew)
		: "a15", "memory"
	);
	return (bool)iOld;
}
#endif

	void ReleaseMutex(int *mutex) {
		*mutex=1;
	}
	
#endif //MUTEX==1

void Log(LoRaModuleB* loRaModule, String text);

// ----------------------------------------------------------------------------
// Read one byte value, par addr is address
// Returns the value of register(addr)
// 
// The SS (Chip select) pin is used to make sure the RFM95 is selected
// The variable is for obvious reasons valid for read and write traffic at the
// same time. Since both read and write mean that we write to the SPI interface.
// Parameters:
//	Address: SPI address to read from. Type uint8_t
// Return:
//	Value read from address
// ----------------------------------------------------------------------------

// define the SPI settings for reading messages
SPISettings readSettings(SPISPEED, MSBFIRST, SPI_MODE0);

uint8_t readRegister(uint8_t addr) {
	SPI.beginTransaction(readSettings);				
	digitalWrite(pins.ss, LOW); // Select Receiver
	SPI.transfer(addr & 0x7F);
	uint8_t res = (uint8_t) SPI.transfer(0x00);
	digitalWrite(pins.ss, HIGH); // Unselect Receiver
	SPI.endTransaction();
	return((uint8_t) res);
}


// ----------------------------------------------------------------------------
// Write value to a register with address addr. 
// Function writes one byte at a time.
// Parameters:
//	addr: SPI address to write to
//	value: The value to write to address
// Returns:
//	<void>
// ----------------------------------------------------------------------------

// define the settings for SPI writing
SPISettings writeSettings(SPISPEED, MSBFIRST, SPI_MODE0);

void writeRegister(uint8_t addr, uint8_t value) {
	SPI.beginTransaction(writeSettings);
	digitalWrite(pins.ss, LOW); // Select Receiver
	SPI.transfer((addr | 0x80) & 0xFF);
	SPI.transfer(value & 0xFF);
	//delayMicroseconds(10);
	digitalWrite(pins.ss, HIGH);// Unselect Receiver
	SPI.endTransaction();
}


// ----------------------------------------------------------------------------
// Write a buffer to a register with address addr. 
// Function writes one byte at a time.
// Parameters:
//	addr: SPI address to write to
//	value: The value to write to address
// Returns:
//	<void>
// ----------------------------------------------------------------------------
void writeBuffer(uint8_t addr, uint8_t *buf, uint8_t len) {
	//noInterrupts(); // XXX
	SPI.beginTransaction(writeSettings);
	digitalWrite(pins.ss, LOW); // Select Receiver
	SPI.transfer((addr | 0x80) & 0xFF); // write buffer address
	for (uint8_t i=0; i<len; i++) {
		SPI.transfer(buf[i] & 0xFF);
	}
	digitalWrite(pins.ss, HIGH); // Unselect Receiver
	SPI.endTransaction();
}

// ----------------------------------------------------------------------------
//  setRate is setting rate and spreading factor and CRC etc. for transmission
//		Modem Config 1 (MC1) == 0x72 for sx1276
//		Modem Config 2 (MC2) == (CRC_ON) | (sf<<4)
//		Modem Config 3 (MC3) == 0x04 | (optional SF11/12 LOW DATA OPTIMIZE 0x08)
//		sf == SF7 default 0x07, (SF7<<4) == SX72_MC2_SF7
//		bw == 125 == 0x70
//		cr == CR4/5 == 0x02
//		CRC_ON == 0x04
// ----------------------------------------------------------------------------

void setRate(LoRaModuleB* loRaModule, uint8_t sf, uint8_t crc) {
	uint8_t mc1=0, mc2=0, mc3=0;
	if ((sf<SF7) || (sf>SF12)) {
		String sfLog = "{\"n\":\"log\",\"p\":\"setRate:: SF=" + String(sf) + "\"}";
		Log(loRaModule, sfLog);
		return;
	}

	// Set rate based on Spreading Factor etc
	if (sx1272) {
		mc1= 0x0A; // SX1276_MC1_BW_250 0x80 | SX1276_MC1_CR_4_5 0x02
		mc2= ((sf<<4) | crc) % 0xFF;
		// SX1276_MC1_BW_250 0x80 | SX1276_MC1_CR_4_5 0x02 | SX1276_MC1_IMPLICIT_HEADER_MODE_ON 0x01
		if (sf == SF11 || sf == SF12) { mc1= 0x0B; }			        
	} else { // For sx1276 chips is the CRC ON is 
		if (sf==SF8) {
			mc1= 0x78; // SX1276_MC1_BW_125==0x70 | SX1276_MC1_CR_4_8==0x08
		} else {
			mc1= 0x72; // SX1276_MC1_BW_125==0x70 | SX1276_MC1_CR_4_5==0x02
		}
		mc2= ((sf<<4) | crc) & 0xFF; // crc is 0x00 or 0x04==SX1276_MC2_RX_PAYLOAD_CRCON
		mc3= 0x04; // 0x04; SX1276_MC3_AGCAUTO
		if (sf == SF11 || sf == SF12) { mc3|= 0x08; }		// 0x08 | 0x04
	}
	
	// Implicit Header (IH), for class b beacons (&& SF6)
	//if (getIh(LMIC.rps)) {
	//   mc1 |= SX1276_MC1_IMPLICIT_HEADER_MODE_ON;
	//    writeRegister(REG_PAYLOAD_LENGTH, getIh(LMIC.rps)); // required length
	//}
	
	writeRegister(REG_MODEM_CONFIG1, (uint8_t) mc1);
	writeRegister(REG_MODEM_CONFIG2, (uint8_t) mc2);
	writeRegister(REG_MODEM_CONFIG3, (uint8_t) mc3);
	
	// Symbol timeout settings
	if (sf == SF10 || sf == SF11 || sf == SF12) {
		writeRegister(REG_SYMB_TIMEOUT_LSB, (uint8_t) 0x05);
	} else {
		writeRegister(REG_SYMB_TIMEOUT_LSB, (uint8_t) 0x08);
	}
	return;
}


// ----------------------------------------------------------------------------
// Set the frequency for our gateway
// The function has no parameter other than the freq setting used in init.
// Since we are usin a 1ch gateway this value is set fixed.
// ----------------------------------------------------------------------------

void setFreq(uint32_t freq) {
	// set frequency
	uint64_t frf = ((uint64_t)freq << 19) / 32000000;
	writeRegister(REG_FRF_MSB, (uint8_t)(frf>>16) );
	writeRegister(REG_FRF_MID, (uint8_t)(frf>> 8) );
	writeRegister(REG_FRF_LSB, (uint8_t)(frf>> 0) );
	return;
}


// ----------------------------------------------------------------------------
//	Set Power for our gateway
// ----------------------------------------------------------------------------
void setPow(uint8_t powe) {
	if (powe >= 16) powe = 15;
	//if (powe >= 15) powe = 14;
	else if (powe < 2) powe =2;
	//ASSERT((powe>=2)&&(powe<=15));
	uint8_t pac = (0x80 | (powe & 0xF)) & 0xFF;
	writeRegister(REG_PAC, (uint8_t)pac); // set 0x09 to pac
	// XXX Power settings for CFG_sx1272 are different
	return;
}


// ----------------------------------------------------------------------------
// Used to set the radio to LoRa mode (transmitter)
// Please note that this mode can only be set in SLEEP mode and not in Standby.
// Also there should be not need to re-init this mode is set in the setup() 
// function.
// For high freqs (>860 MHz) we need to & with 0x08 otherwise with 0x00
// ----------------------------------------------------------------------------

//void ICACHE_RAM_ATTR opmodeLora()
//{
//#ifdef CFG_sx1276_radio
//       uint8_t u = OPMODE_LORA | 0x80; // TBD: sx1276 high freq 
//#else // SX-1272
//	    uint8_t u = OPMODE_LORA | 0x08;
//#endif
//    writeRegister(REG_OPMODE, (uint8_t) u);
//}


// ----------------------------------------------------------------------------
// Set the opmode to a value as defined on top
// Values are 0x00 to 0x07
// The value is set for the lowest 3 bits, the other bits are as before.
// ----------------------------------------------------------------------------
void opmode(uint8_t mode) {
	if (mode == OPMODE_LORA) 
		writeRegister(REG_OPMODE, (uint8_t) mode);
	else
		writeRegister(REG_OPMODE, (uint8_t)((readRegister(REG_OPMODE) & ~OPMODE_MASK) | mode));
}

// ----------------------------------------------------------------------------
// Hop to next frequency as defined by NUM_HOPS
// This function should only be used for receiver operation. The current
// receiver frequency is determined by ifreq index like so: freqs[ifreq] 
// ----------------------------------------------------------------------------
void hop(LoRaModuleB* loRaModule) {

	// 1. Set radio to standby
	opmode(OPMODE_STANDBY);
		
	// 3. Set frequency based on value in freq		
	loRaModule->ch = (loRaModule->ch + 1) % NUM_HOPS; // Increment the freq round robin
	uint32_t freq = (uint32_t) FREQS[loRaModule->pl][loRaModule->ch];
	setFreq(freq);

	// 4. Set spreading Factor
	loRaModule->sf = SF7; // Starting the new frequency 
	setRate(loRaModule, loRaModule->sf, 0x40); // set the sf to SF7 
		
	// Low Noise Amplifier used in receiver
	writeRegister(REG_LNA, (uint8_t) LNA_MAX_GAIN); // 0x0C, 0x23
	
	// 7. set sync word
	writeRegister(REG_SYNC_WORD, (uint8_t) 0x34); // set 0x39 to 0x34 LORA_MAC_PREAMBLE
	
	// prevent node to node communication
	writeRegister(REG_INVERTIQ,0x27); // 0x33, 0x27; to reset from TX
	
	// Max Payload length is dependent on 256 byte buffer. At startup TX starts at
	// 0x80 and RX at 0x00. RX therefore maximized at 128 Bytes
	writeRegister(REG_MAX_PAYLOAD_LENGTH,MAX_PAYLOAD_LENGTH); // set 0x23 to 0x80==128 bytes
	writeRegister(REG_PAYLOAD_LENGTH,PAYLOAD_LENGTH); // 0x22, 0x40==64Byte long
	
	writeRegister(REG_FIFO_ADDR_PTR, (uint8_t) readRegister(REG_FIFO_RX_BASE_AD));	// set reg 0x0D to 0x0F
	writeRegister(REG_HOP_PERIOD,0x00); // reg 0x24, set to 0x00

	// 5. Config PA Ramp up time // set reg 0x0A  
	writeRegister(REG_PARAMP, (readRegister(REG_PARAMP) & 0xF0) | 0x08); // set PA ramp-up time 50 uSec
	
	// Set 0x4D PADAC for SX1276 ; XXX register is 0x5a for sx1272
	writeRegister(REG_PADAC_SX1276,  0x84); // set 0x4D (PADAC) to 0x84
	//writeRegister(REG_PADAC, readRegister(REG_PADAC) | 0x4);
	
	// 8. Reset interrupt Mask, enable all interrupts
	writeRegister(REG_IRQ_FLAGS_MASK, 0x00);
		
	// 9. clear all radio IRQ flags
	writeRegister(REG_IRQ_FLAGS, 0xFF);
	
	// Be aware that micros() has increased significantly from calling 
	// the hop function until printed below
	//String hopLog =  "{\"n\":\"log\",\"p\":\"hop:: hopTime:: ";
	//hopLog += String(micros() - hopTime);
	//hopLog += ", " + SerialStat(loRaModule, 0) + "\"}";
	//Log(loRaModule, hopLog);

	// Remember the last time we hop
	hopTime = micros(); // At what time did we hop
}


// ----------------------------------------------------------------------------
// This LoRa function reads a message from the LoRa transceiver
// on Success: returns message length read when message correctly received
// on Failure: it returns a negative value on error (CRC error for example).
// UP function
// This is the "lowlevel" receive function called by stateMachine()
// dealing with the radio specific LoRa functions
//
// Parameters:
//		Payload: uint8_t[] message. when message is read it is returned in payload.
// Returns:
//		Length of payload received
//
// 9 bytes header
// followed by data N bytes
// 4 bytes MIC end
// ----------------------------------------------------------------------------
uint8_t receivePkt(LoRaModuleB* loRaModule, uint8_t *payload) {
	uint8_t irqflags = readRegister(REG_IRQ_FLAGS); // 0x12; read back flags

	cp_nb_rx_rcv++; // Receive statistics counter

	uint8_t crcUsed = readRegister(REG_HOP_CHANNEL);
	if (crcUsed & 0x40) {
		// String crcUsedLog =  "{\"n\":\"log\",\"p\":\"R rxPkt:: CRC used\"}";
		// Log(loRaModule, crcUsedLog);
	}
	
	//  Check for payload IRQ_LORA_CRCERR_MASK=0x20 set
	if (irqflags & IRQ_LORA_CRCERR_MASK) {
		String errCrcLog =  "{\"n\":\"log\",\"p\":\"rxPkt:: Err CRC, =\"}"; // + SerialTime();
		Log(loRaModule, errCrcLog);
		return 0;
		// Is header OK?
		// Please note that if we reset the HEADER interrupt in RX,
		// that we would here conclude that ther eis no HEADER
	} else if ((irqflags & IRQ_LORA_HEADER_MASK) == false) {
		String errHeaderLog =  "{\"n\":\"log\",\"p\":\"rxPkt:: Err HEADER\"}";
		Log(loRaModule, errHeaderLog);
		// Reset VALID-HEADER flag 0x10
		writeRegister(REG_IRQ_FLAGS, (uint8_t)(IRQ_LORA_HEADER_MASK  | IRQ_LORA_RXDONE_MASK));	// 0x12; clear HEADER (== 0x10) flag
		return 0;
		// If there are no error messages, read the buffer from the FIFO
		// This means "Set FifoAddrPtr to FifoRxBaseAddr"
	} else {
		cp_nb_rx_ok++; // Receive OK statistics counter
		if (readRegister(REG_FIFO_RX_CURRENT_ADDR) != readRegister(REG_FIFO_RX_BASE_AD)) {
			String rxLog = "{\"n\":\"log\",\"p\":\"RX BASE <" + String(readRegister(REG_FIFO_RX_BASE_AD)) + "> != RX CURRENT <" + String(readRegister(REG_FIFO_RX_CURRENT_ADDR)) + ">\"}";
			Log(loRaModule, rxLog);
		}
		
		//uint8_t currentAddr = readRegister(REG_FIFO_RX_CURRENT_ADDR); // 0x10
		uint8_t currentAddr = readRegister(REG_FIFO_RX_BASE_AD); // 0x0F
		uint8_t receivedCount = readRegister(REG_RX_NB_BYTES); // 0x13; How many bytes were read

		if (currentAddr > 64) {
			String addrLog = "{\"n\":\"log\",\"p\":\"rxPkt:: Rx addr>64 " + String(currentAddr) + "\"}";
			Log(loRaModule, addrLog);
		}

		writeRegister(REG_FIFO_ADDR_PTR, (uint8_t) currentAddr); // 0x0D 

		if (receivedCount > PAYLOAD_LENGTH) {
			String rcvLog = "{\"n\":\"log\",\"p\":\"rxPkt:: receivedCount=" + String(receivedCount) + "\"}";
			Log(loRaModule, rcvLog);
			receivedCount = PAYLOAD_LENGTH;
		}

		for(int i=0; i < receivedCount; i++) {
			payload[i] = readRegister(REG_FIFO); // 0x00, FIFO will auto shift register
		}

		uint8_t rssi = readRegister(REG_RSSI); // Read the RSSI
		writeRegister(REG_IRQ_FLAGS, (uint8_t) 0xFF); // Reset ALL interrupts
		
		// the received packet is displayed on the output.
		String log = "{\"n\":\"rtx\",\"p\":\"RX: ch:" + String(loRaModule->ch);	
		log += ", sf:" + String(loRaModule->sf) + ", rssi:-" + String(rssi) + ", dev:";
		log += ((payload[4] < 0x10) ? "0" : "") + String(payload[4], HEX);
		log += ((payload[3] < 0x10) ? "0" : "") + String(payload[3], HEX);
		log += ((payload[2] < 0x10) ? "0" : "") + String(payload[2], HEX);
		log += ((payload[1] < 0x10) ? "0" : "") + String(payload[1], HEX);
		log += ", len:" + String(receivedCount) + ", f:" + String(irqflags, HEX);
		log += ", a:" + String(currentAddr) + "\"}";
		Log(loRaModule, log);

		return(receivedCount);
	}

	writeRegister(REG_IRQ_FLAGS, (uint8_t) (
		IRQ_LORA_RXDONE_MASK | 
		IRQ_LORA_RXTOUT_MASK |
		IRQ_LORA_HEADER_MASK | 
		IRQ_LORA_CRCERR_MASK));							// 0x12; Clear RxDone IRQ_LORA_RXDONE_MASK
    return 0;
} //receivePkt
	
	
	
// ----------------------------------------------------------------------------
// This DOWN function sends a payload to the LoRa node over the air
// Radio must go back in standby mode as soon as the transmission is finished
// 
// NOTE:: writeRegister functions should not be used outside interrupts
// ----------------------------------------------------------------------------
bool sendPkt(LoRaModuleB* loRaModule, uint8_t *payLoad, uint8_t payLength) {
	if (payLength >= 128) {
		String lenLog = "{\"n\":\"log\",\"p\":\"sendPkt:: len=" + String(payLength) + "\"}";
		Log(loRaModule, lenLog);
		return false;
	}
	writeRegister(REG_FIFO_ADDR_PTR, (uint8_t) readRegister(REG_FIFO_TX_BASE_AD)); // 0x0D, 0x0E
	writeRegister(REG_PAYLOAD_LENGTH, (uint8_t) payLength); // 0x22
	payLoad[payLength] = 0x00;
	writeBuffer(REG_FIFO, (uint8_t *) payLoad, payLength);
	return true;
}

// ----------------------------------------------------------------------------
// loraWait()
// This function implements the wait protocol needed for downstream transmissions.
// Note: Timing of downstream and JoinAccept messages is VERY critical.
//
// As the ESP8266 watchdog will not like us to wait more than a few hundred
// milliseconds (or it will kick in) we have to implement a simple way to wait
// time in case we have to wait seconds before sending messages (e.g. for OTAA 5 or 6 seconds)
// Without it, the system is known to crash in half of the cases it has to wait for 
// JOIN-ACCEPT messages to send.
//
// This function uses a combination of delay() statements and delayMicroseconds().
// As we use delay() only when there is still enough time to wait and we use micros()
// to make sure that delay() did not take too much time this works.
// 
// Parameter: uint32-t tmst gives the micros() value when transmission should start. (!!!)
// Note: We assume LoraDown.sfTx contains the SF we will use for downstream message.
// ----------------------------------------------------------------------------

void loraWait(LoRaModuleB* loRaModule, const uint32_t timestamp) {
	uint32_t startMics = micros(); // Start of the loraWait function
	uint32_t tmst = timestamp;
	// XXX
	int32_t adjust=0;
	switch (LoraDown.sfTx) {
		case 7: adjust= 60000; break; // Make time for SF7 longer 
		case 8: break; // Around 60ms
		case 9: break;
		case 10: break;
		case 11: break;
		case 12: break;
		default:
			String uSFLog = "{\"n\":\"log\",\"p\":\"T loraWait:: unknown SF=" + String(LoraDown.sfTx) + "\"}";
			Log(loRaModule, uSFLog);
			break; // to avoid compiler error when DUSB==0
	}
	tmst = tmst + txDelay + adjust; // tmst based on txDelay and spreading factor
	uint32_t waitTime = tmst - micros();
	//	if (waitTime<0) { //uint32_t is never negative! If micros() is > tmst, waitTime assume a very big value and the app hangs
	if (micros() > tmst) { // test if the tmst is in the past to avoid hangs
		String wTimeLog = "{\"n\":\"log\",\"p\":\"loraWait:: Error wait time < 0\"}";
		Log(loRaModule, wTimeLog);
		return;
	}
	
	// For larger delay times we use delay() since that is for > 15ms
	// This is the most efficient way
	while (waitTime > 16000) {
		delay(15);										// ms delay including yield, slightly shorter
		waitTime= tmst - micros();
	}
	// The remaining wait time is less tan 15000 uSecs
	// And we use delayMicroseconds() to wait
	if (waitTime>0) delayMicroseconds(waitTime);
	else if ((waitTime+20) < 0) {
		String tooLateLog = "{\"n\":\"log\",\"p\":\"loraWait:: TOO LATE\"}";
		Log(loRaModule, tooLateLog);
	}

	String waitLog = "{\"n\":\"log\",\"p\":\"T start: " + String(startMics) + ", tmst: " + String(tmst);
	waitLog += ", end: " + String(micros()) + ", waited: " + String(tmst - startMics);
	waitLog += ", delay=" + String(txDelay) + "\"}";
	Log(loRaModule, waitLog);
}


// ----------------------------------------------------------------------------
// txLoraModem
// Init the transmitter and transmit the buffer
// After successful transmission (dio0==1) TxDone re-init the receiver
//
//	crc is set to 0x00 for TX
//	iiq is set to 0x27 (or 0x40 based on ipol value in txpkt)
//
//	1. opmode Lora (only in Sleep mode)
//	2. opmode StandBY
//	3. Configure Modem
//	4. Configure Channel
//	5. write PA Ramp
//	6. config Power
//	7. RegLoRaSyncWord LORA_MAC_PREAMBLE
//	8. write REG dio mapping (dio0)
//	9. write REG IRQ flags
// 10. write REG IRQ mask
// 11. write REG LoRa Fifo Base Address
// 12. write REG LoRa Fifo Addr Ptr
// 13. write REG LoRa Payload Length
// 14. Write buffer (byte by byte)
// 15. Wait until the right time to transmit has arrived
// 16. opmode TX
// ----------------------------------------------------------------------------

void txLoraModem(LoRaModuleB* loRaModule, uint8_t *payLoad, uint8_t payLength, uint32_t tmst, uint8_t sfTx, uint8_t powe, uint32_t freq, uint8_t crc, uint8_t iiq) {
	String txLog = "{\"n\":\"trx\",\"p\":\"txLoraModem::";
	txLog += "  powe: " + String(powe);
	txLog += ", freq: " + String(freq);
	txLog += ", crc: " + String(crc);
	txLog += ", iiq: 0X" + String(iiq, HEX) + "\"}";
	Log(loRaModule, txLog);

	_state = S_TX;
		
	// 1. Select LoRa modem from sleep mode
	//opmode(OPMODE_LORA);									// set register 0x01 to 0x80
	
	// Assert the value of the current mode
	ASSERT((readRegister(REG_OPMODE) & OPMODE_LORA) != 0);
	
	// 2. enter standby mode (required for FIFO loading))
	opmode(OPMODE_STANDBY);									// set 0x01 to 0x01
	
	// 3. Init spreading factor and other Modem setting
	setRate(loRaModule, sfTx, crc);
	
	// Frquency hopping
	//writeRegister(REG_HOP_PERIOD, (uint8_t) 0x00);		// set 0x24 to 0x00 only for receivers
	
	// 4. Init Frequency, config channel
	setFreq(freq);

	// 6. Set power level, REG_PAC
	setPow(powe);
	
	// 7. prevent node to node communication
	writeRegister(REG_INVERTIQ, (uint8_t) iiq);						// 0x33, (0x27 or 0x40)
	
	// 8. set the IRQ mapping DIO0=TxDone DIO1=NOP DIO2=NOP (or lesss for 1ch gateway)
	writeRegister(REG_DIO_MAPPING_1, (uint8_t)(
		MAP_DIO0_LORA_TXDONE | 
		MAP_DIO1_LORA_NOP | 
		MAP_DIO2_LORA_NOP |
		MAP_DIO3_LORA_CRC)
	);
	
	// 9. clear all radio IRQ flags
	writeRegister(REG_IRQ_FLAGS, (uint8_t) 0xFF);
	
	// 10. mask all IRQs but TxDone
	writeRegister(REG_IRQ_FLAGS_MASK, (uint8_t) ~IRQ_LORA_TXDONE_MASK);
	
	// txLora
	opmode(OPMODE_FSTX);									// set 0x01 to 0x02 (actual value becomes 0x82)
	
	// 11, 12, 13, 14. write the buffer to the FiFo
	sendPkt(loRaModule, payLoad, payLength);
	
	// 15. wait extra delay out. The delayMicroseconds timer is accurate until 16383 uSec.
	loraWait(loRaModule, tmst);
	
	//Set the base addres of the transmit buffer in FIFO
	writeRegister(REG_FIFO_ADDR_PTR, (uint8_t) readRegister(REG_FIFO_TX_BASE_AD));	// set 0x0D to 0x0F (contains 0x80);	
	
	//For TX we have to set the PAYLOAD_LENGTH
	writeRegister(REG_PAYLOAD_LENGTH, (uint8_t) payLength);		// set 0x22, max 0x40==64Byte long
	
	//For TX we have to set the MAX_PAYLOAD_LENGTH
	writeRegister(REG_MAX_PAYLOAD_LENGTH, (uint8_t) MAX_PAYLOAD_LENGTH);	// set 0x22, max 0x40==64Byte long
	
	// Reset the IRQ register
	writeRegister(REG_IRQ_FLAGS_MASK, (uint8_t) 0x00);			// Clear the mask
	writeRegister(REG_IRQ_FLAGS, (uint8_t) IRQ_LORA_TXDONE_MASK);// set 0x12 to 0x08, clear TXDONE
	
	// 16. Initiate actual transmission of FiFo
	opmode(OPMODE_TX);											// set 0x01 to 0x03 (actual value becomes 0x83)
	
} // txLoraModem


// ----------------------------------------------------------------------------
// Setup the LoRa receiver on the connected transceiver.
// - Determine the correct transceiver type (sx1272/RFM92 or sx1276/RFM95)
// - Set the frequency to listen to (1-channel remember)
// - Set Spreading Factor (standard SF7)
// The reset RST pin might not be necessary for at least the RGM95 transceiver
//
// 1. Put the radio in LoRa mode
// 2. Put modem in sleep or in standby
// 3. Set Frequency
// 4. Spreading Factor
// 5. Set interrupt mask
// 6. Clear all interrupt flags
// 7. Set opmode to OPMODE_RX
// 8. Set _state to S_RX
// 9. Reset all interrupts
// ----------------------------------------------------------------------------

void rxLoraModem(LoRaModuleB* loRaModule) {
	// 1. Put system in LoRa mode
	//opmode(OPMODE_LORA); // Is already so
	
	// 2. Put the radio in sleep mode
	opmode(OPMODE_STANDBY); // CAD set 0x01 to 0x00
	
	// 3. Set frequency based on value in freq
	int cfreq = FREQS[loRaModule->pl][loRaModule->ch];
	setFreq(cfreq); // set to 868.1MHz

	// 4. Set spreading Factor and CRC
	setRate(loRaModule, loRaModule->sf, 0x04);
	
	// prevent node to node communication
	writeRegister(REG_INVERTIQ, (uint8_t) 0x27); // 0x33, 0x27; to reset from TX
	
	// Max Payload length is dependent on 256 byte buffer. 
	// At startup TX starts at 0x80 and RX at 0x00. RX therefore maximized at 128 Bytes
	//For TX we have to set the PAYLOAD_LENGTH
    //writeRegister(REG_PAYLOAD_LENGTH, (uint8_t) PAYLOAD_LENGTH);	// set 0x22, 0x40==64Byte long

	// Set CRC Protection or MAX payload protection
	//writeRegister(REG_MAX_PAYLOAD_LENGTH, (uint8_t) MAX_PAYLOAD_LENGTH);	// set 0x23 to 0x80==128
	
	//Set the start address for the FiFO (Which should be 0)
	writeRegister(REG_FIFO_ADDR_PTR, (uint8_t) readRegister(REG_FIFO_RX_BASE_AD));	// set 0x0D to 0x0F (contains 0x00);
	
	// Low Noise Amplifier used in receiver
	writeRegister(REG_LNA, (uint8_t) LNA_MAX_GAIN); // 0x0C, 0x23
	
	// Accept no interrupts except RXDONE, RXTOUT en RXCRC
	writeRegister(REG_IRQ_FLAGS_MASK, (uint8_t) ~(
		IRQ_LORA_RXDONE_MASK | 
		IRQ_LORA_RXTOUT_MASK | 
		IRQ_LORA_HEADER_MASK | 
		IRQ_LORA_CRCERR_MASK));

	// set frequency hopping
	if (loRaModule->hop) {
		//writeRegister(REG_HOP_PERIOD, 0x01); // 0x24, 0x01 was 0xFF
		writeRegister(REG_HOP_PERIOD,0x00); // 0x24, 0x00 was 0xFF
	} else {
		writeRegister(REG_HOP_PERIOD,0x00); // 0x24, 0x00 was 0xFF
	}
	// Set RXDONE interrupt to dio0
	writeRegister(REG_DIO_MAPPING_1, (uint8_t)(
			MAP_DIO0_LORA_RXDONE | 
			MAP_DIO1_LORA_RXTOUT |
			MAP_DIO2_LORA_NOP |			
			MAP_DIO3_LORA_CRC));

	// Set the opmode to either single or continuous receive. The first is used when
	// every message can come on a different SF, the second when we have fixed SF
	if (loRaModule->cad) {
		// cad Scanner setup, set _state to S_RX
		// Set Single Receive Mode, goes in STANDBY mode after receipt
		_state= S_RX;
		opmode(OPMODE_RX_SINGLE); // 0x80 | 0x06 (listen one message)
	} else {
		// Set Continous Receive Mode, usefull if we stay on one SF
		_state= S_RX;
		if (loRaModule->hop) {
			String contErrorLog = "{\"n\":\"log\",\"p\":\"rxLoraModem:: ERROR continuous receive in hop mode\"}";
			Log(loRaModule, contErrorLog);
		}
		opmode(OPMODE_RX); // 0x80 | 0x05 (listen)
	}
	
	// 9. clear all radio IRQ flags
	writeRegister(REG_IRQ_FLAGS, 0xFF);
	
	return;
} // rxLoraModem


// ----------------------------------------------------------------------------
// function cadScanner()
//
// CAD Scanner will scan on the given channel for a valid Symbol/Preamble signal.
// So instead of receiving continuous on a given channel/sf combination
// we will wait on the given channel and scan for a preamble. Once received
// we will set the radio to the SF with best rssi (indicating reception on that sf).
// The function sets the _state to S_SCAN
// NOTE: DO not set the frequency here but use the frequency hopper
// ----------------------------------------------------------------------------
void cadScanner(LoRaModuleB* loRaModule) {
	// 1. Put system in LoRa mode (which destroys all other nodes(
	//opmode(OPMODE_LORA);
	
	// 2. Put the radio in sleep mode
	opmode(OPMODE_STANDBY); // Was old value
	
	// 3. Set frequency based on value in ifreq // XXX New, might be needed when receiving down
	int cfreq = FREQS[loRaModule->pl][loRaModule->ch];
	setFreq(cfreq);

	// For every time we start the scanner, we set the SF to the begin value
	//sf = SF7; // XXX 180501 Not by default
	
	// 4. Set spreading Factor and CRC
	setRate(loRaModule, loRaModule->sf, 0x04);
	
	// listen to LORA_MAC_PREAMBLE
	writeRegister(REG_SYNC_WORD, (uint8_t) 0x34); // set reg 0x39 to 0x34
	
	// Set the interrupts we want to listen to
	writeRegister(REG_DIO_MAPPING_1, (uint8_t)(
		MAP_DIO0_LORA_CADDONE | 
		MAP_DIO1_LORA_CADDETECT | 
		MAP_DIO2_LORA_NOP | 
		MAP_DIO3_LORA_CRC ));
	
	// Set the mask for interrupts (we do not want to listen to) except for
	writeRegister(REG_IRQ_FLAGS_MASK, (uint8_t) ~(
		IRQ_LORA_CDDONE_MASK | 
		IRQ_LORA_CDDETD_MASK | 
		IRQ_LORA_CRCERR_MASK | 
		IRQ_LORA_HEADER_MASK));
	
	// Set the opMode to CAD
	opmode(OPMODE_CAD);

	// Clear all relevant interrupts
	//writeRegister(REG_IRQ_FLAGS, (uint8_t) 0xFF );						// May work better, clear ALL interrupts
	
	// If we are here. we either might have set the SF or we have a timeout in which
	// case the receive is started just as normal.
	return;
	
}// cadScanner


// ----------------------------------------------------------------------------
// First time initialisation of the LoRa modem
// Subsequent changes to the modem state etc. done by txLoraModem or rxLoraModem
// After initialisation the modem is put in rx mode (listen)
// ----------------------------------------------------------------------------
void initLoraModem(LoRaModuleB* loRaModule) {
	_state = S_INIT;

	#if ESP32_ARCH==1
		digitalWrite(pins.rst, LOW);
		delayMicroseconds(10000);
		digitalWrite(pins.rst, HIGH);
		delayMicroseconds(10000);
		digitalWrite(pins.ss, HIGH);
	#else
		// Reset the transceiver chip with a pulse of 10 mSec
		digitalWrite(pins.rst, HIGH);
		delayMicroseconds(10000);
		digitalWrite(pins.rst, LOW);
		delayMicroseconds(10000);
	#endif

	// 2. Set radio to sleep
	opmode(OPMODE_SLEEP); // set register 0x01 to 0x00

	// 1 Set LoRa Mode
	opmode(OPMODE_LORA); // set register 0x01 to 0x80
	
	// 3. Set frequency based on value in freq
	//ifreq = 0; // XXX 180326
	uint32_t freq = (uint32_t) FREQS[loRaModule->pl][loRaModule->ch];
	setFreq(freq); // set to 868.1MHz or the last saved frequency
	
	// 4. Set spreading Factor
	setRate(loRaModule, loRaModule->sf, 0x04);
	
	// Low Noise Amplifier used in receiver
	writeRegister(REG_LNA, (uint8_t) LNA_MAX_GAIN); // 0x0C, 0x23
	uint8_t version = readRegister(REG_VERSION); // Read the LoRa chip version id
	if (version == 0x22) {
        // sx1272
        Serial.println(F("WARNING:: SX1272 detected"));
        sx1272 = true;
    } else if (version == 0x12) {
		// sx1276?
		// Serial.println(F("SX1276 starting"));
		sx1272 = false;
	} else {
		// Normally this means that we connected the wrong type of board and
		// therefore specified the wrong type of wiring/pins to the software
		// Maybe this issue can be resolved of we try one of the other defined 
		// boards. (Comresult or Hallard or ...)
		Serial.print(F("Unknown transceiver="));
		Serial.print(version,HEX);
		Serial.print(F(", pins.rst =")); Serial.print(pins.rst);
		Serial.print(F(", pins.ss  =")); Serial.print(pins.ss);
		Serial.print(F(", pins.dio0 =")); Serial.print(pins.dio0);
		Serial.print(F(", pins.dio1 =")); Serial.print(pins.dio1);
		Serial.print(F(", pins.dio2 =")); Serial.print(pins.dio2);
		Serial.println();
		Serial.flush();
	}
	// If we are here, the chip is recognized successfully
	
	// 7. set sync word
	writeRegister(REG_SYNC_WORD, (uint8_t) 0x34);				// set 0x39 to 0x34 LORA_MAC_PREAMBLE
	
	// prevent node to node communication
	writeRegister(REG_INVERTIQ,0x27);							// 0x33, 0x27; to reset from TX
	
	// Max Payload length is dependent on 256 byte buffer. At startup TX starts at
	// 0x80 and RX at 0x00. RX therefore maximized at 128 Bytes
	writeRegister(REG_MAX_PAYLOAD_LENGTH,MAX_PAYLOAD_LENGTH);	// set 0x23 to 0x80==128 bytes
	writeRegister(REG_PAYLOAD_LENGTH,PAYLOAD_LENGTH);			// 0x22, 0x40==64Byte long
	
	writeRegister(REG_FIFO_ADDR_PTR, (uint8_t) readRegister(REG_FIFO_RX_BASE_AD));	// set reg 0x0D to 0x0F
	writeRegister(REG_HOP_PERIOD,0x00);							// reg 0x24, set to 0x00

	// 5. Config PA Ramp up time								// set reg 0x0A  
	writeRegister(REG_PARAMP, (readRegister(REG_PARAMP) & 0xF0) | 0x08); // set PA ramp-up time 50 uSec
	
	// Set 0x4D PADAC for SX1276 ; XXX register is 0x5a for sx1272
	writeRegister(REG_PADAC_SX1276,  0x84); 					// set 0x4D (PADAC) to 0x84
	//writeRegister(REG_PADAC, readRegister(REG_PADAC) | 0x4);
	
	// Reset interrupt Mask, enable all interrupts
	writeRegister(REG_IRQ_FLAGS_MASK, 0x00);
	
	// 9. clear all radio IRQ flags
    writeRegister(REG_IRQ_FLAGS, 0xFF);
} // initLoraModem


// ----------------------------------------------------------------------------
// Void function startReceiver.
// This function starts the receiver loop of the LoRa service.
// It starts the LoRa modem with initLoraModem(), and then starts
// the receiver either in single message (CAD) of in continuous
// reception (STD).
// ----------------------------------------------------------------------------
void startReceiver(LoRaModuleB* loRaModule) {
	initLoraModem(loRaModule); // XXX 180326, after adapting this function 
	if (loRaModule->cad) {
		String pullLog = "{\"n\":\"log\",\"p\":\"S PULL:: _state set to S_SCAN\"}";
		Log(loRaModule, pullLog);
		_state = S_SCAN;
		loRaModule->sf = SF7;
		cadScanner(loRaModule);
	} else {
		_state = S_RX;
		rxLoraModem(loRaModule);
	}
	writeRegister(REG_IRQ_FLAGS_MASK, (uint8_t) 0x00);
	writeRegister(REG_IRQ_FLAGS, 0xFF); // Reset all interrupt flags
}


// ----------------------------------------------------------------------------
// Interrupt_0 Handler.
// Both interrupts DIO0 and DIO1 are mapped on GPIO15. Se we have to look at 
// the interrupt flags to see which interrupt(s) are called.
//
// NOTE:: This method may work not as good as just using more GPIO pins on 
//  the ESP8266 mcu. But in practice it works good enough
// ----------------------------------------------------------------------------
void ICACHE_RAM_ATTR Interrupt_0() {
	_event=1;
}


// ----------------------------------------------------------------------------
// Interrupt handler for DIO1 having High Value
// As DIO0 and DIO1 may be multiplexed on one GPIO interrupt handler
// (as we do) we have to be careful only to call the right Interrupt_x
// handler and clear the corresponding interrupts for that dio.
// NOTE: Make sure all Serial communication is only for debug level 3 and up.
// Handler for:
//		- CDDETD
//		- RXTIMEOUT
//		- (RXDONE error only)
// ----------------------------------------------------------------------------
void ICACHE_RAM_ATTR Interrupt_1() {
	_event = 1;
}

// ----------------------------------------------------------------------------
// Frequency Hopping Channel (FHSS) dio2
// ----------------------------------------------------------------------------
void ICACHE_RAM_ATTR Interrupt_2() {
	_event = 1;
}
