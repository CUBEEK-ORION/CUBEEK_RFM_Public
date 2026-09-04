// Knine-S 2026/9/4

#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_BMP085.h>
#include <SPI.h>
#include <RH_RF24.h>

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
  delay(1000);

  Serial.println();
  Serial.println("=====================================================");
  Serial.println(" Namaste! Welcome to CubeSat Sensor GFSK Transmitter!");
  Serial.println("=====================================================");

  Wire.begin();

  // Initialize DS18B20
  if(!tempSensor.begin())
  {
    Serial.println("Temperature sensor not found!");
    while (1);
  }
    Serial.println("Temperature sensor found!");

  // Initialize BMP180
  if (!bmp.begin()) {
    Serial.println("BMP180 sensor not found!");
    while (1);
  }
    Serial.println("BMP180 sensor found!");

  // Initialize MPU6050
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission(true);

  calculate_IMU_error();

  if (!rf24.init()) {
    Serial.println("RF24 init failed");
    while (1);
  }

  rf24.setFrequency(433.3);

  // Set modem config to GFSK_Rb0_5Fd1 — 0.5 kbps, 1 kHz deviation
  if (!rf24.setModemConfig(RH_RF24::GFSK_Rb0_5Fd1)) {
    Serial.println("Modem config failed");
    while (1);
  }

  rf24.setTxPower(0x7f); // 13 dBm (adjust if needed)
  
  const uint8_t sync_word[] = {0x01,0x2D,0xD4,0x00,0x00};
  rf24.set_properties(0x1100,sync_word,sizeof(sync_word));

  // CRC
  const uint8_t crc_config[] = {0x85};
  rf24.set_properties(0x1200,crc_config,sizeof(crc_config));

  // PREAMBLE
  const uint8_t preamble_config[] = {0x04,0x14,0x00,0x00,0x31};
  rf24.set_properties(0x1000,preamble_config,sizeof(preamble_config));

  // PACKET LENGTH
  const uint8_t pkt_len[] = {0x02,0x01,0x00};
  rf24.set_properties(0x1208,pkt_len,sizeof(pkt_len));

  // FIELD 1
  const uint8_t pkt_field1[] = {0x00,0x01,0x00,0xAA};
  rf24.set_properties(0x120D,pkt_field1,sizeof(pkt_field1));

  // FIELD 2
  const uint8_t pkt_field2[] = {0x00,0xFF,0x00,0xAA};
  rf24.set_properties(0x1211,pkt_field2,sizeof(pkt_field2));

  // UNUSED FIELDS
  const uint8_t pkt_fieldn[] = {0x00,0x00,0x00,0x00};
  rf24.set_properties(0x1215,pkt_fieldn,sizeof(pkt_fieldn));
  rf24.set_properties(0x1219,pkt_fieldn,sizeof(pkt_fieldn));
  rf24.set_properties(0x121D,pkt_fieldn,sizeof(pkt_fieldn));

  currentTime = millis();

  Serial.println();
  Serial.println("---------------------------------");
  Serial.println("RF24 initialized");
  Serial.println("Frequency : 433.3 MHz");
  Serial.println("Modulation: GFSK");
  Serial.println("Bit rate  : 500 bps");
  Serial.println("Deviation : 1 kHz");
  Serial.println("---------------------------------");
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

  uint8_t framedPayload[110];

  uint8_t textLen = snprintf(
    (char *)&framedPayload[5],
    105,
    "TempDS:%sC,TempBMP:%sC,Pres:%sPa,Alt:%sm,Roll:%s,Pitch:%s,Yaw:%s",
    dsBuf,
    bmpTBuf,
    pBuf,
    altBuf,
    rollBuf,
    pitchBuf,
    yawBuf
  );

  // Check payload length
  if (textLen > 100)
  {
    Serial.println("Payload too long!");
    delay(2000);
    return;
  }

  // Add marker
  framedPayload[1] = 0xFF;
  framedPayload[2] = 0xFF;
  framedPayload[3] = 0x00;
  framedPayload[4] = 0x00;


  // Field 1 = Field 2 length
  // Field 2 = marker + actual payload
  uint8_t field2Len = textLen + 4;
  framedPayload[0] = field2Len;
  uint8_t framedLen = textLen + 5;

  // UPDATE FIELD 2 LENGTH
  rf24.set_properties(0x1212,&field2Len,1);

  // TRANSMIT
  if (!rf24.send(framedPayload,framedLen))
  {
    Serial.println("RF24 send failed!");
  }
  else
  {
    bool sent = rf24.waitPacketSent(3000);

    if (!sent)
    {
      Serial.println(
        "waitPacketSent timed out!"
      );
    }
    else
    {
      Serial.print("TX (");
      Serial.print(framedLen);
      Serial.print(" bytes): ");

      Serial.println(
        (char *)&framedPayload[5]
      );
    }
  }

  delay(2000);
}

void calculate_IMU_error() {
  int c = 0;
  
  AccErrorX = 0;
  AccErrorY = 0;

  GyroErrorX = 0;
  GyroErrorY = 0;
  GyroErrorZ = 0;

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
