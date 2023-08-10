/*****************************************************************
  File:             BM25S3221-1.h
  Author:           BESTMODULES
  Description:      Define classes and required variables
  History：         None
  V1.0.1   -- initial version; 2023-04-04; Arduino IDE : v1.8.19
******************************************************************/
#ifndef _BM25S3221_1_H_
#define _BM25S3221_1_H_

#include <Arduino.h>
#include <SoftwareSerial.h>

#define BAUDRATE 9600
#define CHECK_OK 0
#define CHECK_ERROR 1
#define TIMEOUT_ERROR 2
#define AUTO 0x40
#define CMD 0x41

class BM25S3221_1
{
public:
  BM25S3221_1(uint8_t statusPin, HardwareSerial *theSerial = &Serial);
  BM25S3221_1(uint8_t statusPin, uint8_t rxPin, uint8_t txPin);
  void begin();
  void preheatCountdown();
  uint16_t readPM25Value();
  uint8_t readDustValue(uint16_t array[]);
  bool isInfoAvailable();
  void readInfoPacket(uint8_t array[]);
  void setUploadMode(uint8_t modeCode);
  uint8_t sleep();
  uint8_t wakeUp();

private:
  uint8_t _statusPin, _rxPin, _txPin;
  uint8_t _receiveBuffer[32]; // Array for storing received data

  void writeBytes(uint8_t wBuf[], uint8_t wLen = 9);
  uint8_t readBytes(uint8_t rBuf[], uint8_t rLen = 9, uint16_t timeout = 10);
  HardwareSerial *_hardSerial = NULL;
  SoftwareSerial *_softSerial = NULL;
};

#endif
