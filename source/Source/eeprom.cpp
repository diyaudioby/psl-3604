//----------------------------------------------------------------------------

//Модуль поддержки внешней EEPROM

//----------------------- Используемые ресурсы: ------------------------------

//Используется внешняя микросхема EEPROM типа 24С04, которая подключена
//к пинам SCL (PB8), SDA (PB9). Интерфейс I2C эмулируется программно.
//К сожалению, аппаратный I2C1 (remap) использовать нельзя,
//так как он конфликтует с SPI1 (remap): на выводе PB5
//всегда единица (см. errata).
//Для формирования таймингов шины используется таймер TIM16.

//----------------------------------------------------------------------------

#include "main.h"
#include "i2csw.h"
#include "eeprom.h"

//----------------------------- Константы: -----------------------------------

#define I2C_ADDR  0xA0 //адрес микросхемы EEPROM
#define EEPROM_WRTM 25 //длительность цикла записи, мс

#define EE_SIGNATURE 0xBED3

//----------------------------------------------------------------------------
//----------------------------- Класс TEEPROM: -------------------------------
//----------------------------------------------------------------------------

//----------------------------- Инициализация: -------------------------------

uint16_t TEeprom::EeTop;
uint8_t TEeprom::Error;
uint8_t TEeprom::ByteAddress;
uint8_t TEeprom::PageAddress;

void TEeprom::Init(void)
{
  TI2Csw::Init();
  Error = ER_NONE;
  EeTop = 0;  //начало свободной области
}

//------------- Запись адреса с ожиданием готовности EEPROM: -----------------

//addr - адрес слова
//возвращает true если обнаружен ответ EEPROM

bool TEeprom::SetAddress(uint16_t addr)
{
  bool ask;
  ByteAddress = (addr << 1) & 0xFE;
  PageAddress = (addr >> 6) & 0x0E;
  TSysTimer::TimeoutStart_ms(EEPROM_WRTM);
  do
  {
    TI2Csw::Start();
    ask = TI2Csw::Write(I2C_ADDR | PageAddress);
  }
  while(!ask && !TSysTimer::TimeoutOver_ms());
  if(ask)
  {
    TI2Csw::Write(ByteAddress);
  }
  else
  {
    TI2Csw::Stop();
    Error |= ER_ASK;
  }
  return(ask);
}

//---------------------- Чтение данных из EEPROM: ----------------------------

//addr - адрес слова
//data - слово данных для записи в EEPROM

uint16_t TEeprom::Read(uint16_t addr)
{
  if(!SetAddress(addr)) return(0);
  TI2Csw::Stop();
  TI2Csw::Start();
  TI2Csw::Write(I2C_ADDR | PageAddress | I2C_RD);
  char data_l = TI2Csw::Read(I2C_ACK);
  char data_h = TI2Csw::Read(I2C_NACK);
  return(WORD(data_h, data_l));
}

//----------------------- Запись данных в EEPROM: ----------------------------

//addr - адрес слова
//data - слово данных для записи в EEPROM

void TEeprom::Write(uint16_t addr, uint16_t data)
{
  if(!SetAddress(addr)) return;
  TI2Csw::Write(LO(data));
  TI2Csw::Write(HI(data));
  TI2Csw::Stop();
}

//--------------------- Обновление данных в EEPROM: --------------------------

//addr - адрес слова
//data - слово данных для записи в EEPROM
//Запись производится только в том случае, если новые данные отличаются.

void TEeprom::Update(uint16_t addr, uint16_t data)
{
  uint16_t d = Read(addr);
  if(data != d)
    Write(addr, data);
}

//----------------------------------------------------------------------------
//--------------------------- Класс TEeSection: ------------------------------
//----------------------------------------------------------------------------

//Простая секция EEPROM, валидность данных
//устанавливается проверкой сигнатуры.

//----------------------------- Конструктор: ---------------------------------

TEeSection::TEeSection(uint16_t size)
{
  Base = EeTop;       //начало секции
  Size = size;        //размер секции
  Sign = Base + size; //смещение сигнатуры
  EeTop = Sign + 1;   //новое начало свободного места EEPROM
  Valid = 1;
  if(EeTop > EEPROM_SIZE)
  {
    TEeprom::Error |= ER_ALLOC;
    Valid = 0;
  }
  else if(TEeprom::Read(Sign) != EE_SIGNATURE)
  {
    TEeprom::Error |= ER_SIGN;
    Valid = 0;
  }
  if(!Valid) TEeprom::Error |= ES_PLAIN;
}

//------------------------- Установка валидности: ----------------------------

void TEeSection::Validate(void)
{
  TEeprom::Update(Sign, EE_SIGNATURE);
  Valid = 1;
}

//------------------------- Чтение данных секции: ----------------------------

uint16_t TEeSection::Read(uint16_t addr)
{
  return(TEeprom::Read(Base + addr));
}

//------------------------- Запись данных секции: ----------------------------

void TEeSection::Write(uint16_t addr, uint16_t data)
{
  if(addr < Size)
    TEeprom::Write(Base + addr, data);
}

//----------------------- Обновление данных секции: --------------------------

void TEeSection::Update(uint16_t addr, uint16_t data)
{
  if(addr < Size)
    TEeprom::Update(Base + addr, data);
}

//----------------------------------------------------------------------------
//--------------------------- Класс TCrcSection: -----------------------------
//----------------------------------------------------------------------------

//Секция EEPROM повышенной надежности, валидность данных
//устанавливается проверкой сигнатуры и CRC.

//----------------------------- Конструктор: ---------------------------------

TCrcSection::TCrcSection(uint16_t size) : TEeSection(size)
{
  Crc = EeTop;     //смещение CRC
  EeTop = Crc + 1; //новое начало свободного места EEPROM
  if(EeTop > EEPROM_SIZE)
  {
    TEeprom::Error |= ER_ALLOC;
    Valid = 0;
  }
  else if(TEeprom::Read(Crc) != GetCRC())
  {
    TEeprom::Error |= ER_CRC;
    Valid = 0;
  }
  if(!Valid) TEeprom::Error |= ES_CRC;
}

//------------------------------ Расчет CRC: ---------------------------------

uint16_t TCrcSection::GetCRC(void)
{
  RCC->AHBENR |= RCC_AHBENR_CRCEN;
  CRC->CR = CRC_CR_RESET;
  for(uint16_t i = 0; i < Size; i++)
    CRC->DR = (uint32_t)TEeprom::Read(Base + i);
  uint16_t result = CRC->DR;
  RCC->AHBENR &= ~RCC_AHBENR_CRCEN;
  return(result);
}

//------------------------- Установка валидности: ----------------------------

void TCrcSection::Validate(void)
{
  TEeSection::Validate();
  TEeprom::Update(Crc, GetCRC());
  Valid = 1;
}

//----------------------------------------------------------------------------
//--------------------------- Класс TRingSection: ----------------------------
//----------------------------------------------------------------------------

//Секция EEPROM повышенного ресурса, весь объем секции отводится для
//хранения одной переменной. Организуется кольцевой буфер, новое
//значение записывается по следующему адресу, предыдущее значение
//стирается (записывается 0xFFFF). Валидность данных устанавливается
//проверкой сигнатуры.

//----------------------------- Конструктор: ---------------------------------

TRingSection::TRingSection(uint16_t size) : TEeSection(size)
{
  Ptr = 0;
  //поиск данных:
  if(Valid)
    while((Ptr < Size) && (TEeprom::Read(Base + Ptr) == 0xFFFF))
      Ptr++;
  if(Ptr == Size) Ptr = 0;
  if(!Valid) TEeprom::Error |= ES_RING;
}

//------------------------- Чтение данных секции: ----------------------------

uint16_t TRingSection::Read(void)
{
  return(TEeprom::Read(Base + Ptr));
}

//------------------------- Запись данных секции: ----------------------------

void TRingSection::Write(uint16_t data)
{
  uint16_t pre = Ptr;
  if(++Ptr == Size) Ptr = 0;
  TEeprom::Write(Base + Ptr, data);
  TEeprom::Write(Base + pre, 0xFFFF);
}

//----------------------- Обновление данных секции: --------------------------

void TRingSection::Update(uint16_t data)
{
  if(Read() != data)
    Write(data);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
