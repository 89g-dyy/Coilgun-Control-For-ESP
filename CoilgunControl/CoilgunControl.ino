/*********************************************************************
This code is written by 89g for the study and research of the Coilgun timing controller. Unauthorized use of this code by any individual, company, or organization is prohibited. To use this code, you must obtain my written authorization.
This code is only allowed to be used for hardware research in legally permitted venues; otherwise, it is considered illegal and unauthorized use.
If you do not have my authorization manual, it is also considered unauthorized. Please delete the code text and compiled files after learning the code.
*********************************************************************/

/*********************************************************************
Declaration: Any person obtaining a copy of this software and associated documentation files (the "Software") is only allowed to use the Software under the following conditions:

Unauthorized individuals, companies, or organizations are prohibited from illegally using, copying, modifying, merging, publishing, distributing, sublicensing, and/or selling copies of the Software.
Any use of the Software must first obtain my written authorization.
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
The Software is provided "as is", without any express or implied warranties, including but not limited to the warranties of merchantability, fitness for a particular purpose, and non-infringement. In no event shall the author or copyright holder be liable for any claim, damages, or other liability, whether in an action of contract, tort, or otherwise, arising from, out of, or in connection with the Software or the use or other dealings in the Software.

To obtain authorization to use this software, please contact: [heidawu@foxmail.com]
*********************************************************************/

/* This program is an implementation of a Coilgun timing controller. It includes the following features:
 * - Precise control of multiple timing modes
 * - Control and operation through specific GPIO pins
 * - Web-based timing and working state adjustment via WiFi connection
 * - UART serial information monitoring
 * - Operation logic for anti-information intrusion
 *
 * Hardware requirements:
 * - ESP32 microcontroller or module
 * - A sufficiently stable power circuit
 * - Mainstream UART communication chips such as CH340, CP2102, etc.
 *
 * Note: Please ensure you have legal authorization before using this program.
 */

/*
Steps to modify pin operation:
Modify the numbers following NULL in const int ledPins[]. The first parameter is NULL to prevent instability caused by process blocking.
The pin is high when not triggered and low when triggered. Please pay attention to this when designing the circuit.

WiFi parameter adjustment steps:
The wifiControlPin is built-in pull-up by default.
To use WiFi for parameter adjustment, ensure the wifiControlPin is grounded when powering on or pressing EN to restart. You can disconnect after the adjustment is completed.

Note: Avoid using the io0 pin on ESP32.
The IO0 pin on ESP32 can affect the chip startup mode under certain circumstances.
The IO0 pin is configured as a button pin, pulling it low will trigger an interrupt that affects the normal operation of the ESP32.

It is recommended to use optocoupler isolation to trigger IGBT. Generally, when the LED in the optocoupler is lit, the optocoupler is triggered.
The LED+ of the optocoupler is connected to 3.3V voltage with a series resistor of about 1K, and the LED- is connected to the trigger pin.
*/
#include <WiFi.h>
#include <EEPROM.h>
#include <WebServer.h>

const char* ssid = "Coilgun"; // Set WiFi SSID within ""
const char* password = "88888888"; // Set WiFi password within ""

const int ledPins[] = {NULL, 32, 33, 25, 26, 27, 14, 2, 4, 16, 17, 5, 18, 19, 21, 22, 23}; // Define 17 trigger pins
const int ledCount = 17; // Number of triggers
const int buttonPin = 12; // Pin connected to the button
const int wifiControlPin = 13; // Define the pin controlling WiFi signal to prevent WiFi attacks

// Define the on-duration and off-duration of each parameter (in microseconds)
int onDurations[ledCount];
int offDurations[ledCount];
bool ledEnabled[ledCount]; // Whether activated

hw_timer_t *timer = NULL; // Timer handle
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED; // Mutex lock
volatile int ledIndex = -1; // Index of the current trigger, initialized to -1 meaning no LED is lit
volatile bool buttonPressed = false; // Whether the button is pressed
volatile bool isOn = false; // Whether the current state is triggered
volatile int currentDuration = 0; // Current timer duration

WebServer server(80); // Create a Web server object

void readEEPROM() {
  for (int i = 0; i < ledCount; i++) {
    onDurations[i] = EEPROM.read(i * 4) | (EEPROM.read(i * 4 + 1) << 8) | (EEPROM.read(i * 4 + 2) << 16) | (EEPROM.read(i * 4 + 3) << 24);
    offDurations[i] = EEPROM.read(ledCount * 4 + i * 4) | (EEPROM.read(ledCount * 4 + i * 4 + 1) << 8) | (EEPROM.read(ledCount * 4 + i * 4 + 2) << 16) | (EEPROM.read(ledCount * 4 + i * 4 + 3) << 24);
    ledEnabled[i] = EEPROM.read(2 * ledCount * 4 + i);
  }
}

void writeEEPROM() {
  for (int i = 0; i < ledCount; i++) {
    EEPROM.write(i * 4, onDurations[i] & 0xFF);
    EEPROM.write(i * 4 + 1, (onDurations[i] >> 8) & 0xFF);
    EEPROM.write(i * 4 + 2, (onDurations[i] >> 16) & 0xFF);
    EEPROM.write(i * 4 + 3, (onDurations[i] >> 24) & 0xFF);
    EEPROM.write(ledCount * 4 + i * 4, offDurations[i] & 0xFF);
    EEPROM.write(ledCount * 4 + i * 4 + 1, (offDurations[i] >> 8) & 0xFF);
    EEPROM.write(ledCount * 4 + i * 4 + 2, (offDurations[i] >> 16) & 0xFF);
    EEPROM.write(ledCount * 4 + i * 4 + 3, (offDurations[i] >> 24) & 0xFF);
    EEPROM.write(2 * ledCount * 4 + i, ledEnabled[i]);
  }
  EEPROM.commit();
}

void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  if (ledIndex >= 0) {
    if (isOn) {
      // If currently triggered, set delay duration and switch to non-triggered state
      digitalWrite(ledPins[ledIndex], HIGH); // Turn off the trigger (high level)
      currentDuration = offDurations[ledIndex];
      isOn = false;
    } else {
      // If currently not triggered, update index
      do {
        ledIndex++;
        if (ledIndex >= ledCount) {
          // If the cycle completes once, stop the timer
          timerAlarmDisable(timer);
          ledIndex = -1; // Reset trigger index
          break;
        }
      } while (!ledEnabled[ledIndex]); // Skip disabled trigger pins

      if (ledIndex < ledCount) {
        // Trigger the next pin, set the on-duration and switch to triggered state
        digitalWrite(ledPins[ledIndex], LOW); // Trigger the next IO (low level)
        currentDuration = onDurations[ledIndex];
        isOn = true;
      }
    }
    if (ledIndex >= 0) {
      timerAlarmWrite(timer, currentDuration, true); // Set timer alarm, in microseconds
    }
  }
  portEXIT_CRITICAL_ISR(&timerMux);
}

void IRAM_ATTR onButtonPress() {
  portENTER_CRITICAL_ISR(&timerMux);
  if (ledIndex == -1) { // If no cycle is in progress
    buttonPressed = true;
  }
  portEXIT_CRITICAL_ISR(&timerMux);
}

void handleRoot() {
  String html = "<html><body><h1>Railgun Timing Adjustment Page (us)</h1><form action='/update' method='POST'>";
  for (int i = 1; i < ledCount; i++) { // Skip the 0th level
    html += "Level " + String(i) + " On Duration: <input type='text' name='on" + String(i) + "' value='" + String(onDurations[i]) + "'><br>";
    html += "Level " + String(i) + " Off Duration: <input type='text' name='off" + String(i) + "' value='" + String(offDurations[i]) + "'><br>";
    html += "Level " + String(i) + " Enabled: <input type='checkbox' name='enabled" + String(i) + "'" + (ledEnabled[i] ? " checked" : "") + "><br><br>";
  }
  html += "<input type='submit' value='Update Data'></form></body></html>";
  server.send(200, "text/html", html);
}

// Function to handle form submission
void handleUpdate() {
  for (int i = 1; i < ledCount; i++) { // Skip the 0th level
    if (server.hasArg("on" + String(i))) {
      onDurations[i] = server.arg("on" + String(i)).toInt();
    }
    if (server.hasArg("off" + String(i))) {
      offDurations[i] = server.arg("off" + String(i)).toInt();
    }
    ledEnabled[i] = server.hasArg("enabled" + String(i));
  }
  writeEEPROM();
  server.send(200, "text/html", "<html><body><h1>Updated</h1><a href='/'>Back</a></body></html>");
}

void setup() {
  Serial.begin(115200);

  EEPROM.begin(2 * ledCount * 4 + ledCount); // Initialize EEPROM, each variable 4 bytes, 2 times because of on and off arrays, plus ledEnabled
  readEEPROM(); // Read data from EEPROM

  // Manually set the values of the 0th level
  onDurations[0] = 1000; // Set the on-duration of the 0th level (in microseconds)
  offDurations[0] = 1000; // Set the off-duration of the 0th level (in microseconds)
  ledEnabled[0] = true; // Enable the 0th level

  for (int i = 0; i < ledCount; i++) {
    pinMode(ledPins[i], OUTPUT); // Set the pin to output mode
    digitalWrite(ledPins[i], HIGH); // Turn off all optocouplers (high level)
  }

  pinMode(buttonPin, INPUT_PULLUP); // Set the button pin to input mode and enable pull-up resistor
  attachInterrupt(digitalPinToInterrupt(buttonPin), onButtonPress, FALLING); // Attach button interrupt service routine

  pinMode(wifiControlPin, INPUT_PULLUP); // Set the WiFi control pin to input mode and enable pull-up resistor

  // Check the status of the WiFi control pin
  if (digitalRead(wifiControlPin) == LOW) {
    // Set AP mode
    WiFi.softAP(ssid, password);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);

    // Set up Web server
    server.on("/", handleRoot);
    server.on("/update", HTTP_POST, handleUpdate);
    server.begin();
  } else {
    Serial.println("WiFi is disabled");
  }

  timer = timerBegin(0, 80, true); // Initialize timer
  timerAttachInterrupt(timer, &onTimer, true); // Attach interrupt handler
}

void loop() {
  if (digitalRead(wifiControlPin) == LOW && WiFi.softAPgetStationNum() == 0) {
    // WiFi pin is pulled low, and WiFi is not connected, start WiFi
    WiFi.softAP(ssid, password);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    server.begin();
  } else if (digitalRead(wifiControlPin) == HIGH && WiFi.softAPgetStationNum() > 0) {
    // WiFi pin is not pulled low, and WiFi is connected, turn off WiFi
    WiFi.softAPdisconnect(true);
    Serial.println("WiFi is disabled");
  }

  server.handleClient();

  if (buttonPressed) {
    portENTER_CRITICAL(&timerMux);
    buttonPressed = false; 
    ledIndex = 0; // Reset trigger index, start triggering the first pin
    while (!ledEnabled[ledIndex] && ledIndex < ledCount) { // Skip disabled pins
      ledIndex++;
    }
    if (ledIndex < ledCount) {
      isOn = true; // Start with the triggered state
      digitalWrite(ledPins[ledIndex], LOW); // Trigger and light up the first one (low level)
      currentDuration = onDurations[ledIndex];
      timerAlarmWrite(timer, currentDuration, true); // Set the on-duration, in microseconds
      timerAlarmEnable(timer); // Enable timer alarm
    } else {
      ledIndex = -1; // If no IO is enabled, reset IO index
    }
    portEXIT_CRITICAL(& timerMux);
  }

  // Ensure only one IO is triggered, the rest are off
  for (int i = 0; i < ledCount; i++) {
    if (i != ledIndex || !isOn) {
      digitalWrite(ledPins[i], HIGH); // Turn off pins that should not be triggered (high level)
    }
  }
}
