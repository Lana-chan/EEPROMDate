EEPROMDate
==========

 Adaptation of the MEEPROMMER project for larger memories.

 The code as it currently stands was last modified in 2014-08-05  and isn't very
well documented,  since it was intended  for my personal use only.  This project
currently supports only the  AT29C040A  Atmel 4MBit Flash chip,  but it could be
modified to support similar parallel (E)EPROMs.

Credits
-------

  * Mario Keller for the original MEEPROMMER -
      https://github.com/mkeller0815/MEEPROMMER
  * Tiido Primagi for insight on writing to the AT29C040A chip -
      http://www.tmeeco.eu/
      
Wiring guide
------------

 To achieve a greater number of  control outputs  from a standard  Arduino UNO's
digital pins, I use a common 8-bit "bus"  (pins 2-9)  for both data and address, 
wired directly to the input of 3  Octal D-Latch flip-flops  (74HC574 or similar)
and the I/O pins of the  EEPROM at the same time.  The outputs of the flip-flops
go to the address pins of the EEPROM and the enable pins of the flip-flops go to
pins 10-12 on the Arduino.  Pins WE, OE and CE of the  EEPROM go to  analog pins
0, 1 and 2 respectively.

Python tool usage
-----------------

 The Python tool is derived from the one found in the MEEPROMMER project and has
only been slightly modified to  account for the changes in the new Arduino code,
but it is by no means polished. The usage should be similar that of the original
tool, only note that when writing to the AT29C040A chip you must always do so in
256-byte pages.