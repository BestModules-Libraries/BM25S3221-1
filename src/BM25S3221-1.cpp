/*****************************************************************
File:           BM25S3221-1.cpp
Author:         BESTMODULES
Description:    None
History：       None
V1.0.1   -- initial version; 2023-04-04; Arduino IDE : v1.8.19
******************************************************************/
#include "BM25S3221-1.h"

/**********************************************************
Description: Constructor
Parameters: statusPin: Status pin connection with Arduino or BMduino
            *theSerial: Serial object, if your board has multiple UART interfaces
Return: None
Others: None
**********************************************************/
BM25S3221_1::BM25S3221_1(uint8_t statusPin, HardwareSerial *theSerial)
{
  _softSerial = NULL;
  _statusPin = statusPin;
  _hardSerial = theSerial;
}

/**********************************************************
Description:  Constructor
Parameters: statusPin: Status pin connection with Arduino or BMduino
            rxPin : Receiver pin of the UART
            txPin : Send signal pin of UART
Return: None
Others: None
**********************************************************/
BM25S3221_1::BM25S3221_1(uint8_t statusPin, uint8_t rxPin, uint8_t txPin)
{
  _hardSerial = NULL;
  _statusPin = statusPin;
  _rxPin = rxPin;
  _txPin = txPin;
  _softSerial = new SoftwareSerial(_rxPin, _txPin);
}

/**********************************************************
Description: Module initial
Parameters: None
Return: None
Others: None
**********************************************************/
void BM25S3221_1::begin()
{
  if (_softSerial != NULL)
  {
    _softSerial->begin(BAUDRATE); // baud rate:9600
  }
  if (_hardSerial != NULL)
  {
    _hardSerial->begin(BAUDRATE); // baud rate:9600
  }
  pinMode(_statusPin, INPUT);
}
/**********************************************************
Description: Preheat Module(about 30 second)
Parameters: None
Return: None
Others: None
**********************************************************/
void BM25S3221_1::preheatCountdown()
{
  for (uint8_t i = 30; i > 0; i--)
  {
    delay(1030);
  }
}

/**********************************************************
Description: Read the current PM2.5 concentration value through PWM
Parameters: None
Return: PM2.5 concentration value,unit: μg/m³
Others: None
**********************************************************/
uint16_t BM25S3221_1::readPM25Value()
{
  uint16_t PM25Value = 0;
  int32_t highLevelTime = 0;
  highLevelTime = pulseIn(_statusPin, HIGH, 2000000); // timeout: 2 second
  if (highLevelTime >= 200)
  {
    PM25Value = highLevelTime / 1000;
  }
  return PM25Value;
}

/**********************************************************
Description: Use the command to read the dust concentration
Parameters: array[]:storing dust concentration values
Return: 0: read successed
        1: Verification failed
        2: Timeout error
Others: array[0]:PM1.0, array[1]:PM2.5, array[2]:PM10
**********************************************************/
uint8_t BM25S3221_1::readDustValue(uint16_t array[])
{
  uint16_t tmp = 0;
  uint8_t sendBuf[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
  uint8_t recBuf[9] = {0}, errFlag = 1;
  writeBytes(sendBuf);
  delay(1);
  errFlag = readBytes(recBuf);
  if (errFlag == 0)
  {
    if (recBuf[1] == 0x86)
    {
      tmp = recBuf[6];
      tmp = tmp << 8;
      array[0] = tmp + recBuf[7]; // PM1.0

      tmp = recBuf[2];
      tmp = tmp << 8;
      array[1] = tmp + recBuf[3]; // PM2.5

      tmp = recBuf[4];
      tmp = tmp << 8;
      array[2] = tmp + recBuf[5]; // PM10
      return 0;
    }
    else
    {
      array[0] = 0;
      array[1] = 0;
      array[2] = 0;
    }
  }
  return errFlag;
}

/**********************************************************
Description: Query whether the 32-byte data automatically uploaded by the module is received
Parameters: None
Return: true(1): Received
        false(0): Not received
Others: Only used in module automatic upload mode
**********************************************************/
bool BM25S3221_1::isInfoAvailable()
{
  uint8_t header[4] = {0x42, 0x4D, 0x00, 0x1C}; // Fixed code for first 4 bytes of 32-byte data
  uint8_t recBuf[32] = {0}, recLen = 32;
  uint8_t i, num = 0, readCnt = 0, failCnt = 0;
  uint8_t checkSumH = 0, checkSumL = 0;
  uint16_t checkSum = 0;
  bool isHeader = false, result = false;

  for (i = 0; i < recLen; i++)
  {
    _receiveBuffer[i] = 0;
  }

  /* Select hardSerial or softSerial according to the setting */
  if (_softSerial != NULL)
  {
    num = _softSerial->available();
  }
  else if (_hardSerial != NULL)
  {
    num = _hardSerial->available();
  }

  /* Serial buffer contains at least one 32-byte data */
  if (num >= recLen)
  {
    while (failCnt < 3) // Didn't read the required data twice, exiting the loop
    {
      /* Find 4-byte data header */
      for (i = 0; i < 4;)
      {
        if (_softSerial != NULL)
        {
          recBuf[i] = _softSerial->read();
        }
        else if (_hardSerial != NULL)
        {
          recBuf[i] = _hardSerial->read();
        }
        if (recBuf[i] == header[i])
        {
          isHeader = true; // Fixed code is correct
          i++;             // Next byte
        }
        else if ((recBuf[i] != header[i]) && (i > 0))
        {
          isHeader = false; // Next fixed code error
          failCnt++;
          break;
        }
        else if ((recBuf[i] != header[i]) && (i == 0))
        {
          readCnt++; // "0x42" not found, continue
        }
        if (readCnt > (num - recLen))
        {
          return false;
        }
      }

      /* Find the correct fixed code */
      if (isHeader)
      {
        for (checkSum = 0, i = 0; i < 4; i++)
        {
          checkSum += recBuf[i]; // Sum checkSum
        }
        for (i = 4; i < recLen; i++) // Read subsequent 28-byte data
        {
          if (_softSerial != NULL)
          {
            recBuf[i] = _softSerial->read();
          }
          else if (_hardSerial != NULL)
          {
            recBuf[i] = _hardSerial->read();
          }
          checkSum += recBuf[i]; // Sum checkSum
        }
        /* Calculate checkSum */
        checkSumH = 0;
        checkSumL = 0;
        checkSum = checkSum - recBuf[recLen - 2];
        checkSum = checkSum - recBuf[recLen - 1];
        checkSumH = checkSum >> 8;
        checkSumL = checkSum & 0x00ff;

        /* Compare whether the check code is correct */
        if ((checkSumH == recBuf[recLen - 2]) && (checkSumL == recBuf[recLen - 1]))
        {
          for (i = 0; i < recLen; i++)
          {
            _receiveBuffer[i] = recBuf[i]; // True, assign data to _recBuf[]
          }
          result = true;
          break; // Exit "while (failCnt < 3)" loop
        }
        else
        {
          failCnt++; // Error, failCnt++, return "while (failCnt < 3)" loop
        }
      }
    }
  }
  return result;
}

/**********************************************************
Description: Read the 32-byte data of sent by the module
Parameters: array[]: The array for storing the 32-byte module information
                     (refer to datasheet for meaning of each bit)
Return: None
Others: Use after "isInfoAvailable() == true"
**********************************************************/
void BM25S3221_1::readInfoPacket(uint8_t array[])
{
  for (uint8_t i = 0; i < 32; i++)
  {
    array[i] = _receiveBuffer[i];
    _receiveBuffer[i] = 0;
  }
}

/**********************************************************
Description: Set the mode of module uploading data(dust concentration)
Parameters: modeCode = 0x40: Automatically upload data
            modeCode = 0x41: Not automatically upload data
Return: None
Others: None
**********************************************************/
void BM25S3221_1::setUploadMode(uint8_t modeCode)
{
  uint8_t sendBuf[9] = {0xFF, 0x01, 0x78, modeCode, 0x00, 0x00, 0x00, 0x00, 0x00};
  for (uint8_t i = 1; i < 8; i++)
  {
    sendBuf[8] += sendBuf[i];
  }
  sendBuf[8] = ~sendBuf[8] + 1;
  writeBytes(sendBuf);
}

/**********************************************************
Description: Enter sleep mode
Parameters: None
Return: 0: Setup successed
        1: Verification failed
        2: Timeout error
        3: Setup failed
Others: None
**********************************************************/
uint8_t BM25S3221_1::sleep()
{
  uint8_t errFlag;
  uint8_t sendBuf[9] = {0xFF, 0x01, 0xA7, 0x01, 0x00, 0x00, 0x00, 0x00, 0x57};
  uint8_t recBuf[9] = {0};
  writeBytes(sendBuf);
  delay(20); // Waiting for module to receive data and reply
  errFlag = readBytes(recBuf);
  if (errFlag == CHECK_OK)
  {
    if (recBuf[2] == 0x01)
    {
      return 0;
    }
    else
    {
      return 3;
    }
  }
  else
  {
    return errFlag;
  }
}

/**********************************************************
Description: Exit sleep mode
Parameters: None
Return: 0: Setup successed
        1: Verification failed
        2: Timeout error
        3: Setup failed
Others: None
**********************************************************/
uint8_t BM25S3221_1::wakeUp()
{
  uint8_t errFlag;
  uint8_t sendBuf[9] = {0xFF, 0x01, 0xA7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x58};
  uint8_t recBuf[9] = {0};
  writeBytes(sendBuf);
  delay(45); // Waiting for module to receive data and reply
  errFlag = readBytes(recBuf);
  if (errFlag == CHECK_OK)
  {
    if (recBuf[2] == 0x01)
    {
      return 0;
    }
    else
    {
      return 3;
    }
  }
  else
  {
    return errFlag;
  }
}

/**********************************************************
Description: Write data through UART
Parameters: wBuf: The array for storing data to be sent(9 bytes)
            wLen:Length of data sent
Return: None
Others: None
**********************************************************/
void BM25S3221_1::writeBytes(uint8_t wbuf[], uint8_t wLen)
{
  /*Select hardSerial or softSerial according to the setting*/
  if (_softSerial != NULL)
  {
    while (_softSerial->available() > 0)
    {
      _softSerial->read();
    }
    _softSerial->write(wbuf, wLen);
    _softSerial->flush(); // Wait for the end of serial port data transmission
  }
  else
  {
    while (_hardSerial->available() > 0)
    {
      _hardSerial->read();
    }
    _hardSerial->write(wbuf, wLen);
    _hardSerial->flush(); // Wait for the end of serial port data transmission
  }
}

/**********************************************************
Description: Read data through UART
Parameters: rBuf: The array for storing Data to be sent
            rlen: Length of data to be read
            timeout: Receive timeout(unit: ms)
Return: 0: Verification succeeded
        1: Verification failed
        2: Timeout error
Others: None
**********************************************************/
uint8_t BM25S3221_1::readBytes(uint8_t rBuf[], uint8_t rLen, uint16_t timeout)
{
  uint16_t delayCnt = 0;
  uint8_t i = 0, checkSum = 0;

  /* Select SoftwareSerial Interface */
  if (_softSerial != NULL)
  {
    for (i = 0; i < rLen; i++)
    {
      delayCnt = 0;
      while (_softSerial->available() == 0)
      {
        if (delayCnt > timeout)
        {
          return TIMEOUT_ERROR; // Timeout error
        }
        delay(1); // delay 1ms
        delayCnt++;
      }
      rBuf[i] = _softSerial->read();
    }
  }
  /* Select HardwareSerial Interface */
  if (_hardSerial != NULL)
  {
    for (i = 0; i < rLen; i++)
    {
      delayCnt = 0;
      while (_hardSerial->available() == 0)
      {
        if (delayCnt > timeout)
        {
          return TIMEOUT_ERROR; // Timeout error
        }
        delay(1);
        delayCnt++;
      }
      rBuf[i] = _hardSerial->read();
    }
  }

  /* check Sum */
  for (i = 1; i < (rLen - 1); i++)
  {
    checkSum += rBuf[i];
  }
  checkSum = ~checkSum + 1;
  if (checkSum == rBuf[rLen - 1])
  {
    return CHECK_OK; // Check correct
  }
  else
  {
    return CHECK_ERROR; // Check error
  }
}
