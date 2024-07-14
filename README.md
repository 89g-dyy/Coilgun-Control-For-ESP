# Railgun Timing Controller

This repository contains code for a railgun timing controller, designed by 89g for study and research purposes. Unauthorized use of this code by any individual, company, or organization is prohibited. Please ensure you have obtained written authorization from the author before using this code.

## Features

- Precise control of multiple timing modes
- Control and operation through specific GPIO pins
- Web-based timing and working state adjustment via WiFi connection
- UART serial information monitoring
- Operation logic for anti-information intrusion

## Hardware Requirements

- ESP32 microcontroller or module
- A sufficiently stable power circuit
- Mainstream UART communication chips such as CH340, CP2102, etc.

## Legal Notice

Unauthorized use of this code is strictly prohibited. This code is only allowed to be used for hardware research in legally permitted venues. Unauthorized copying, modifying, merging, publishing, distributing, sublicensing, or selling copies of the software is prohibited. Use of this software must first obtain written authorization from the author. The software is provided "as is", without any express or implied warranties. The author or copyright holder is not liable for any claim, damages, or other liabilities arising from the use or other dealings in the software.

For authorization to use this software, please contact: [heidawu@foxmail.com](mailto:heidawu@foxmail.com)

## Getting Started

### Prerequisites

- ESP32 development board
- Arduino IDE with ESP32 board support
- A stable power supply
- Basic knowledge of electronics and programming

### Installation

1. **Clone this repository:**
   ```sh
   git clone https://github.com/yourusername/railgun-timing-controller.git
   cd railgun-timing-controller

2. **Open the project in Arduino IDE:**
    - Open railgun_timing_controller.ino in the Arduino IDE.
     
3. **Configure the WiFi settings:**
    - Modify the ssid and password variables in the code to match your WiFi network.
4. **Upload the code to the ESP32:**
    - Select the correct board and port in the Arduino IDE.
    - Click the upload button to flash the code to the ESP32.
  
### Usage

1. **Hardware Setup:**
    - Connect the ESP32 to your hardware setup following the pin definitions in the code.
    - Ensure a stable power supply to avoid disruptions.
2. **WiFi Control:**
    - he WiFi control pin is set to pull-up by default.
    - To use WiFi for parameter adjustment, ensure the wifiControlPin is grounded when powering on or pressing EN to restart.
    - After adjustment, you can disconnect the WiFi control pin.
3. **Web Interface:**
    - If the wifiControlPin is low, the ESP32 will start in AP mode with the specified SSID and password.
    - Connect to the WiFi network and access the web interface at the IP address printed in the serial monitor.
    - Use the web interface to adjust timing parameters and enable/disable triggers.
4. **Button Operation:**
    - The button pin is set to input mode with a pull-up resistor.
    - Press the button to start the timing sequence.
5. **EEPROM Storage:**
    - Timing parameters and trigger states are stored in EEPROM.
    - Use the web interface to update the parameters, which are then written to EEPROM.
  
## Code Overview

- `readEEPROM()`: Reads timing parameters and trigger states from EEPROM.
- `writeEEPROM()`: Writes timing parameters and trigger states to EEPROM.
- `onTimer()`: Timer interrupt service routine for handling timing sequences.
- `onButtonPress()`: Interrupt service routine for handling button presses.
- `handleRoot()`: Handles the root URL, providing the web interface.
- `handleUpdate()`: Handles form submission for updating timing parameters.

## License

This project is licensed under the terms stated in the legal notice section. For further details, please contact the author.

---

For any issues or questions, please open an issue on this repository or contact the author at [heidawu@foxmail.com](mailto:heidawu@foxmail.com).

---

Enjoy your hardware research and experimentation
