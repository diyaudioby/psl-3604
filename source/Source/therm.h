//----------------------------------------------------------------------------

//Модуль поддержки термометра DS18B20, заголовочный файл

//----------------------------------------------------------------------------

#ifndef THERM_H
#define THERM_H

//----------------------------- Константы: -----------------------------------

#define TEMP_MIN      0 //минимальная измеряемая температура, *0.1°C
#define TEMP_MAX    999 //максимальная измеряемая температура, *0.1°C
#define TEMP_FAIL -1000 //код ошибки температуры

enum OwpAct_t //коды операций на шине 1-Wire
{
  OWP_NONE,
  OWP_RESET,
  OWP_RW,
  OWP_ACT,
  OWP_READY,
  OWP_FAIL
};

//----------------------------------------------------------------------------
//--------------------- Абстрактный класс TOwpAction: ------------------------
//----------------------------------------------------------------------------

extern "C" void USART2_IRQHandler(void);

class TOwpAction
{
private:
  static char DataRd;
  friend void USART2_IRQHandler(void);
protected:
  volatile static OwpAct_t Action;
  static char BitCounter;
  static char DataWr;
  char Data;
public:
  TOwpAction(char data);
  OwpAct_t Result;
  char Value;
  virtual void Start(void) = 0;
  void Execute(void);
};

//----------------------------------------------------------------------------
//----------------------------- Класс TOwpReset: -----------------------------
//----------------------------------------------------------------------------

class TOwpReset : public TOwpAction
{
private:
public:
  TOwpReset(char data = 0xF0) : TOwpAction(data) {};
  void Start(void);
};

//----------------------------------------------------------------------------
//------------------------------- Класс TOwpRW: ------------------------------
//----------------------------------------------------------------------------

class TOwpRW : public TOwpAction
{
private:
public:
  TOwpRW(char data = 0xFF) : TOwpAction(data) {};
  void Start(void);
};

//----------------------------------------------------------------------------
//------------------------------ Класс TOwpTask: -----------------------------
//----------------------------------------------------------------------------

class TOwpTask
{
private:
  char MaxActions;
  char ActCount;
  char Index;
  OwpAct_t State;
  bool Error;
public:
  TOwpTask(char maxact);
  TOwpAction** Actions;
  void AddAction(TOwpAction *act);
  void Start(void);
  void Execute(void);
  bool Done(void);
  bool Fail(void);
};

//----------------------------------------------------------------------------
//----------------------------- Класс TTherm: --------------------------------
//----------------------------------------------------------------------------

class TTherm
{
private:
  TGpio<PORTA, PIN2> Pin_OWPO;
  TGpio<PORTA, PIN3> Pin_OWPI;
  TSoftTimer *ThermTimer;
  TOwpTask *OwpStartTherm;
  TOwpTask *OwpReadTherm;
  int16_t CalculateT(void);
public:
  TTherm(void);
  void Execute(void);
  bool Update;
  int16_t Value;
};

//----------------------------------------------------------------------------

extern TTherm *Therm;

//----------------------------------------------------------------------------

#endif
