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

// Bitmask with messages.
byte messages = 0;

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
void leds_pwm(byte on, byte a);


#define NUM_ALARMS 8

byte alarm_enable             = 0x00;
byte alarm_acknowledge        = 0x00;
byte alarm_hour[NUM_ALARMS]   = { 11, 23, 23, 23, 23, 23, 23, 23 };
byte alarm_minute[NUM_ALARMS] = { 10,  0,  0,  0,  0,  0,  0,  0 };

/**
 * Returns 1 if an alarm is active.
 * An alarm is active for 1 minute.
 */
byte alarm_active()
{
    for(byte i = 0; i < NUM_ALARMS; i++) {
        byte index = (1 << i);
        if( (alarm_enable&index) > 0 && (alarm_acknowledge&index) == 0 ) {
            if(alarm_hour[i] == hour ) {
                if ( alarm_minute[i] == minute ) {
                    return 1;
                }
            }
        }
    }
    return 0;
}
/**
 * Reset Acknowledgments 
 */
void alarm_ack_reset()
{
    alarm_acknowledge = 0x00;
}

void alarm_ack()
{
    byte retv = 0;
    for(byte i = 0; i < NUM_ALARMS; i++) {
        byte index = (1 << i);
        if( alarm_enable&(1<<i) && (alarm_acknowledge&index) == 0 ) {
            if(alarm_hour[i] == hour ) {
                if ( alarm_minute[i] == minute ) {
                    alarm_acknowledge |= index;
                }
            }
        }
    }
}

inline void buzzer_set (bool a)
{
    if(a) {
        PORTD |= (1<<4);
    }else{
        PORTD &= (~(1<<4));
    }
}

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
 * Set all pins to floating.
 * This is done by setting them as input and with no pull-up resistor.
 */
void disable_leds()
{
    // Set pin 6 & 12 to input
    DDRD=DDRD&0xFF3F;
    // Disable pull-ups
    PORTD=PORTD&0xFF3F;
    // set 8,9,10,11 as input
    DDRB=DDRB&0x0F00;
    // Disable pull-ups
    PORTB=PORTB&0x0F00;
}

/**
 * Test loop LEDS
 */
void leds_test()
{
    disable_leds();
    byte i = 0;
    do {
        for(volatile byte u =0; u < 255;u++){
            leds_pwm(1,i);
            leds_pwm(0,i);
            leds_pwm(0,i);
            leds_pwm(0,i);
        }
        i++;
    }while(i < 16);

    do {
        for(volatile byte u =0; u < 255;u++){
            leds_pwm(1,i-1);
            leds_pwm(0,i-1);
            leds_pwm(0,i-1);
            leds_pwm(0,i-1);
        }
        i--;
    }while(i >  0);
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
        // Normal or Inverse subbank
        if((t&1)==0) {
            PORTD=PORTD|(0x0040);
        } // Else 0 , already is 0
        // Enable Bank 1 as output
        DDRD|=0x0040;
    }
    else {
        // Normal or Inverse subbank
        if((t&1)==0) {
            PORTD=PORTD|(0x0080);
        } // Else 0 , already is 0
        // Bank 2 as output
        DDRD|=0x0080;
    }

    // Make column either high or low, depending on normal/inverse bank
    if((t&1) != 0) {
        PORTB|=((1<<index));
    } // Else 0 , already is 0
    // Set the right column as output
    DDRB|=(1<<index);

}

void leds_pwm(byte on, byte a)
{
    volatile byte iter;
    disable_leds();
    for(iter=ddelay;iter; iter--);
    if(on) {
        set_out(remap[a]);
    }
    for(iter=ldelay;iter; iter--);
}

void nightrider(byte l)
{
    disable_leds();
    do {
        for(byte a = 0; a < 4; a++) {
            for(volatile int u = 0; u < 255;u++) {
                leds_pwm(1,a);
                leds_pwm(0,a);
                leds_pwm(0,a);
                leds_pwm(0,a);
            }
        }
        for(byte a = 4; a ; a--) {
            for(volatile int u = 0; u < 255;u++) {
                leds_pwm(1,a-1);
                leds_pwm(0,a);
                leds_pwm(0,a);
                leds_pwm(0,a);
            }
        }
    }while(--l);
}
/**
 * Sample temperature
 * Display it.
 */
void display_temperature()
{
    volatile byte iter;
    disable_leds();
    // Request temperature
    sensors.setWaitForConversion(false);  // makes it async
    sensors.requestTemperatures();
    // Disable LEDs


    nightrider(3);

    sensors.setWaitForConversion(true);  // makes it async
    // Show temp.
    float value = sensors.getTempCByIndex(0);
    byte temp = value;
    byte ltemp = value*10;
    ltemp%=10;
    for(int l = 0; l < 2600 ; l++) {
        for(byte a= 0; a < 4; a++) {
            leds_pwm(1,a);
        }
        for(byte a= 0; a < 6; a++) {
            byte index = (1<<a);
            leds_pwm(temp&index, a+4);
        }
        for(byte a= 0; a < 6; a++) {
            byte index = (1<<a);
            leds_pwm(ltemp&index, a+10);
        }
    }
}
/**
 * Handle UART
 */
void handle_uart_commands_read_alarm()
{
    while(!Serial.available());
    byte alarm = Serial.read();
    alarm -= '0';
    if(alarm >= NUM_ALARMS) {
        Serial.println("Invalid alarm number");
        return;
    }

    while(!Serial.available());
    byte c = Serial.read();
    // Write alarm time.
    if( c == 'w' ) {
        // Read 1st digit of hour
        while(!Serial.available());
        byte t = Serial.read();
        alarm_hour[alarm] = (t-'0')*10;
        // Read 2nd digit of hour
        while(!Serial.available());
        t = Serial.read();
        alarm_hour[alarm] += (t-'0');
        // The -1 offset
        alarm_hour[alarm] -= 1; 

        // Read 1st digit of minute
        while(!Serial.available());
        t = Serial.read();
        alarm_minute[alarm] = (t-'0')*10;
        // Read 2nd digit of minute
        while(!Serial.available());
        t = Serial.read();
        alarm_minute[alarm] += (t-'0');
    } else if ( c == 'r' ) {
        Serial.write(alarm_hour[alarm]+1);
        Serial.write(alarm_minute[alarm]);
        if((alarm_enable&(1<<alarm)) > 0 ) {
            Serial.write((uint8_t)1);
        }else {
            Serial.write((uint8_t)0);
        }
        if((alarm_acknowledge&(1<<alarm)) > 0 ) {
            Serial.write((uint8_t)1);
        }else {
            Serial.write((uint8_t)0);
        }

    } else if ( c == 'e' ) {
        alarm_enable |= (1<<alarm);
    } else if ( c == 'd' ) {
        alarm_enable &= (~(1<<alarm));
    }
}

void handle_uart_command_time()
{
    while(!Serial.available());
    byte c = Serial.read();

    if ( c == 'w' ) {
        // Processes left to right.
        // Read 1st digit of hour
        while(!Serial.available());
        byte t = Serial.read();
        hour = (t-'0')*10;
        // Read 2nd digit of hour
        while(!Serial.available());
        t = Serial.read();
        hour += (t-'0');
        // Substract 1.
        hour -= 1;
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
    // Read
    else if ( c == 'r' ) 
    {
        Serial.write(hour+1);
        Serial.write(minute);
        Serial.write(second);
    }
}

void handle_uart_commands()
{
    byte t = Serial.read();
    if ( t == 'l' ) {
        while(!Serial.available());
        byte v = Serial.read();
        v %= 128;
        ddelay = v;
        ldelay = 128-v;
    } else if ( t == 'a' ) {
        handle_uart_commands_read_alarm();
    } else if ( t == 't' ) {
        display_temperature();
    } else if ( t == 'x' ) {
        leds_test();
    }
    else if ( t == 'a' ) {
        for(int l = 0; l < 20000;l++) {
            for(byte a= 0; a < 16;a++) {
                leds_pwm(1,a);
            }
        }
        disable_leds();
    }
    else if ( t == 'm' ) {
        while(!Serial.available());
        byte v = Serial.read();
        if(v == 0) {
            Serial.println(messages);
        }
        if (v > 0 && v < 7) {
            byte index = (1<<(v-1));
            if(messages&index) {
                messages &= ~index;
            }else{
                messages |= index;
            }
        }

    }
    else if (t == 's')
    {
        handle_uart_command_time();
    }
    //while(Serial.available()) Serial.read();
}

/** 
 * Show messages
 */
void show_messages()
{
    for ( byte a = 0; a < 6; a++) {
        byte index = (1<<a);
        leds_pwm(messages&index, a+4);
    }
    leds_pwm(1,10); 
    leds_pwm(1,11); 
    leds_pwm(1,12); 
    leds_pwm(1,13); 
    leds_pwm(1,14); 
    leds_pwm(1,15); 
}

void loop()
{
    volatile byte iter;

    /**
     * If button pushed, display pressed.
     */
    if(digitalRead(temp_pin) == LOW) {
        // Acknowledge an alarm, if it is active now.
        alarm_ack();
        display_temperature();
    }

    /**
     * Handle input.
     */
    if(Serial.available()){
        handle_uart_commands();
    }

    if((PIND&1) == 1) {
        disable_leds();
        if((second&15) == 0){
            set_out(remap[3]);
            for( volatile int j = 0; j < 10;j++);
            disable_leds();
        }
        for( volatile int j = 0; j < 10000;j++);
        return;
    }

    if(messages && (second%10) == 4) {
        show_messages();
        return;
    }

    // Hour, we display 12 hour time. (1 - 12)
    byte hour12 = ( hour % 12 ) + 1;
    for(byte a= 0; a < 4; a++) {
        byte index = 1 << a;
        leds_pwm(hour12&index, a);
    }
    //Minutes (0 - 59)
    for(byte a= 0; a < 6; a++) {
        byte index = 1 << a;
        leds_pwm(minute&index, a+4);
    }
    //Seconds (0 - 59)
    for(byte a= 0; a < 6; a++) {
        byte index = 1 << a;
        leds_pwm(second&index,a+10);
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

            if(hour > 23) {
                hour = 0;
                // Reset acknowledgments
                alarm_ack_reset();
            }
        }

    }
    /**
     * Alarm
     */
    buzzer_set(0); 
    if(alarm_active()) {
        if((second&1) == 0) {
            buzzer_set(1); 
        }
    }
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
    // Set pin 3 as input, pullup. 
    DDRD  &= (~1);
    PORTD |= 1;

    // Set pin 4 as output.
    DDRD  |= (1<<4);
    // Disable it
    PORTD &= ~(1<<4);
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
