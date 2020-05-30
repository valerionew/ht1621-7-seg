/*******************************************************************************
Copyright 2016-2018 anxzhu (github.com/anxzhu)
Copyright 2018-2020 Valerio Nappi (github.com/5N44P) (changes)
Based on segment-lcd-with-ht1621 from anxzhu (2016-2018)
(https://github.com/anxzhu/segment-lcd-with-ht1621)

Partially rewritten and extended by Valerio Nappi (github.com/5N44P) in 2018

This file is part of the HT1621 arduino library, and thus under the MIT license.
More info on the project and the license conditions on :
https://github.com/5N44P/ht1621-7-seg

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/

#include <Arduino.h>
#include "HT1621.h"

HT1621::HT1621(){
		_buffer[0] = 0x00;
		_buffer[1] = 0x00;
		_buffer[2] = 0x00;
		_buffer[3] = 0x00;
		_buffer[4] = 0x00;
		_buffer[5] = 0x00;
		_buffer[6] = 0x00;
}

void HT1621::begin(int cs_p,int wr_p,int data_p,int backlight_p)
{
	pinMode(cs_p, OUTPUT);
	pinMode(wr_p, OUTPUT);
	pinMode(data_p, OUTPUT);
	pinMode(backlight_p, OUTPUT);
	_cs_p=cs_p;
	_wr_p=wr_p;
	_data_p=data_p;
	_backlight_p=backlight_p;
	_backlight_en=true;
	config();

}

void HT1621::begin(int cs_p,int wr_p,int data_p)
{
	pinMode(cs_p, OUTPUT);
	pinMode(wr_p, OUTPUT);
	pinMode(data_p, OUTPUT);
	_cs_p=cs_p;
	_wr_p=wr_p;
	_data_p=data_p;
	_backlight_en = false;
	config();
}

void HT1621::wrDATA(unsigned char data, unsigned char cnt) {
	unsigned char i;
	for (i = 0; i < cnt; i++) {
		digitalWrite(_wr_p, LOW);
		delayMicroseconds(4);
		if (data & 0x80) {
			digitalWrite(_data_p, HIGH);
		}
		else
		{
			digitalWrite(_data_p, LOW);
		}
		digitalWrite(_wr_p, HIGH);
		delayMicroseconds(4);
		data <<= 1;
	}
}
void HT1621::wrclrdata(unsigned char addr, unsigned char sdata)
{
	addr <<= 2;
	digitalWrite(_cs_p, LOW);
	wrDATA(0xa0, 3);
	wrDATA(addr, 6);
	wrDATA(sdata, 8);
	digitalWrite(_cs_p, HIGH);
}

void HT1621::display()
{
	wrCMD(LCDON);
}

void HT1621::noDisplay()
{
	wrCMD(LCDOFF);
}

void HT1621::wrone(unsigned char addr, unsigned char sdata)
{
	addr <<= 2;
	digitalWrite(_cs_p, LOW);
	wrDATA(0xa0, 3);
	wrDATA(addr, 6);
	wrDATA(sdata, 8);
	digitalWrite(_cs_p, HIGH);
}
void HT1621::backlight()
{
	if (_backlight_en)
		digitalWrite(_backlight_p, HIGH);
	delay(1);
}
void HT1621::noBacklight()
{
	if(_backlight_en)
		digitalWrite(_backlight_p, LOW);
	delay(1);
}
void HT1621::wrCMD(unsigned char CMD) {  //100
	digitalWrite(_cs_p, LOW);
	wrDATA(0x80, 4);
	wrDATA(CMD, 8);
	digitalWrite(_cs_p, HIGH);
}
void HT1621::config()
{
	wrCMD(BIAS);
	wrCMD(RC256);
	wrCMD(SYSDIS);
	wrCMD(WDTDIS1);
	wrCMD(SYSEN);
	wrCMD(LCDON);

}
void HT1621::wrCLR(unsigned char len) {
	unsigned char addr = 0;
	unsigned char i;
	for (i = 0; i < len; i++) {
		wrclrdata(addr, 0x00);
		addr = addr + 2;
	}
}
void HT1621::setBatteryLevel(int level) {
	// zero out the previous (otherwise the or couldn't be possible)
		_buffer[0] &= 0x7F;
		_buffer[1] &= 0x7F;
		_buffer[2] &= 0x7F;

	switch(level){
		case 3: // battery on and all 3 segments
			_buffer[0] |= 0x80;
		case 2: // battery on and 2 segments
			_buffer[1] |= 0x80;
		case 1: // battery on and 1 segment
			_buffer[2] |= 0x80;
		case 0: // battery indication off
		default:
			break;
	}

	update();
}


void HT1621::clear(){

	wrCLR(16);

}

void HT1621::update(){ // takes the buffer and puts it straight into the driver
		// the buffer is backwards with respect to the lcd. could be improved
		wrone(0, _buffer[5]);
		wrone(2, _buffer[4]);
		wrone(4, _buffer[3]);
		wrone(6, _buffer[2]);
		wrone(8, _buffer[1]);
		wrone(10,_buffer[0]);

}


void HT1621::print(long num, char*flags="%6li",int precision = 0){
	if(num > 999999) // basic checks
		num = 999999; // clip into 999999
	if(num < -99999) // basic checks
		num = -99999; // clip into -99999

	char localbuffer[7]; //buffer to work within the function
	snprintf(localbuffer,7, flags, num); // convert the decimal into string

	// horrible handling but should get us working. needs refactor in next major
  if(precision > 0 && num < pow(10, precision)) {
    for(int i = 0; i<(5-precision); i++){
      localbuffer[i] = ' ';
    }
  }

	for(int i=0; i<6; i++){
		_buffer[i] &= 0x80; // mask the first bit, used by batter and decimal point
		switch(localbuffer[i]){ // map the digits to the seg bits
			case '0':
				_buffer[i] |= 0x7D;
				break;
			case '1':
				_buffer[i] |= 0x60;
				break;
			case '2':
				_buffer[i] |= 0x3e;
				break;
			case '3':
				_buffer[i] |= 0x7a;
				break;
			case '4':
				_buffer[i] |= 0x63;
				break;
			case '5':
				_buffer[i] |= 0x5b;
				break;
			case '6':
				_buffer[i] |= 0x5f;
				break;
			case '7':
				_buffer[i] |= 0x70;
				break;
			case '8':
				_buffer[i] |= 0x7f;
				break;
			case '9':
				_buffer[i] |= 0x7b;
				break;
			case '-':
				_buffer[i] |= 0x02;
				break;
			default: // do nothing, blank digit!
				break;
			}
		}

		update();

}

void HT1621::print(float num){
	// could be smarter and actually check how many
	// non zeros we have in the decimals
	print(num, 3);
}

void HT1621::print(float num, int precision){
	if(num > 999999) // basic checks
		num = 999999; // clip into 999999
	if(num < -99999) // basic checks
		num = -99999; // clip into -99999
	if(precision > 3 && num > 0)
		precision = 3; // if positive max precision allowed = 3
	else if(precision > 2 && num < 0)
		precision = 2;// if negative max precision allowed = 2
	if(precision < 0)
		precision = 0; // negative precision?!

	char * flags = "%6li";

	if(precision > 0 && num < 1){
		flags = "%06li";
	}

	long ingegerpart;
	ingegerpart = ((long)(num*pow(10,precision)));

	print(ingegerpart,flags); // draw the integerized number
	setdecimalseparator(precision); // draw the decimal point


	update();
}

void HT1621::setdecimalseparator(int decimaldigits) {
	 // zero out the eight bit
		_buffer[3] &= 0x7F;
		_buffer[4] &= 0x7F;
		_buffer[5] &= 0x7F;

	if( decimaldigits <= 0 || decimaldigits > 3){
		return;
	}

	// 3 is the digit offset
	// the first three eights bits in the buffer are for the battery signs
	// the last three are for the decimal point
	_buffer[6-decimaldigits] |= 0x80;

	}
