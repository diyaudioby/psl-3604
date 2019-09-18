//----------------------------------------------------------------------------

//Модуль поддержки клавиатуры: header file

//----------------------------------------------------------------------------

#ifndef KEYBOARD_H
#define KEYBOARD_H

//----------------------------------------------------------------------------

#include "systimer.h"

//----------------------------- Константы: -----------------------------------

enum KeyMsg_t //коды сообщений клавиатуры
{
  KBD_NOP    = 0x00, //нет нажатия
  KBD_SETV   = 0x01, //кнопка SET V
  KBD_SETI   = 0x02, //кнопка SET I
  KBD_OUT    = 0x04, //кнопка OUT ON/OFF
  KBD_FINE   = 0x08, //кнопка FINE
  KBD_ENC    = 0x10, //кнопка энкодера
  KBD_HOLD   = 0x80, //флаг удержания кнопки
  KBD_SETVI  = KBD_SETV + KBD_SETI, //кнопки SET V + SET I
  KBD_ENCH   = KBD_ENC  + KBD_HOLD, //удержание кнопки энкодера
  KBD_SETVH  = KBD_SETV + KBD_HOLD, //удержание SET V
  KBD_SETIH  = KBD_SETI + KBD_HOLD, //удержание SET I
  KBD_SETVIH = KBD_SETV + KBD_SETI + KBD_HOLD, //удержание SET V + SET I
  KBD_ERROR  = 0x7F  //код ошибки для запрета операций
};

//----------------------------------------------------------------------------
//--------------------------- Класс TKeyboard: -------------------------------
//----------------------------------------------------------------------------

class TKeyboard
{
private:
  TGpio<PORTB, PIN15> Pin_KeyV; 
  TGpio<PORTB, PIN12> Pin_KeyC; 
  TGpio<PORTB, PIN10> Pin_KeyO; 
  TGpio<PORTB, PIN11> Pin_KeyF; 
  TGpio<PORTA, PIN8>  Pin_KeyE; 
  TSoftTimer *DebounceTimer;
  TSoftTimer *HoldTimer;
  KeyMsg_t Prev_Key;
public:
  TKeyboard(void);
  void Execute(void);
  KeyMsg_t Scan(void);
  KeyMsg_t Message;
};

//----------------------------------------------------------------------------

#endif
