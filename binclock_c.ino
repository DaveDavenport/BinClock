/**
 * MIT/X11 License
 * 
 * Copyright (c) 2014 Dave Davenport <qball@gmpclient.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

// Temperature sensor.
#include <OneWire.h>
#include <DallasTemperature.h>

// Header file for TimerOne library
#include <TimerOne.h>

// Temperature
const int one_wire_ping = 13;
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(one_wire_ping);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// Tick interval
#define TIMER_US 1000000

/***** GLOBALS ******/
/** Time bytes. */
byte    hour = 11;
byte  minute = 22;
byte  second = 0;

/** Dimming */
byte ldelay  = 200;
byte ddelay  = 10;


const int temp_pin = 2;

/** ES behavior */
//#define CALIBRATE
// Baudrate.. 115200 is to big for int? why this work?
const unsigned int serial_speed = 115200U;

/**
 * Function definitions.
 */
void set_out(byte index);
void disable_leds();
void display_temperature();

/**
 * Remapping table, so we can hook up led in any order direction.
 * It is easier to do this in software.
 */
byte remap[16] = {
    /* hour */
    2,3,4,5,
    /* minutes */
    0,1,6,7,14,15,
    /* seconds */
    9,8,11,10,13,12
};

/**
 * Set the bit in the bitvector according to:
 * [hour:3][minutes:6][seconds:6]
 */
inline void ld_out(byte index)
{
    set_out(remap[index]);
}

/**
 * Set all pins to floating.
 * This is done by setting them as input and with no pull-up resistor.
 */
void disable_leds()
{
    // Set pin 6 & 12 to input
    DDRD=DDRD&0x3F;
    // Disable pull-ups
    PORTD=PORTD&0x3F;
    // set 8,9,10,11 as input
    DDRB=DDRB&0x0F;
    // Disable pull-ups
    PORTB=PORTB&0x0F;
}

/**
 * Test loop LEDS
 */
void leds_test()
{
    disable_leds();
    byte i = 0;
    do {
        disable_leds();
        ld_out(i);
        delay(200);
        i++;
    }while(i < 16);

    do {
        disable_leds();
        ld_out(i-1);
        i--;
        delay(125);
    }while(i >  0);
}


/**
 * Setup of this ES
 */
void setup()
{
    // Enable serial (used for setting time).
    Serial.begin(serial_speed);

    /**
     * Loop for checking out how LEDS are connected
     */
#ifdef CALIBRATE
    while(1){
        if(Serial.available())
        {
            byte a = Serial.read();
            disable_leds();
            set_out(a-'0');
            int t = (a-'0');
            Serial.print(t);
            Serial.write('\n');
            a = Serial.read();

        }
    }
#endif
    /**
     * 1 second timers.
     */
    Timer1.initialize(TIMER_US);                  // Initialise timer 1
    Timer1.attachInterrupt( timerIsr );           // attach the ISR routine here

    // Start sensors
    sensors.begin();
    sensors.setResolution(12);

    // A small loop going up and down the LEDs in order.
    leds_test();

    // Disable all leds
    disable_leds();
    
    pinMode(temp_pin, INPUT_PULLUP);
}

/**
 * Enable LED t.
 * REMEMBER that all leds should be off, before calling this.
 * Or in the same ban & subbank.
 */
void set_out(byte t)
{
    byte index = (t&6);
    index = index>>1;
    index+=4;

    // Bank select. Top bottom
    if((t&8) >0) {
        // Enable Bank 1 as output
        DDRD|=0x0040;
        // Normal or Inverse subbank
        if((t&1)==0) {
            PORTD=PORTD|(0x0040);
        }
        else {
            PORTD=PORTD&~(0x0040);
        }
    }
    else {
        // Bank 2 as output
        DDRD|=0x0080;
        // Normal or Inverse subbank
        if((t&1)==0) {
            PORTD=PORTD|(0x0080);
        }
        else {
            PORTD=PORTD&~(0x0080);
        }
    }

    // Set the right column as output
    DDRB|=(1<<index);
    // Make column either high or low, depending on normal/inverse bank
    if((t&1) == 0) {
        PORTB&=(~(1<<index));
    }
    else{
        PORTB|=((1<<index));
    }

}
void nightrider(byte l)
{
    volatile byte iter;
    disable_leds();
    do {
        for(byte a = 0; a < 4; a++) {
            ld_out(a);
            delay(100);
            disable_leds();
        }
        for(byte a = 4; a ; a--) {
            ld_out(a-1);
            delay(100);
            disable_leds();
        }
    }while(--l);
}
void display_temperature()
{
    volatile byte iter;
    disable_leds();
    // Request temperature
    sensors.setWaitForConversion(false);  // makes it async
    sensors.requestTemperatures();
    // Disable LEDs


    nightrider(2);

    sensors.setWaitForConversion(true);  // makes it async
    // Show temp.
    float value = sensors.getTempCByIndex(0);
    Serial.println(value);
    byte temp = value;
    byte ltemp = value*10;
    ltemp%=10;
    for(int l = 0; l < 2600 ; l++) {
        for(byte a= 0; a < 4; a++) {
            disable_leds();
            for(iter=ddelay;iter; iter--);
            ld_out(a);
            for(iter=ldelay;iter; iter--);
        }

        for(byte a= 0; a < 6; a++) {
            disable_leds();
            for(iter=ddelay;iter; iter--);
            byte index = 1 << a;
            if(temp&index) {
                ld_out(a+4);
            }
            for(iter=ldelay;iter; iter--);
        }
        for(byte a= 0; a < 6; a++) {
            disable_leds();
            for(iter=ddelay;iter; iter--);
            byte index = 1 << a;
            if(ltemp&index) {
                ld_out(a+10);
            }
            for(iter=ldelay;iter; iter--);
        }
    }
}

void handle_uart_commands()
{
    byte t = Serial.read();
    if ( t == 'l' ) {
        while(!Serial.available());
        ldelay = Serial.read();
    } else if ( t == 'd' ) {
        while(!Serial.available());
        ddelay = Serial.read();
    } else if ( t == 't' ) {
        display_temperature();
    } else if ( t == 'x' ) {
        leds_test();
    }
    else if ( t == 'a' ) {
        for(int l = 0; l < 20000;l++) {
            for(byte a= 0; a < 16;a++) {
                disable_leds();
                for(volatile byte iter=ddelay;iter; iter--);
                ld_out(a);
                for(volatile byte iter=ldelay;iter; iter--);
            }
        }
        disable_leds();
    }
    else if (t == 's')
    {
        // Processes left to right.

        // Read 1st digit of hour
        while(!Serial.available());
        t = Serial.read();
        hour = (t-'0')*10;
        // Read 2nd digit of hour
        while(!Serial.available());
        t = Serial.read();
        hour += (t-'0');
        // Read 1st digit minutes
        while(!Serial.available());
        t = Serial.read();
        minute = (t-'0')*10;
        // Read 2nd digit minutes
        while(!Serial.available());
        t = Serial.read();
        minute += (t-'0');

        // Read 1st digit seconds
        while(!Serial.available());
        t = Serial.read();
        second = (t-'0')*10;
        // Read 2nd digit seconds
        while(!Serial.available());
        t = Serial.read();
        second += (t-'0');
    }
    while(Serial.available()) Serial.read();
}


void loop()
{
    volatile byte iter;
    // Hour
    for(byte a= 0; a < 4; a++) {
        disable_leds();
        for(iter=ddelay;iter; iter--);
        byte index = 1 << a;
        if(hour&index) {
            ld_out(a);
        }
        for(iter=ldelay;iter; iter--);
    }
    //Minutes
    for(byte a= 0; a < 6; a++) {
        disable_leds();
        for(iter=ddelay;iter; iter--);
        byte index = 1 << a;
        if(minute&index) {
            ld_out(a+4);
        }
        for(iter=ldelay;iter;iter--);
    }
    //Seconds
    for(byte a= 0; a < 6; a++) {
        disable_leds();
        for(iter=ddelay;iter; iter--);
        byte index = 1 << a;
        if(second&index) {
            ld_out(a+10);
        }
        for(iter=ldelay;iter;iter--);

    }
    if(digitalRead(temp_pin) == LOW) {
       display_temperature(); 
    }
    if(Serial.available()){
        handle_uart_commands();
    }

}

/**
 * Timer interrupt routine
 *
 * Seconds: 0-59
 * Minutes: 0-59
 * Hour:    1-12
 */
void timerIsr()
{
    second++;

    if(second > 59 )
    {
        second=0;
        minute++;
        if(minute > 59) {
            hour++;
            minute=0;

            if(hour > 12) {
                hour = 1;
            }
        }
    }
}

