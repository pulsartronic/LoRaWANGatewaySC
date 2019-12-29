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
// This file contains the utilities for time and other functions
// ========================================================================================

// ----------------------------------------------------------------------------
// Fill a HEXadecimal String  from a 4-byte char array
//
// ----------------------------------------------------------------------------
static void printHEX(char * hexa, const char sep, String& response) 
{
	char m;
	m = hexa[0]; if (m<016) response+='0'; response += String(m, HEX);  response+=sep;
	m = hexa[1]; if (m<016) response+='0'; response += String(m, HEX);  response+=sep;
	m = hexa[2]; if (m<016) response+='0'; response += String(m, HEX);  response+=sep;
	m = hexa[3]; if (m<016) response+='0'; response += String(m, HEX);  response+=sep;
}

// ----------------------------------------------------------------------------
// stringTime
// Print the time t into the String reponse. t is of type time_t in seconds.
// Only when RTC is present we print real time values
// t contains number of seconds since system started that the event happened.
// So a value of 100 would mean that the event took place 1 minute and 40 seconds ago
// ----------------------------------------------------------------------------
static void stringTime(time_t t, String& response) {

	if (t==0) { response += "--"; return; }
	
	// now() gives seconds since 1970
	// as millis() does rotate every 50 days
	// So we need another timing parameter
	time_t eTime = t;
	
	// Rest is standard
	byte _hour   = hour(eTime);
	byte _minute = minute(eTime);
	byte _second = second(eTime);
	
	switch(weekday(eTime)) {
		case 1: response += "Sunday "; break;
		case 2: response += "Monday "; break;
		case 3: response += "Tuesday "; break;
		case 4: response += "Wednesday "; break;
		case 5: response += "Thursday "; break;
		case 6: response += "Friday "; break;
		case 7: response += "Saturday "; break;
	}
	response += String() + day(eTime) + "-";
	response += String() + month(eTime) + "-";
	response += String() + year(eTime) + " ";
	
	if (_hour < 10) response += "0";
	response += String() + _hour + ":";
	if (_minute < 10) response += "0";
	response += String() + _minute + ":";
	if (_second < 10) response += "0";
	response += String() + _second;
}


// ----------------------------------------------------------------------------
// SerialTime
// Print the current time on the Serial (USB), with leading 0.
// ----------------------------------------------------------------------------
void SerialTime()  {
	uint32_t thrs = hour();
	uint32_t tmin = minute();
	uint32_t tsec = second();
			
	if (thrs<10) Serial.print('0'); Serial.print(thrs);
	Serial.print(':');
	if (tmin<10) Serial.print('0'); Serial.print(tmin);
	Serial.print(':');
	if (tsec<10) Serial.print('0'); Serial.print(tsec);
			
	if (debug>=2) Serial.flush();
}

// ----------------------------------------------------------------------------
// SerialStat
// Print the statistics on Serial (USB) port
// ----------------------------------------------------------------------------

// ----------------------------------------
// Bits masking the corresponding IRQs from the radio


	
// ----------------------------------------------------------------------------
// SerialName(id, response)
// Check whether for address a (4 bytes in Unsigned Long) there is a name
// This function only works if _TRUSTED_NODES is set
// ----------------------------------------------------------------------------

int SerialName(char * a, String& response)
{
#if _TRUSTED_NODES>=1
	uint32_t id = ((a[0]<<24) | (a[1]<<16) | (a[2]<<8) | a[3]);

	int i;
	for ( i=0; i< (sizeof(nodes)/sizeof(nodex)); i++) {

		if (id == nodes[i].id) {
#if DUSB >=1
			if (( debug>=3 ) && ( pdebug & P_GUI )) {
				Serial.print(F("G Name="));
				Serial.print(nodes[i].nm);
				Serial.print(F(" for node=0x"));
				Serial.print(nodes[i].id,HEX);
				Serial.println();
			}
#endif
			response += nodes[i].nm;
			return(i);
		}
	}

#endif
	return(-1);									// If no success OR is TRUSTED NODES not defined
}

#if _LOCALSERVER==1
// ----------------------------------------------------------------------------
// inDecodes(id)
// Find the id in Decodes array, and return the index of the item
// Parameters:
//		id: The first field in the array (normally DevAddr id). Must be char[4]
// Returns:
//		The index of the ID in the Array. Returns -1 if not found
// ----------------------------------------------------------------------------
int inDecodes(char * id) {
	uint32_t ident = ((id[3]<<24) | (id[2]<<16) | (id[1]<<8) | id[0]);
	int i;
	for ( i=0; i< (sizeof(decodes)/sizeof(codex)); i++) {
		if (ident == decodes[i].id) {
			return(i);
		}
	}
	return(-1);
}
#endif


// ----------------------------------------------------------------------------
// Convert a float to string for printing
// Parameters:
//	f is float value to convert
//	p is precision in decimal digits
//	val is character array for results
// ----------------------------------------------------------------------------
void ftoa(float f, char *val, int p) {
	int j=1;
	int ival, fval;
	char b[7] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	
	for (int i=0; i< p; i++) { j= j*10; }

	ival = (int) f;								// Make integer part
	fval = (int) ((f- ival)*j);					// Make fraction. Has same sign as integer part
	if (fval<0) fval = -fval;					// So if it is negative make fraction positive again.
												// sprintf does NOT fit in memory
	if ((f<0) && (ival == 0)) strcat(val, "-");
	strcat(val,itoa(ival,b,10));				// Copy integer part first, base 10, null terminated
	strcat(val,".");							// Copy decimal point
	
	itoa(fval,b,10);							// Copy fraction part base 10
	for (int i=0; i<(p-strlen(b)); i++) {
		strcat(val,"0"); 						// first number of 0 of faction?
	}
	
	// Fraction can be anything from 0 to 10^p , so can have less digits
	strcat(val,b);
}

// ----------------------------------------------------------------------------
// Print leading '0' digits for hours(0) and second(0) when
// printing values less than 10
// ----------------------------------------------------------------------------
void printDigits(unsigned long digits) {
    // utility function for digital clock display: prints leading 0
    if(digits < 10)
        Serial.print(F("0"));
    Serial.print(digits);
}

// ----------------------------------------------------------------------------
// Print utin8_t values in HEX with leading 0 when necessary
// ----------------------------------------------------------------------------
void printHexDigit(uint8_t digit) {
    // utility function for printing Hex Values with leading 0
    if(digit < 0x10)
        Serial.print('0');
    Serial.print(digit,HEX);
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

String btostr(byte b) {
  String str = "";
  byte bs[]  = {b >> 4, b & 0b00001111};
  for (int i = 0; i < 2; i++) {
    str += String(bs[i], 16);
  }
  return str;
}

