//----------------------------------------------------------------------------

//Модуль поддержки энкодера, заголовочный файл

//----------------------------------------------------------------------------

#ifndef ENCODER_H
#define ENCODER_H

//----------------------------------------------------------------------------

#include "systimer.h"

//------------------------------- Константы: ---------------------------------

#define ENC_NOP        0 //нет вращения
#define ENC_STEP       1 //нормальный шаг энкодера
#define ENC_FAST_STEP 10 //быстрый шаг энкодера
#define ENC_FLT        1 //размер массива цифрового фильтра

//----------------------------------------------------------------------------
//---------------------------- Класс TEncoder: -------------------------------
//----------------------------------------------------------------------------

class TEncoder
{
private:
  enum EncoderState
  {
    STATE_0,
    STATE_A,
    STATE_B,
    STATE_AB
  };
  TGpio<PORTA, PIN0> Pin_F1;
  TGpio<PORTA, PIN1> Pin_F2;
  char EncPrev;
  char EncPrevPrev;
  char EncCur;
  TSoftTimer *EncTimer;
  TSoftTimer *RevTimer;
  bool Dir;
  char EncV;
  char EncS;
#if ENC_FLT > 1 
  char EncF[ENC_FLT];
  char FPtr;
#endif  
public:
  TEncoder(void);
  void Execute(void);
  bool Rev;
  int8_t Message;
};

//----------------------------------------------------------------------------

extern TEncoder *Encoder;

//----------------------------------------------------------------------------

#endif
