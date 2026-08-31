#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_BMP085.h>
#include <SPI.h>
#include <RH_RF24.h>
#include <Adafruit_MPU6050.h>

// --- RF24 Setup ---
#define RFM_CS 10
#define RFM_INT 2
RH_RF24 rf24(RFM_CS, RFM_INT);

// --- Temperature Sensor (DS18B20) Setup ---
#define ONE_WIRE_BUS 6
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature tempSensor(&oneWire);

// --- Pressure Sensor (BMP180) Setup ---
Adafruit_BMP085 bmp;

// --- MPU6050 Setup ---
const int MPU = 0x69; // AD0 High
float AccX, AccY, AccZ;
float GyroX, GyroY, GyroZ;
float accAngleX, accAngleY, gyroAngleX, gyroAngleY, gyroAngleZ;
float roll, pitch, yaw;
float AccErrorX, AccErrorY, GyroErrorX, GyroErrorY, GyroErrorZ;
float elapsedTime, currentTime, previousTime;

void setup() {
  Serial.begin(115200);

  // Initialize DS18B20
  tempSensor.begin();

  // Initialize BMP180
  if (!bmp.begin()) {
    Serial.println("BMP180 sensor not found!");
    while (1);
  }

  // Initialize MPU6050
  Wire.begin();
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission(true);

  calculate_IMU_error();

if (!rf24.init()) {
    Serial.println("RF24 init failed");
    while (1);
  }

rf24.setFrequency(436.6);

  // Set modem config to GFSK_Rb0_5Fd1 — 0.5 kbps, 1 kHz deviation
  if (!rf24.setModemConfig(RH_RF24::GFSK_Rb0_5Fd1)) {
    Serial.println("Modem config failed");
    while (1);
  }

  rf24.setTxPower(13); // 13 dBm (adjust if needed)

  currentTime = millis();
  
  Serial.println("RF24 initialized with GFSK 500bps @1kHz deviation");
}



void loop() {
  // --- Read DS18B20 Temperature ---
  tempSensor.requestTemperatures();
  float ds_temp = tempSensor.getTempCByIndex(0);

  // --- Read BMP180 ---
  float pressure = bmp.readPressure();
  float bmp_temp = bmp.readTemperature();
  float altitude = bmp.readAltitude();

  // --- Read MPU6050 ---
  readIMU();

  // --- Combine and Print All Sensor Data ---
  Serial.print("DS18B20 Temp: "); Serial.print(ds_temp); Serial.print(" *C\t");
  Serial.print("BMP180 Temp: "); Serial.print(bmp_temp); Serial.print(" *C\t");
  Serial.print("Pressure: "); Serial.print(pressure); Serial.print(" hPa\t");
  Serial.print("Altitude: "); Serial.print(altitude); Serial.print(" m\t");
  Serial.print("Roll: "); Serial.print(roll); Serial.print("\tPitch: "); Serial.print(pitch); Serial.print("\tYaw: "); Serial.println(yaw);

  char dsBuf[8], bmpTBuf[8], pBuf[10], altBuf[8];
  char rollBuf[8], pitchBuf[8], yawBuf[8];

  dtostrf(ds_temp,  1, 1, dsBuf);
  dtostrf(bmp_temp, 1, 1, bmpTBuf);
  dtostrf(pressure, 1, 1, pBuf);
  dtostrf(altitude, 1, 1, altBuf);
  dtostrf(roll,     1, 1, rollBuf);
  dtostrf(pitch,    1, 1, pitchBuf);
  dtostrf(yaw,      1, 1, yawBuf);

  static char payload[140];
  snprintf(payload, sizeof(payload),
           "TempDS:%sC,TempBMP:%sC,Pres:%sPa,Alt:%sm,Roll:%s,Pitch:%s,Yaw:%s",
           dsBuf, bmpTBuf, pBuf, altBuf, rollBuf, pitchBuf, yawBuf);

  uint16_t payloadLen = strlen(payload);

  if (!rf24.send((uint8_t *)payload, payloadLen)) {
    Serial.println("RF24 send failed");
  }
  rf24.waitPacketSent();

  Serial.print("TX ("); Serial.print(payloadLen); Serial.print(" bytes): ");
  Serial.println(payload);

  delay(2000);
}

void calculate_IMU_error() {
  int c = 0;
  while (c < 200) {
    Wire.beginTransmission(MPU);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU, 6, true);
    AccX = (Wire.read() << 8 | Wire.read()) / 16384.0;
    AccY = (Wire.read() << 8 | Wire.read()) / 16384.0;
    AccZ = (Wire.read() << 8 | Wire.read()) / 16384.0;
    AccErrorX += atan(AccY / sqrt(pow(AccX, 2) + pow(AccZ, 2))) * 180 / PI;
    AccErrorY += atan(-AccX / sqrt(pow(AccY, 2) + pow(AccZ, 2))) * 180 / PI;
    c++;
  }
  AccErrorX /= 200;
  AccErrorY /= 200;

  // NOTE: raw ticks are converted to deg/s ONCE here (raw/131.0), then
  // averaged directly. Dividing by 131.0 a second time when accumulating
  // makes the bias ~131x too small.
  c = 0;
  while (c < 200) {
    Wire.beginTransmission(MPU);
    Wire.write(0x43);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU, 6, true);
    int16_t rawGX = (Wire.read() << 8) | Wire.read();
    int16_t rawGY = (Wire.read() << 8) | Wire.read();
    int16_t rawGZ = (Wire.read() << 8) | Wire.read();
    GyroErrorX += rawGX / 131.0;
    GyroErrorY += rawGY / 131.0;
    GyroErrorZ += rawGZ / 131.0;
    c++;
  }
  GyroErrorX /= 200;
  GyroErrorY /= 200;
  GyroErrorZ /= 200;
}

void readIMU() {
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 6, true);
  AccX = (Wire.read() << 8 | Wire.read()) / 16384.0;
  AccY = (Wire.read() << 8 | Wire.read()) / 16384.0;
  AccZ = (Wire.read() << 8 | Wire.read()) / 16384.0;

  accAngleX = (atan(AccY / sqrt(pow(AccX, 2) + pow(AccZ, 2))) * 180 / PI) - AccErrorX;
  accAngleY = (atan(-AccX / sqrt(pow(AccY, 2) + pow(AccZ, 2))) * 180 / PI) + AccErrorY;

  previousTime = currentTime;
  currentTime = millis();
  elapsedTime = (currentTime - previousTime) / 1000.0;

  Wire.beginTransmission(MPU);
  Wire.write(0x43);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 6, true);
  GyroX = (Wire.read() << 8 | Wire.read()) / 131.0;
  GyroY = (Wire.read() << 8 | Wire.read()) / 131.0;
  GyroZ = (Wire.read() << 8 | Wire.read()) / 131.0;

  GyroX -= GyroErrorX;
  GyroY -= GyroErrorY;
  GyroZ -= GyroErrorZ;

  gyroAngleX += GyroX * elapsedTime;
  gyroAngleY += GyroY * elapsedTime;
  yaw += GyroZ * elapsedTime;

  roll = 0.96 * gyroAngleX + 0.04 * accAngleX;
  pitch = 0.96 * gyroAngleY + 0.04 * accAngleY;
}
