BOARD_TAG    = leonardo
ARDUINO_LIBS =


include /usr/share/arduino/Arduino.mk

indent: binclock_c.ino
	uncrustify -c uncrustify.cfg --replace binclock_c.ino 
