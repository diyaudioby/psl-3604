//----------------------------------------------------------------------------

//Модуль термостата, заголовочный файл

//----------------------------------------------------------------------------

#ifndef SYSTIMER_H
#define SYSTIMER_H

//----------------------------- Константы: -----------------------------------

#define SYSTEM_CORE_CLOCK 24000000   //тактовая частота ядра, Гц
#define AHB_CLOCK SYSTEM_CORE_CLOCK  //частота шины AHB, Гц
#define APB2_CLOCK SYSTEM_CORE_CLOCK //частота шины APB2, Гц
#define APB1_CLOCK SYSTEM_CORE_CLOCK //частота шины APB1, Гц

#define USE_SEC //используется секундный тик

//------------------------ Настройка тактирования: ---------------------------

extern "C" void SystemInit(void);

//----------------------------------------------------------------------------
//--------------------------- Класс TSysTimer: -------------------------------
//----------------------------------------------------------------------------

extern "C" void SysTick_Handler(void);

class TSysTimer
{
private:
  friend void SysTick_Handler(void);
#ifdef USE_SEC  
  static uint32_t SecCount;
#endif  
  static uint32_t Start_us;
  static uint32_t Interval_us;
  static uint32_t Start_ms;
  static uint32_t Interval_ms;
protected:
  static volatile uint32_t Counter;
public:
  static void Init(void);
  static bool Tick;
#ifdef USE_SEC  
  static bool SecTick;
  static void SecReset(void);
#endif  
  static void Sync(void);
  static void Delay_us(uint16_t d);
  static void Delay_ms(uint32_t d);
  static void TimeoutStart_us(uint16_t t);
  static bool TimeoutOver_us(void);
  static void TimeoutStart_ms(uint32_t t);
  static bool TimeoutOver_ms(void);
};

//----------------------------------------------------------------------------
//-------------------------- Класс TSoftTimer: -------------------------------
//----------------------------------------------------------------------------

class TSoftTimer : public TSysTimer
{
private:
  uint32_t Interval;
  uint32_t StartCount;
  bool Event;
protected:
public:
  TSoftTimer(uint32_t = 0);
  bool Autoreload;
  bool Oneshot;
  void Start();
  void Start(uint32_t t);
  void SetInterval_ms(uint32_t t);
  void SetInterval_sec(uint32_t t);
  void SetInterval_min(uint32_t t);
  void SetInterval_hrs(uint32_t t);
  void Force(void);
  bool Over(void);
};

//----------------------------------------------------------------------------

#endif
