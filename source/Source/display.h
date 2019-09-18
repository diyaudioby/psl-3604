//----------------------------------------------------------------------------

//Модуль поддержки LED-дисплея, заголовочный файл

//----------------------------------------------------------------------------

#ifndef DISPLAY_H
#define DISPLAY_H

//----------------------------------------------------------------------------

#include "systimer.h"
#include "sreg.h"

//----------------------------- Константы: -----------------------------------

#define POINT      0x80 //флаг децимальной точки
#define AUTO_SCALE 0x80 //флаг автомасштабирования

//маски знакомест:

#define ROW1POS1 0x01
#define ROW1POS2 0x02
#define ROW1POS3 0x04
#define ROW1POS4 0x08
#define ROW2POS1 0x10
#define ROW2POS2 0x20
#define ROW2POS3 0x40
#define ROW2POS4 0x80

enum PosName_t //имена позиций дисплея
{
  POS_1,
  POS_2,
  POS_3,
  POS_4,
  DIGS
};

enum RowName_t //имена строк дисплея
{
  ROW_V,
  ROW_I,
  ROWS
};

enum Blink_t //имена режимов мигания
{
  BLINK_NO   = 0x00,
  BLINK_V    = ROW1POS1 | ROW1POS2 | ROW1POS3 | ROW1POS4,
  BLINK_I    = ROW2POS1 | ROW2POS2 | ROW2POS3 | ROW2POS4,
  BLINK_VI   = BLINK_V | BLINK_I,
  BLINK_TIM  = ROW1POS3 | ROW1POS4 | BLINK_I,
  BLINK_NC   = ROW2POS4 //для мигания такая комбинация разрядов не используется
};

//----------------------------------------------------------------------------
//---------------------------- Класс TDisplay: -------------------------------
//----------------------------------------------------------------------------

class TDisplay
{
private:
  char SegDataV[DIGS];
  char SegDataI[DIGS];
  char Row;
  char Pos;
  char Conv(char d);
  char SetScan(char phase);
  Blink_t BlinkEn;
  bool BlinkOn;
  bool DispOn;
  TSreg Sreg;
  TSoftTimer *BlinkTimer;
public:
  TDisplay(void);
  void Execute(void); 
  bool LedCV;              //управление светодиодом CV
  bool LedCC;              //управление светодиодом CC
  bool LedOut;             //управление светодиодом OUT
  void Blink(Blink_t blink); //включение/выключение мигания
  bool LedFine;            //управление светодиодом FINE
  void Clear(void);        //очистка дисплея
  void Off(void);          //отключение дисплея (только цифры)
  void On(void);           //включение дисплея (только цифры)
  void Disable(void);      //отключение всей индикации
  void Enable(void);       //включение всей индикации
  void SetPos(char row, char pos); //установка позиции
  void PutChar(char ch);   //вывод символа
  void PutString(char *s); //вывод строки из RAM
  void PutString(const char *s); //вывод строки из ROM
  void PutIntF(int32_t v, char n, char d); //форматированный вывод числа
};

//----------------------------------------------------------------------------

extern TDisplay *Display;

//----------------------------------------------------------------------------

#endif
