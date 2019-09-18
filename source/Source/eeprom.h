//----------------------------------------------------------------------------

//Модуль поддержки внешней EEPROM, заголовочный файл

//----------------------------------------------------------------------------

#ifndef EEPROM_H
#define EEPROM_H

//----------------------------------------------------------------------------

#include "systimer.h"

//----------------------------- Константы: -----------------------------------

#define EEPROM_SIZE 512 //объем микросхемы памяти 24С04, байт

//Флаги ошибок EEPROM:

enum EError_t
{
  ER_NONE  = 0x00, //нет ошибки
  ER_SIGN  = 0x01, //ошибка сигнатуры
  ER_CRC   = 0x02, //ошибка CRC
  ER_ALLOC = 0x04, //ошибка выделения памяти
  ER_ASK   = 0x08, //нет ответа EEPROM
  ES_PLAIN = 0x00, //простая секция
  ES_CRC   = 0x10, //секция с защитой CRC
  ES_RING  = 0x20  //ring-секция
};

//----------------------------------------------------------------------------
//---------------------------- Класс TEeprom: --------------------------------
//----------------------------------------------------------------------------

class TEeprom
{
private:
  static bool SetAddress(uint16_t addr);
  static uint8_t ByteAddress;
  static uint8_t PageAddress;
protected:
  static uint16_t EeTop;
  static uint16_t Read(uint16_t addr);
  static void Write(uint16_t addr, uint16_t data);
  static void Update(uint16_t addr, uint16_t data);
public:
  static void Init(void);
  static uint8_t Error;
};

//----------------------------------------------------------------------------
//--------------------------- Класс TEeSection: ------------------------------
//----------------------------------------------------------------------------

class TEeSection : public TEeprom
{
private:
protected:
  uint16_t Base;
  uint16_t Size;
  uint16_t Sign;
public:
  TEeSection(uint16_t size);
  bool Valid;
  virtual void Validate(void);
  uint16_t Read(uint16_t addr);
  void Write(uint16_t addr, uint16_t data);
  void Update(uint16_t addr, uint16_t data);
};

//----------------------------------------------------------------------------
//--------------------------- Класс TCrcSection: -----------------------------
//----------------------------------------------------------------------------

class TCrcSection : public TEeSection
{
private:
  uint16_t Crc;
  uint16_t GetCRC(void);
protected:
public:
  TCrcSection(uint16_t size);
  virtual void Validate(void);
};

//----------------------------------------------------------------------------
//--------------------------- Класс TRingSection: ----------------------------
//----------------------------------------------------------------------------

class TRingSection : public TEeSection
{
private:
  uint16_t Ptr;
protected:
public:
  TRingSection(uint16_t size);
  //Не очень хорошо, что здесь объявляются дополнительные
  //функции. Если в родительском классе функции с этими именами
  //сделать виртуальными, то при переопределении будут перекрываться
  //все сигнатуры, т.е. родительские функции с другой сигнатурой
  //тоже видны не будут. Здесь это и нужно, но компилятор выдает warning.
  uint16_t Read(void);
  void Write(uint16_t data);
  void Update(uint16_t data);
};

//----------------------------------------------------------------------------

#endif
