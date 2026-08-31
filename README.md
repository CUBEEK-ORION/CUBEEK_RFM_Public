Welcome to Cubeek Software!

# CUBEEK - A Training Kit for Cube-Sat
![CUBEEK Image](images/cubeek.png)

CubeSat are tiny box-shaped satellites that are mainly launched into
low Earth orbit to observe the Earth, test new communications
technology or perform miniature experiments.

The basic CubeSat design is a cube 10 cm x 10 cm x 10 cm in size and
is called 1U (standing for ‘one unit’). It is similar in size to a standard
Rubik’s Cube. The mass of 1U is not allowed to be greater than 1.33
kg.

## Modules used in CUBEEK
Housekeeping Sensor:
* INA219

Modules that are used for Payload:
* GPS
* BMP085
* MPU6050
* Temt6000
* Dallas Temperature Sensor

The communication module:
* RFM26W

## Libraries Required
Download and add these libraries in order to run CUBEEK software properly. Follow the respective link to download them.
1. INA219: [click here](https://github.com/adafruit/Adafruit_INA219)
2. Dallas Temperature Sensor: [click here](https://github.com/milesburton/Arduino-Temperature-Control-Library)
3. GPS: [click here](https://github.com/mikalhart/TinyGPSPlus.git)
4. BMP085: [click here](https://github.com/adafruit/Adafruit-BMP085-Library.git)
5. BusIO: [click here](https://github.com/adafruit/Adafruit_BusIO) (library for I2C abstraction for bmp085)
6. OneWire: [click here](https://github.com/PaulStoffregen/OneWire) 
7. MPU6050: [click here](https://github.com/electroniccats/mpu6050) 
Note: If this MPU6050 library does not work, download this one:   
8. Adafruit_MPU6050: [click here](https://github.com/adafruit/Adafruit_MPU6050) 

## Software Required
Download software from following their links. Additional Software links are provided in the file named SDR-Manual available in this repo. 
1. Arduino IDE: [click here](https://www.arduino.cc/en/software)
2. FT232R Driver: [click here](https://ftdichip.com/drivers/vcp-drivers/)
3. Soundmodem_500bd: [click here](https://github.com/orionspacenepal/SanoSat-1-SoundModem-500-bps-Public)

In this repository, you will find two separate files to transmit different signal formats using the CUBEEK educational kit.
Cubeek_for_RFM26W_CW contains the code to transmit morse signal that contains sensor reads from the CUBEEK.
Cubeek_for_RFM26W_GFSK contains the code to transmit GFSK signal containing the sensor reads from the CUBEEK.

Necessary libraries that need to be copy pasted inside the libraries folder of Arduino are also provided.

IMPORTANT NOTE: DO NOT forget to select Atmega 328P (3.3V, 8Mhz) under Tools-Processor before uploading. Read the SDR-Manual PDF file to understand the steps to follow, libraries to use/download, and the expected results after uploading the code in CUBEEK.
You also need to download CWGet software to decode Morse and SoundModem_500bps software to decode GFSK.

Enjoy!!
