//----------------------------------------------------------------------------

//Модуль поддержки LED-дисплея

//----------------------- Используемые ресурсы: ------------------------------

//Используется дисплей из 7-сегментных LED-индикаторов.
//Организация дисплея - 2 строки по 4 разряда.
//Индикация - динамическая, коэффициент мультиплексирования 4.
//Сегменты и скан-линии включаются ВЫСОКИМ уровнем.
//Дополнительно используются 4 светодиода:
//LED_CC, LED_CV, LED_OUT и LED_FINE.
//Светодиоды включаются ВЫСОКИМ уровнем.
//Индикаторы и светодиоды подключены через внешний сдвиговый
//регистр, доступ к которому осуществляется через класс TSreg.

//----------------------------------------------------------------------------

#include "main.h"
#include "display.h"

//----------------------------- Константы: -----------------------------------

#define BLINK_TM 400 //период мигания, мс
#define BLDEL_TM 800 //задержка начала мигания, мс

//Расположение битов сегментов в регистрах индикатора:

enum { SEG_F, SEG_A, SEG_B, SEG_G, SEG_C, SEG_H, SEG_E, SEG_D }; 

//Расположение битов скан-линий и светодиодов в регистре:

enum { LOT, SC1, SC2, SC3, SC4, LFN, LCC, LCV };

#define SCAN1    (1 << SC1)   //линия сканирования 1
#define SCAN2    (1 << SC2)   //линия сканирования 2
#define SCAN3    (1 << SC3)   //линия сканирования 3
#define SCAN4    (1 << SC4)   //линия сканирования 4

#define LED_CV   (1 << LCV)   //светодиод "CV"
#define LED_CC   (1 << LCC)   //светодиод "CC"
#define LED_OUT  (1 << LOT)   //светодиод "OUT ON/OFF"
#define LED_FINE (1 << LFN)   //светодиод "FINE"

#define SEG_PNT  (1 << SEG_H) //сегмент точки

//----------------------------------------------------------------------------
//---------------------------- Класс TDisplay --------------------------------
//----------------------------------------------------------------------------

//----------------------------- Конструктор: ---------------------------------

TDisplay::TDisplay(void)
{
  Sreg.Init();
  Sreg = 0;
  Sreg.Enable();
  BlinkTimer = new TSoftTimer();
  LedCV = 0;
  LedCC = 0;
  LedOut = 0;
  LedFine = 0;
  BlinkEn = BLINK_NO;
  BlinkOn = 1;
  DispOn = 1;
  Clear();
}

//------------ Преобразование кода символа в 7-сегментный код: ---------------

char TDisplay::Conv(char d)
{
  //маски сегментов:
  enum
  {
    A = (1 << SEG_A),
    B = (1 << SEG_B),
    C = (1 << SEG_C),
    D = (1 << SEG_D),
    E = (1 << SEG_E),
    F = (1 << SEG_F),
    G = (1 << SEG_G),
    H = (1 << SEG_H),
  };
  
  //Таблица знакогенератора:
  //для цифр используются коды 0..F,
  //для букв и специальных символов
  //используются коды, совпадающие с кодировкой Win-1251.
  //Расположение сегментов на индикаторе:
  //
  //    -- A --
  //   |       |
  //   F       B
  //   |       |
  //    -- G --
  //   |       |
  //   E       C
  //   |       |
  //    -- D --  (H)
  
  static const char Font[]=
  {
    A | B | C | D | E | F    , // 00H - "0"
    B | C                    , // 01H - "1"
    A | B | D | E | G        , // 02H - "2"
    A | B | C | D | G        , // 03H - "3"
    B | C | F | G            , // 04H - "4"
    A | C | D | F | G        , // 05H - "5"
    A | C | D | E | F | G    , // 06H - "6"
    A | B | C                , // 07H - "7"
    A | B | C | D | E | F | G, // 08H - "8"
    A | B | C | D | F | G    , // 09H - "9"
    A | B | C | E | F | G    , // 0AH - "A"
    C | D | E | F | G        , // 0BH - "b"
    D | E | G                , // 0CH - "c"
    B | C | D | E | G        , // 0DH - "d"
    A | D | E | F | G        , // 0EH - "E"
    A | E | F | G            , // 0FH - "F"
    A | C | D | E | F        , // 10H - "G"
    B | C | E | F | G        , // 11H - "H"
    D | E | F                , // 12H - "L"
    C | E | G                , // 13H - "n"
    C | D | E | G            , // 14H - "o"
    A | B | E | F | G        , // 15H - "P"
    E | G                    , // 16H - "r"
    D | E | F | G            , // 17H - "t"
    C | D | E                , // 18H - "u"
    B | C | D | E | F        , // 19H - "U"
    B | C | D | F | G        , // 1AH - "Y"
    A | D | E | F            , // 1BH - "С"
    A | B | F | G            , // 1CH - "°"
    G                        , // 1DH - "-"
    0                          // 1EH - " "
  };
  //перекодировка символов:
  if(d >= 0x30 && d <= 0x39) d -= 0x30;
  if(d > 0x0F)
  {
    switch(d)
    {
    case 'A': d = 0x0A; break; // "A"
    case 'b': d = 0x0B; break; // "b"
    case 'c': d = 0x0C; break; // "c"
    case 'C': d = 0x1B; break; // "C"
    case 'd': d = 0x0D; break; // "d"
    case 'E': d = 0x0E; break; // "E"
    case 'F': d = 0x0F; break; // "F"
    case 'G': d = 0x10; break; // "G"
    case 'H': d = 0x11; break; // "H"
    case 'I': d = 0x01; break; // "I"
    case 'L': d = 0x12; break; // "L"
    case 'n': d = 0x13; break; // "n"
    case 'o': d = 0x14; break; // "o"
    case 'O': d = 0x00; break; // "O"
    case 'P': d = 0x15; break; // "P"
    case 'r': d = 0x16; break; // "r"
    case 'S': d = 0x05; break; // "S"
    case 't': d = 0x17; break; // "t"
    case 'u': d = 0x18; break; // "u"
    case 'U': d = 0x19; break; // "U"
    case 'Y': d = 0x1A; break; // "Y"
    case '*': d = 0x1C; break; // "°"
    case ' ': d = 0x1E; break; // " "
    default:  d = 0x1D;        // "-"
    }
  }
  return(Font[d]); //возвращаем 7-сегментный код
}

//------------------------ Сканирование дисплея: -----------------------------

void TDisplay::Execute(void)
{
  if(TSysTimer::Tick)
  {
    static char Phase = POS_1;
    char Scans = SetScan(Phase);
    char DataV = SegDataV[Phase];
    char DataI = SegDataI[Phase];

    if(BlinkEn)
    {
      if(BlinkTimer->Over())
      {
        BlinkTimer->Start(BLINK_TM / 2);
        BlinkOn = !BlinkOn;
      }
      if(!BlinkOn)
      {
        switch(Phase)
        {
        case POS_1:
          if(BlinkEn & ROW1POS1) DataV = 0;
          if(BlinkEn & ROW2POS1) DataI = 0;
          break;
        case POS_2:
          if(BlinkEn & ROW1POS2) DataV = 0;
          if(BlinkEn & ROW2POS2) DataI = 0;
          break;
        case POS_3:
          if(BlinkEn & ROW1POS3) DataV = 0;
          if(BlinkEn & ROW2POS3) DataI = 0;
          break;
        case POS_4:
          if(BlinkEn & ROW1POS4) DataV = 0;
          if(BlinkEn & ROW2POS4) DataI = 0;
          break;
        }
      }
    }

    Sreg = DWORD(0, Scans, DataV, DataI);
    if(++Phase == DIGS) Phase = POS_1;
  }
}

//------------ Включение требуемых скан-линий и светодиодов: -----------------

inline char TDisplay::SetScan(char phase)
{
  char c = 0;
  if(DispOn)
  {
    if(phase == POS_1) c |= SCAN1;
    if(phase == POS_2) c |= SCAN2;
    if(phase == POS_3) c |= SCAN3;
    if(phase == POS_4) c |= SCAN4;
  }
  if(LedCV)   c |= LED_CV;
  if(LedCC)   c |= LED_CC;
  if(LedOut)  c |= LED_OUT;
  if(LedFine) c |= LED_FINE;
  return(c);
}

//--------------------- Включение/выключение мигания: ------------------------

void TDisplay::Blink(Blink_t blink)
{
  if(blink != BLINK_NC) BlinkEn = blink;
  BlinkTimer->Start(BLINK_TM / 2);
  BlinkOn = 0; //мигание начинается с гашения
}
    
//--------------------------- Очистка дисплея: -------------------------------

void TDisplay::Clear(void)
{
  for(char i = 0; i < DIGS; i++)
  {
    SegDataV[i] = 0;
    SegDataI[i] = 0;
  }
  Row = ROW_V; Pos = POS_1;
}

//-------------------------- Отключение дисплея: -----------------------------

void TDisplay::Off(void)
{
  DispOn = 0;
  char Scans = SetScan(0);
  Sreg = DWORD(0, Scans, 0, 0);
}

//--------------------------- Включение дисплея: -----------------------------

void TDisplay::On(void)
{
  DispOn = 1;
}

//----------------- Отключение дисплея вместе со светодиодами: ---------------

void TDisplay::Disable(void)
{
  Sreg.Disable();
}

//----------------- Включение дисплея вместе со светодиодами: ----------------

void TDisplay::Enable(void)
{
  Sreg.Enable();
}

//-------------------------- Установка позиции: ------------------------------

void TDisplay::SetPos(char row, char pos)
{
  if(row < ROWS && pos < DIGS)
  {
    Row = row; Pos = pos;
  }
}

//--------------------------- Вывод символа: ---------------------------------

void TDisplay::PutChar(char ch)
{
  if(((BlinkEn & BLINK_V) && (Row == ROW_V)) || //приостановка мигания V
     ((BlinkEn & BLINK_I) && (Row == ROW_I)))   //приостановка мигания I
  {
    BlinkOn = 1;
    BlinkTimer->Start(BLDEL_TM);
  }
  char s = ((ch & POINT)? SEG_PNT : 0) + Conv(ch & ~POINT);
  if(Row == ROW_V) SegDataV[Pos] = s;
  if(Row == ROW_I) SegDataI[Pos] = s;
  if(++Pos == DIGS)
  {
    Pos = POS_1;
    if(++Row == ROWS)
      Row = ROW_V;
  }
}

//------------------ Вывод null-terminated string из RAM: --------------------

void TDisplay::PutString(char *s)
{
  while(*s) PutChar(*s++);
}

//------------------ Вывод null-terminated string из ROM: --------------------

void TDisplay::PutString(const char *s)
{
  while(*s) PutChar(*s++);
}

//---------- Форматированный вывод целого числа с условной точкой: -----------

//n - общее количество выводимых цифр (с учетом
//символа "минус" для отрицательных значений),
//d - количество цифр после точки.
//Всегда выводится n разрядов, остальные разряды не изменяются.

//Примеры:
//v =    1,  n = 4, d = 0:     1
//v =    1,  n = 4, d = 3: 0.001
//v = 9999,  n = 4, d = 3: 9.999
//v = 10000, n = 4, d = 3: -.---
//v =    -1, n = 4, d = 3: -.---
//v =    -1, n = 4, d = 0:    -1
//v =  -999, n = 4, d = 2: -9.99
//v =   999, n = 3, d = 1: 99.9

//Пример автомасштабирования (n = 4, d = 3 + AUTO_SCALE):
//v =        9: 0.009
//v =     9999: 9.999
//v =    99999: 99.99
//v =   999999: 999.9
//v =  9999999:  9999
//v = 10000000:  ----
//v =       -9: -0.00
//v =      -99: -0.09
//v =    99999: -99.9
//v =   999999:  -999
//v =  1000000:  ----

#define MAX_DIGITS 10 //длина буфера для 32-битного числа

void TDisplay::PutIntF(int32_t v, char n, char d)
{
  char buff[MAX_DIGITS];
  char i;
  //проверка знака числа:
  bool minus = v < 0;
  if(minus) v = -v;
  //преобразование числа в строку:
  for(i = 0; i < MAX_DIGITS; i++)
  {
    char dig = v % 10;
    v /= 10;
    buff[i] = dig;
  }
  //добавление точки:
  char pnt = d & ~AUTO_SCALE;
  if(pnt >= MAX_DIGITS) pnt = 0;
  buff[pnt] |= POINT;
  //контроль переполнения:
  char j = n - (minus? 1 : 0);
  if(d & AUTO_SCALE) j += d & ~AUTO_SCALE;
  bool overflow = 0;
  for(i = j; i < MAX_DIGITS; i++)
    overflow |= buff[i];
  if(overflow)
  {
    for(i = 0; i < MAX_DIGITS; i++)
      buff[i] = (i < n)? '-' : 0;
    if(!(d & AUTO_SCALE))
      buff[pnt] |= POINT;
    minus = 0;  
  }
  //подавление лидирующих нулей:
  i = MAX_DIGITS;
  while(!buff[--i])
    buff[i] = ' ';
  //добавление минуса:
  if(minus) buff[++i] = '-';
  //вычисление индекса первого символа для вывода:
  if(i < n - 1) i = n - 1;
  //удаление точки в крайней правой позиции:
  buff[i - (n - 1)] &= ~POINT;
  //вывод числа:
  for(char j = 0; j < n; j++)
    PutChar(buff[i--]);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
