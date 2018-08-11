# ESP8266-Smartlight

Code for a smart light that uses an ESP8266 to control an SK6812 led strip. A user can either control the light by pressing a button or by sending data for each individual pixel over udp.

You can see the build of this lamp in [this YT Video](https://youtu.be/6UEOXfMJE6Q).

## Uploading the code to the ESP

### Prerequisites
Before you can compile and upload the code using the Arduino IDE, you will have to install some libraries first.
These are:
* [ESPAsyncUDP](https://github.com/me-no-dev/ESPAsyncUDP)
* [ClickButton](https://github.com/marcobrianza/ClickButton)

## Upload

Initially you will have to upload the code using a serial connection. Since this code also includes OTA (wireless programming), you can then upload new code, whithout having the ESP connected to you PC. Keep in mind that both your PC and the ESP have to be in the same network for OTA to work.

For uploading the code to my ESP I used the following settings:

* Board: "Generic ESP8266 Module"
* Flash Mode: "QIO"
* Flash Size: "4M (2M SPIFFS)"
* Debug port: "Disabled"
* Debug Level: "None"
* lwIP Variant: "v2 Higher Bandwith"
* Reset Method: "ck"
* Crystal Frequency: "26 MHz"
* VTables: "Flash"
* Flash Frequency: "80 MHz"
* CPU Frequency: "80 MHz"
* Builtin Led: "2"
* Upload Speed: "115200"
* Erase Flash: "Only Sketch"
* Port: either "cu.Something" or "Light1(<ip address>)"

Keep in mind that these can vary, if you use a different ESP, so make sure to use the correct settings for yours.

## Compiling the UDP sender application
Open a terminal and go inside the 'sender' folder `cd sender`.
Then create a new folder for the binary files `mkdir bin`. Change to that folder `cd bin` and run CMake `cmake ..`. After that compile with `make`. Then you should find a file called sender which you can execute with `./sender`. To change the ip to send data to modify the source code of `sender/main.cpp`.

# Credits
Petteri Aimonen [nanopb](https://github.com/nanopb/nanopb)
