/*
    SimpleURemote
    Simple universal IR-remote
    By jpma - Matias Aaltonen
    Version 1.0: April 2020

How to use:
  - Press button 1 (Red in the example) to record an IR-signal.
    -LED blinks once and then stays on to indicate device is waiting for IR-signal.
    -When signal is received LED blinks quickly 5 times.
    -If there is no signal for about 10 seconds. LED shuts off.
    -This also wipes the previously recorded signal if there is one.

  - Press button 2 (White in the example) to send the recorded IR-signal.
    -LED blinks 3 times quickly to indicate the recorded IR-signal was sent.
    -If the LED slowly blinks twice there was no recorded signal to send. Please record a signal first.

Board used:
Wemos D1 mini (ESP 8266)
Should work for any ESP 8266 -based board. Just check the right pins.

This code uses IRremoteESP8266 -library by David Conran (crankyoldgit)
https://github.com/crankyoldgit/IRremoteESP8266

Based on IRremoteESP8266 -library example
called SmartIRRepeater
https://github.com/crankyoldgit/IRremoteESP8266/blob/master/examples/SmartIRRepeater

Description of the function from library example:
" This program will try to capture incoming IR messages and tries to
  intelligently replay them back.
  It uses the advanced detection features of the library, and the custom
  sending routines. Thus it will try to use the correct frequencies,
  duty cycles, and repeats as it thinks is required.
  Anything it doesn't understand, it will try to replay back as best it can,
  but at 38kHz."


*/

#include <Arduino.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRutils.h>

// Defining pins

// Red led
// GPIO 5 = D1
const int led_pin = 5;

// IR led
// GPIO 4 = D2
const uint16_t kIrLedPin = 4;

// IR detector/demodulator
// GPIO 14 = D5
const uint16_t kRecvPin = 14;

// Red button
// GPIO 12 = D6
const int button1_pin = 12;

// White button
// GPIO 13 = D7
const int button2_pin = 13;

// The Serial connection baud rate.
// Make sure you set your Serial Monitor to the same speed.
const uint32_t kBaudRate = 115200;

// Capture buffer
// Larger than expected buffer so we can handle very large IR messages.
// i.e. Up to 512 bits.
const uint16_t kCaptureBufferSize = 1024;

// kTimeout is the Nr. of milli-Seconds of no-more-data before we consider
// a message ended.
const uint8_t kTimeout = 50; // Milli-Seconds

// kFrequency is the modulation frequency all messages will be replayed at.
// in Hz. e.g. 38kHz.
const uint16_t kFrequency = 38000;

// Default Button states
int button1_state = HIGH;
int button2_state = HIGH;

// Default previous button states
int button1_prev = HIGH;
int button2_prev = HIGH;

// Declare functions

// Blinks led on pin, multiplier -times with blink_delay -time(ms) between blinks.
void blinkled(int pin, int delay, int multiplier);

// Configure objects

// The IR transmitter.
IRsend irsend(kIrLedPin);
// The IR receiver.
IRrecv irrecv(kRecvPin, kCaptureBufferSize, kTimeout, false);
// Object to store the captured message.
decode_results results;

// Setup

void setup()
{

    // Setting up buttons and red LED as inputs/outputs.

    // Configure button pins as inputs.
    pinMode(button1_pin, INPUT_PULLUP);
    pinMode(button2_pin, INPUT);

    // Configure red LED as output.
    pinMode(led_pin, OUTPUT);

    // Setting initial red LED state as OFF
    digitalWrite(led_pin, LOW);

    // Starting serial monitor.
    Serial.begin(kBaudRate, SERIAL_8N1);
    while (!Serial) // Wait for serial port to connect.
        delay(50);
    Serial.println();
    Serial.print("Serial connection ON");
    Serial.println();

    // Start up the IR sender.
    irsend.begin();
}

// Main loop

void loop()
{
    // Read the state of both buttons
    button1_state = digitalRead(button1_pin);
    button2_state = digitalRead(button2_pin);

    // If Button 1 is pressed and released.
    if ((button1_prev == LOW) && (button1_state == HIGH))
    {

        // Start up the IR receiver.
        irrecv.enableIRIn();

        Serial.println("Recording IR-signal");

        // Blink led once and then leave it on
        // to indicate device is starting recording.
        blinkled(led_pin, 500, 1);
        digitalWrite(led_pin, HIGH);

        // Print out text in serial monitor while waiting for IR-signal.
        // If there is no signal after ~10 seconds the loop ends.
        // When signal is received statement turns true and we exit the loop.
        // delay is there to not flood the serial monitor and to not hit the WDT reset.
        for (int i = 0; (i < 20); i++)
        {
            Serial.println("waiting for signal...");
            delay(500);
            if (irrecv.decode(&results))
            {
                break;
            }
        }

        // If there is results print out the decoded result.
        // And blink led fast 5 times.
        if (irrecv.decode(&results))
        {
            // Received a signal. Blink led 5 times fast.
            Serial.println("Got results!");
            Serial.print(resultToHumanReadableBasic(&results));
            blinkled(led_pin, 50, 5);
        }

        // No signal. Turn off the LED.
        else
        {
            Serial.println("You took too long! Nothing recorded.");
            digitalWrite(led_pin, LOW);
        }
    }

    // If Button 2 is pressed and released.
    if ((button2_prev == LOW) && (button2_state == HIGH))
    {

        if (irrecv.decode(&results))
        {
            // Check that we have results.
            // Blink LED 3 times quickly to indicate sending the signal.
            blinkled(led_pin, 30, 3);

            decode_type_t protocol = results.decode_type;
            uint16_t size = results.bits;
            bool success = true;

            // Is it a protocol we don't understand?
            // Yes.
            if (protocol == decode_type_t::UNKNOWN)
            {

                // Convert the results into an array suitable for sendRaw().
                // resultToRawArray() allocates the memory we need for the array.
                uint16_t *raw_array = resultToRawArray(&results);

                // Find out how many elements are in the array.
                size = getCorrectedRawLength(&results);

                // Send it out via the IR LED circuit.
                irsend.sendRaw(raw_array, size, kFrequency);

                // Deallocate the memory allocated by resultToRawArray().
                delete[] raw_array;
            }

            // Does the message require a state[]?
            else if (hasACState(protocol))
            {
                // It does, so send with bytes instead.
                success = irsend.send(protocol, results.state, size / 8);
            }

            // Anything else must be a simple message protocol. ie. <= 64 bits
            else
            {
                success = irsend.send(protocol, results.value, size);
            }

            // Print sent signal. Print "..unsuccessfully.." if transmit fails.
            Serial.println("Sending IR-signal");
            Serial.print(resultToHumanReadableBasic(&results));
            Serial.printf("Message %ssuccessfully retransmitted.\n", success ? "" : "un");
        }

        // Indicate that there is no results to send.
        // Blink led twice.
        else
        {
            Serial.println("Nothing to send. Capture something first.");
            blinkled(led_pin, 600, 2);
        }
    }

    //Remember button state for next iteration
    button1_prev = button1_state;
    button2_prev = button2_state;

    yield(); //This ensures the ESP doesn't WDT reset.
}

// Define functions

// Blinks led on pin, count -times with blink_delay -time(ms) between blinks.
void blinkled(int pin, int blink_delay, int multiplier)
{
    for (int i = 0; i < multiplier; i++)
    {
        digitalWrite(pin, HIGH);
        delay(blink_delay);
        digitalWrite(led_pin, LOW);
        delay(blink_delay);
    }
    return;
}
