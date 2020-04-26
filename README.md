# SimpleURemote
Simple universal IR-remote. 2 button operation. For ESP8266 -based microcontrollers.

This code uses IRremoteESP8266 -library by David Conran (crankyoldgit)
https://github.com/crankyoldgit/IRremoteESP8266

How to use:
  - Press and release button 1 (Red in the example) to record an IR-signal.
    -LED blinks once and then stays on to indicate device is waiting for IR-signal.
    -When signal is received LED blinks quickly 5 times.
    -If there is no signal for about 10 seconds. LED shuts off.
    -This also wipes the previously recorded signal if there is one.

  - Press and release button 2 (White in the example) to send the recorded IR-signal.
    -LED blinks 3 times quickly to indicate the recorded IR-signal was sent.
    -If the LED slowly blinks twice there was no recorded signal to send. Please record a signal first.
    
    
    Example schematics:
    ![Breadboard example](https://github.com/jpmaaltonen/SimpleURemote/blob/master/SimpluURemote_schematic_bb.jpg)
    ![schematic example](https://github.com/jpmaaltonen/SimpleURemote/blob/master/SimpluURemote_schematic_schem.jpg)
    
