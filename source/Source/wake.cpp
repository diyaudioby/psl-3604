//----------------------------------------------------------------------------

//Модуль релизации протокола Wake

//----------------------------------------------------------------------------

#include "main.h"
#include "wake.h"

//-------------------------- Опции компиляции: -------------------------------

#define TABLE_CRC     //включение табличного способа расчета CRC

//----------------------------- Константы: -----------------------------------

#define CRC_INIT 0xDE //начальное значение контрольной суммы
#define CRC_FEND 0x82 //начальное значение контрольной суммы с учетом FEND

//----------------------------------------------------------------------------
//----------------------------- Класс TWake ----------------------------------
//----------------------------------------------------------------------------

//----------------------------- Конструктор: ---------------------------------

TWake::TWake(char frame)
{
  Frame = frame;
  RxData = new char[frame + PTR_DAT + 1]; //буфер приема
  TxData = new char[frame + PTR_DAT + 1]; //буфер передачи
  Addr = 0;
  TxState = WST_DONE;
  RxState = WST_IDLE;
}

//--------------------- Вычисление контрольной суммы: ------------------------

void TWake::Do_Crc8(char b, char *crc)
{
#ifdef TABLE_CRC
  static const char CrcTable[256] =
  {
    0x00, 0x5E, 0xBC, 0xE2, 0x61, 0x3F, 0xDD, 0x83, 
    0xC2, 0x9C, 0x7E, 0x20, 0xA3, 0xFD, 0x1F, 0x41, 
    0x9D, 0xC3, 0x21, 0x7F, 0xFC, 0xA2, 0x40, 0x1E, 
    0x5F, 0x01, 0xE3, 0xBD, 0x3E, 0x60, 0x82, 0xDC, 
    0x23, 0x7D, 0x9F, 0xC1, 0x42, 0x1C, 0xFE, 0xA0, 
    0xE1, 0xBF, 0x5D, 0x03, 0x80, 0xDE, 0x3C, 0x62, 
    0xBE, 0xE0, 0x02, 0x5C, 0xDF, 0x81, 0x63, 0x3D, 
    0x7C, 0x22, 0xC0, 0x9E, 0x1D, 0x43, 0xA1, 0xFF, 
    0x46, 0x18, 0xFA, 0xA4, 0x27, 0x79, 0x9B, 0xC5, 
    0x84, 0xDA, 0x38, 0x66, 0xE5, 0xBB, 0x59, 0x07, 
    0xDB, 0x85, 0x67, 0x39, 0xBA, 0xE4, 0x06, 0x58, 
    0x19, 0x47, 0xA5, 0xFB, 0x78, 0x26, 0xC4, 0x9A, 
    0x65, 0x3B, 0xD9, 0x87, 0x04, 0x5A, 0xB8, 0xE6, 
    0xA7, 0xF9, 0x1B, 0x45, 0xC6, 0x98, 0x7A, 0x24, 
    0xF8, 0xA6, 0x44, 0x1A, 0x99, 0xC7, 0x25, 0x7B, 
    0x3A, 0x64, 0x86, 0xD8, 0x5B, 0x05, 0xE7, 0xB9, 
    0x8C, 0xD2, 0x30, 0x6E, 0xED, 0xB3, 0x51, 0x0F, 
    0x4E, 0x10, 0xF2, 0xAC, 0x2F, 0x71, 0x93, 0xCD, 
    0x11, 0x4F, 0xAD, 0xF3, 0x70, 0x2E, 0xCC, 0x92, 
    0xD3, 0x8D, 0x6F, 0x31, 0xB2, 0xEC, 0x0E, 0x50, 
    0xAF, 0xF1, 0x13, 0x4D, 0xCE, 0x90, 0x72, 0x2C, 
    0x6D, 0x33, 0xD1, 0x8F, 0x0C, 0x52, 0xB0, 0xEE, 
    0x32, 0x6C, 0x8E, 0xD0, 0x53, 0x0D, 0xEF, 0xB1, 
    0xF0, 0xAE, 0x4C, 0x12, 0x91, 0xCF, 0x2D, 0x73, 
    0xCA, 0x94, 0x76, 0x28, 0xAB, 0xF5, 0x17, 0x49, 
    0x08, 0x56, 0xB4, 0xEA, 0x69, 0x37, 0xD5, 0x8B, 
    0x57, 0x09, 0xEB, 0xB5, 0x36, 0x68, 0x8A, 0xD4, 
    0x95, 0xCB, 0x29, 0x77, 0xF4, 0xAA, 0x48, 0x16, 
    0xE9, 0xB7, 0x55, 0x0B, 0x88, 0xD6, 0x34, 0x6A, 
    0x2B, 0x75, 0x97, 0xC9, 0x4A, 0x14, 0xF6, 0xA8, 
    0x74, 0x2A, 0xC8, 0x96, 0x15, 0x4B, 0xA9, 0xF7, 
    0xB6, 0xE8, 0x0A, 0x54, 0xD7, 0x89, 0x6B, 0x35 
  };
  *crc = CrcTable[*crc ^ b]; //табличное вычисление
#else  
  for(char i = 0; i < 8; b = b >> 1, i++) //вычисление в цикле
    if((b ^ *crc) & 1) *crc = ((*crc ^ 0x18) >> 1) | 0x80;
     else *crc = (*crc >> 1) & ~0x80;
#endif     
}

//----------------------------------------------------------------------------
//----------------------------- Прием пакета: --------------------------------
//----------------------------------------------------------------------------

//----------------------------- Прием байта: ---------------------------------

void TWake::Rx(char data)
{
  if(RxState != WST_DONE)            //если прием разрешен
  {
    if(data == FEND)                 //обнаружен FEND (из любого состояния)
    {
      RxState = WST_ADD;             //переход к приему адреса
      RxPtr = RxData;                //указатель на начало буфера
      RxStuff = 0; return;           //нет дестаффинга
    }
    if(data == FESC)                 //принят FESC,
    { RxStuff = 1; return; }         //начало дестафинга
    if(RxStuff)                      //если идет дестафинг,
    {
      if(data == TFESC)              //если принят TFESC,
        data = FESC;                 //замена его на FESC
      else if(data == TFEND)         //если принят TFEND,
        data = FEND;                 //замена его на FEND
        else { RxState = WST_IDLE; return; } //иначе ошибка стаффинга
      RxStuff = 0;                   //дестаффинг закончен
    }
    switch(RxState)
    {
    case WST_ADD:                    //прием адреса
        RxState = WST_CMD;           //далее - прием команды
        if(data & 0x80)              //если принят адрес,
        {
          data &= ~0x80;             //восстановление значения адреса
          if(data != Addr)           //адрес не совпал,
          { RxState = WST_IDLE; return; } //переход к поиску FEND
          break;                     //переход к сохранению адреса
        }
        else *RxPtr++ = 0;           //сохранение нулевого адреса
    case WST_CMD:                    //прием кода команды
        RxState = WST_LNG;           //далее - прием длины пакета
        break;                       //переход к сохранению команды
    case WST_LNG:                    //идет прием длины пакета
        RxState = WST_DATA;          //далее - прием данных            
        if(data > Frame) data = Frame;   //ограничение длины пакета
        RxEnd = RxData + PTR_DAT + data; //указатель на конец данных
        break;
    case WST_DATA:                   //идет прием данных
        if(RxPtr == RxEnd)           //если все данные и CRC приняты,
          RxState = WST_DONE;        //прием окончен
        break;
    default: return;
    }
    *RxPtr++ = data;                 //сохранение данных в буфере
  }
}

//--------------------- Возвращает текущий код команды: ----------------------

char TWake::GetCmd(void)
{
  char cmd = CMD_NOP;
  if(RxState == WST_DONE)            //если прием пакета завершен
  {
    RxCount = RxEnd - RxData - PTR_DAT; //количество принятых байт данных
    char crc = CRC_FEND;             //инициализация CRC
    RxPtr = RxData;                  //указатель на начало буфера
    if(!*RxPtr) RxPtr++;             //если адрес нулевой, пропускаем его
    while(RxPtr <= RxEnd)            //для всего буфера
      Do_Crc8(*RxPtr++, &crc);       //считаем CRC 
    RxPtr = RxData + PTR_CMD;        //указатель на код команды
    if(!crc) cmd = *RxPtr;           //если CRC совпадает, код команды
      else  cmd = CMD_ERR;           //иначе код ошибки
    TxCount = 0;                     //обнуление количества байт для передачи
    RxPtr = RxData + PTR_DAT;        //указатель приема на данные
    TxPtr = TxData + PTR_DAT;        //указатель передачи на данные
  }
  return(cmd);
}

//------------------- Возвращает количество принятых байт: -------------------

char TWake::GetRxCount(void)
{
  return(RxCount);
}

//----------------- Устанавливает указатель на буфер приема: -----------------

void TWake::SetRxPtr(char p)
{
  if(p < Frame)
    RxPtr = RxData + PTR_DAT + p;
}

//--------------------- Читает указатель буфера приема: ----------------------

char TWake::GetRxPtr(void)
{
  return(RxPtr - RxData - PTR_DAT);
}

//---------------------- Читает байт из буфера приема: -----------------------

char TWake::GetByte(void)
{
  return(*RxPtr++);
}

//--------------------- Читает слово из буфера приема: -----------------------

int16_t TWake::GetWord(void)
{
  char l = *RxPtr++;
  char h = *RxPtr++;
  return(WORD(h, l));
}

//----------------- Читает двойное слово из буфера приема: -------------------

int32_t TWake::GetDWord(void)
{
  char b1 = *RxPtr++;
  char b2 = *RxPtr++;
  char b3 = *RxPtr++;
  char b4 = *RxPtr++;
  return(DWORD(b4, b3, b2, b1));
}

//--------------------- Читает данные из буфера приема: ----------------------

void TWake::GetData(char *d, char count)
{
  for(char i = 0; i < count; i++)
    *d++ = *RxPtr++;
}

//----------------------------------------------------------------------------
//-------------------------- Передача пакета: --------------------------------
//----------------------------------------------------------------------------

//---------------- Устанавливает указатель на буфер передачи: ----------------

void TWake::SetTxPtr(char p)
{
  if(p < Frame)
    TxPtr = TxData + PTR_DAT + p;
}

//-------------------- Читает указатель буфера передачи: ---------------------

char TWake::GetTxPtr(void)
{
  return(TxPtr - TxData - PTR_DAT);
}

//--------------------- Помещает байт в буфер передачи: ----------------------

void TWake::AddByte(char b)
{
  if(TxPtr < TxData + PTR_DAT + Frame)
    *TxPtr++ = b;
}

//-------------------- Помещает слово в буфер передачи: ----------------------

void TWake::AddWord(int16_t w)
{
  if(TxPtr < TxData + PTR_DAT + Frame - 1)
  {
    *TxPtr++ = LO(w);
    *TxPtr++ = HI(w);
  }
}

//---------------- Помещает двойное слово в буфер передачи: ------------------

void TWake::AddDWord(int32_t dw)
{
  if(TxPtr < TxData + PTR_DAT + Frame - 3)
  {
    *TxPtr++ = BYTE1(dw);
    *TxPtr++ = BYTE2(dw);
    *TxPtr++ = BYTE3(dw);
    *TxPtr++ = BYTE4(dw);
  }
}

//-------------------- Помещает данные в буфер передачи: ---------------------

void TWake::AddData(char *d, char count)
{
  if(TxPtr <= (TxData + PTR_DAT + Frame) - count)
    for(char i = 0; i < count; i++)
      *TxPtr++ = *d++;
}

//-------------------------- Начало передачи пакета: -------------------------

void TWake::TxStart(char cmd, char &data)
{
  TxEnd = TxPtr;                     //указатель конца пакета
  TxCount = TxPtr - TxData - PTR_DAT; //количество байт для передачи
  TxPtr = TxData;                    //указатель на начало буфера
  *TxPtr++ = Addr | 0x80;            //добавление в буфер адреса
  *TxPtr++ = cmd;                    //добавление в буфер кода команды
  *TxPtr = TxCount;                  //добавление в буфер размера пакета
  char crc = CRC_FEND;               //инициализация CRC
  TxPtr = TxData;                    //указатель на начало буфера
  if(!Addr) TxPtr++;                 //пропускаем нулевой адрес
  while(TxPtr < TxEnd)
    Do_Crc8(*TxPtr++, &crc);         //расчет CRC для всего буфера
  *TxPtr = crc;                      //добавление в буфер CRC
  TxPtr = TxData;                    //указатель на начало буфера
  if(!Addr) TxPtr++;                 //пропускаем нулевой адрес
  TxStuff = 0;                       //нет стаффинга
  RxState = WST_IDLE;                //разрешение приема пакета
  TxState = WST_DATA;                //состояние передачи данных
  data = FEND;
}

//---------------------------- Передача байта: -------------------------------

bool TWake::Tx(char &data)
{
  if(TxState == WST_DATA)            //если идет передача данных,
  {
    data = *TxPtr++;                 //то чтение байта из буфера
    if(data == FEND || data == FESC) //попытка передать FEND или FESC,
      if(!TxStuff)                   //нужен стаффинг
      {
        data = FESC;                 //передача FESC
        TxStuff = 1;                 //начало стаффинга
        TxPtr--;                     //возврат к тому же байту
      }
      else
      {
        if(data == FEND) data = TFEND; //передача TFEND
          else data = TFESC;         //или TFESC
        TxStuff = 0;                 //конец стаффинга
      }
    if(TxPtr > TxEnd)                //если конец буфера достигнут,
      TxState = WST_CRC;             //передается CRC
    return(1);
  }
  else                               //если передача закончена,
  {
    TxState = WST_DONE;              //передача пакета закончена
    return(0);
  }
}

//------------------- Определение конца передачи пакета: ---------------------

bool TWake::AskTxEnd(void)
{
  return(TxState == WST_DONE);
}

//----------------------------------------------------------------------------
