/*************************************************
File: readDustConcentration.ino
Description: Get the dust concentration.
             Users can control how the module runs
             by sending corresponding serial numbers(1~4)
             through the Serial Monitor.
Note:
**************************************************/

#include <BM25S3221-1.h>

#define STA_PIN 22 // Input pin
// #define RX_PIN 2  // Simulate as RX pin
// #define TX_PIN 3  // Simulate as TX pin
#define PM1_0 dustValue[0]
#define PM2_5 dustValue[1]
#define PM10 dustValue[2]

uint8_t dataBuf[32] = {0}, command = 0;
uint16_t dustValue[3] = {0};
// BM25S3221_1 dust(STA_PIN, RX_PIN, TX_PIN); // Softeware serial:8->Input pin, 2->RX pin, 3->TX pin

/* BMduino-UNO */
// BM25S3221_1 dust(STA_PIN, &Serial); // Hardware serial: Serial
BM25S3221_1 dust(STA_PIN, &Serial1); // Hardware serial: Serial1
// BM25S3221_1 dust(STA_PIN, &Serial2); // Hardware serial: Serial2
// BM25S3221_1 dust(STA_PIN, &Serial3); // Hardware serial: Serial3
// BM25S3221_1 dust(STA_PIN, &Serial4); // Hardware serial: Serial4

void setup()
{
  dust.begin();       // Initialize module, baud rate: 9600bps
  Serial.begin(9600); // Initialize Serial, baud rate: 9600bps

  Serial.println("Module preheating...(about 30 second)");
  dust.preheatCountdown(); // Wait for the End of module preheating.
  Serial.println("End of module preheating.");
  Serial.println();
  Serial.println("Perform initial setup.");
  displayMenu();
}

void loop()
{
  selectMode();
  delay(30);
  if (command == 1)
  {
    if (dust.readDustValue(dustValue) != 0)
    {
      Serial.println("read failed!");
    }
    delay(1000);
  }
  if (command == 2)
  {
    if (dust.isInfoAvailable() == true)
    {
      dust.readInfoPacket(dataBuf);
      PM1_0 = ((uint16_t)dataBuf[10] << 8) + dataBuf[11];
      PM2_5 = ((uint16_t)dataBuf[12] << 8) + dataBuf[13];
      PM10 = ((uint16_t)dataBuf[14] << 8) + dataBuf[15];
    }
    else
    {
      Serial.println("read failed!");
    }
  }
  if (command > 0)
  {
    Serial.print("PM1.0: ");
    Serial.print(PM1_0);
    Serial.println(" μg/m³");

    Serial.print("PM2.5: ");
    Serial.print(PM2_5);
    Serial.println(" μg/m³");

    Serial.print("PM10: ");
    Serial.print(PM10);
    Serial.println(" μg/m³");
    Serial.println();
    delay(1000);
  }
}

void displayMenu()
{
  Serial.println("==== Enter the serial number to run the corresponding command ====");
  Serial.println("1. Setup the module to command query mode.");
  Serial.println("2. Setup the module to active upload mode.");
  Serial.println("==================================================================");
  Serial.println();
}

void selectMode()
{
  uint8_t tmp[2] = {0};
  while (Serial.available() > 0)
  {
    tmp[0] = Serial.read();
    tmp[1] = Serial.read();
    if (tmp[1] == 13) // Carriage return: 13(ASCII code)
    {
      command = tmp[0] - 48;
    }
    else
    {
      command = 0;
    }
    switch (command)
    {
    case 0:
      Serial.println("Please enter the correct serial number and end with carriage return.");
      break;
    case 1:
      Serial.println("1. Setup the module to command query mode.");
      dust.setUploadMode(CMD);
      break;
    case 2:
      Serial.println("2. Setup the module to active upload mode.");
      dust.setUploadMode(AUTO);
      delay(1200);
      break;
    }
    Serial.println();
  }
}
