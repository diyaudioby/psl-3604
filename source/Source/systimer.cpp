//----------------------------------------------------------------------------

//Модуль системного таймера

//----------------------------------------------------------------------------

#include "main.h"
#include "systimer.h"

//----------------------------- Константы: -----------------------------------

#define CLK_PER_US (SYSTEM_CORE_CLOCK / 1000000)
#define CLK_PER_MS (SYSTEM_CORE_CLOCK / 1000)
#define MS_PER_S   (1000)

#define INPUT_PULL 0x88888888 //включение подтяжек входов (CNFx_1 = 1)

//----------------------- Инициализация системы: -----------------------------

void SystemInit(void)
{
  //включение PVD и ожидание установления питания:
  RCC->APB1ENR |= RCC_APB1ENR_PWREN;
  PWR->CR = PWR_CR_PVDE | PWR_CR_PLS_2V9;
  while(PWR->CSR & PWR_CSR_PVDO);

  //Тактирование от HSI с PLL:
  RCC->CFGR |= RCC_CFGR_PLLMULL6;         //HSI / 2 * 6 = 24 MHz 
  RCC->CR |= RCC_CR_PLLON;                //разрешение PLL
  while((RCC->CR & RCC_CR_PLLRDY) == 0);  //ожидание готовности PLL
  RCC->CFGR &= ~RCC_CFGR_SW;              //очистка битов SW
  RCC->CFGR |= RCC_CFGR_SW_PLL;           //выбор тактирования от PLL
  while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_1); //ожидание выбора PLL
  
  //Включение тактирования всех портов:
  RCC->APB2ENR |=
    RCC_APB2ENR_AFIOEN |
    RCC_APB2ENR_IOPAEN |
    RCC_APB2ENR_IOPBEN |
    RCC_APB2ENR_IOPCEN |
    RCC_APB2ENR_IOPDEN;
  //Настройка всех портов на ввод с подтяжкой вниз (CNFx_1 = 1),
  //оставлять CMOS-входы висящими в воздухе нехорошо.
  GPIOA->ODR = 0; //JTAG pins reset
  GPIOB->ODR = 0; //JTAG pins reset
  GPIOA->CRL = INPUT_PULL;
  GPIOA->CRH = INPUT_PULL;
  GPIOB->CRL = INPUT_PULL;
  GPIOB->CRH = INPUT_PULL;
  GPIOC->CRL = INPUT_PULL;
  GPIOC->CRH = INPUT_PULL;
  GPIOD->CRL = INPUT_PULL;
  GPIOD->CRH = INPUT_PULL;
}

//----------------------------------------------------------------------------
//--------------------------- Класс TSysTimer --------------------------------
//----------------------------------------------------------------------------

//-------------------- Инициализация системного таймера: ---------------------

void TSysTimer::Init(void)
{
  Counter = 0;
  SysTick_Config(CLK_PER_MS);
}

//--------------------- Прерывание системного таймера: -----------------------

volatile uint32_t TSysTimer::Counter;

void SysTick_Handler(void)
{
  TSysTimer::Counter++;
}

//----------------------- Сброс секундного таймера: --------------------------

#ifdef USE_SEC  
void TSysTimer::SecReset(void)
{
  SecCount = Counter;
}
#endif  

//-------------------- Синхронизация системных тиков: ------------------------

bool TSysTimer::Tick;
#ifdef USE_SEC  
  bool TSysTimer::SecTick;
  uint32_t TSysTimer::SecCount;
#endif  

void TSysTimer::Sync(void)
{
  Tick = (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk);
#ifdef USE_SEC  
  if(Tick && (Counter - SecCount >= MS_PER_S))
  {
    SecTick = 1;
    SecCount += MS_PER_S;
  }
  else
  {
    SecTick = 0;
  }
#endif  
}

//-------------- Функция задержки микросекундного диапазона: -----------------

void TSysTimer::Delay_us(uint16_t d)
{
  uint32_t DelayStart = SysTick->VAL;
  uint32_t DelayTicks = d * CLK_PER_US;
  int32_t  Delta;
  do
  {
    Delta = DelayStart - SysTick->VAL;
    if(Delta < 0) Delta += CLK_PER_MS;
  }
  while(Delta < DelayTicks);
}

//-------------- Функция задержки миллисекундного диапазона: -----------------

void TSysTimer::Delay_ms(uint32_t d)
{
  uint32_t DelayStart = Counter;
  while(Counter - DelayStart < d);
}

//------------------ Таймаут микросекундного диапазона: ----------------------

uint32_t TSysTimer::Start_us;
uint32_t TSysTimer::Interval_us;

void TSysTimer::TimeoutStart_us(uint16_t t)
{
  Start_us = SysTick->VAL;
  Interval_us = t * CLK_PER_US;
}

bool TSysTimer::TimeoutOver_us(void)
{
  int32_t Delta = Start_us - SysTick->VAL;
  if(Delta < 0) Delta += CLK_PER_MS;
  return(Delta >= Interval_us);
}

//------------------ Таймаут миллисекундного диапазона: ----------------------

uint32_t TSysTimer::Start_ms;
uint32_t TSysTimer::Interval_ms;

void TSysTimer::TimeoutStart_ms(uint32_t t)
{
  Start_ms = Counter;
  Interval_ms = t;
}

bool TSysTimer::TimeoutOver_ms(void)
{
  return(Counter - Start_ms >= Interval_ms);
}

//----------------------------------------------------------------------------
//-------------------------- Класс TSoftTimer: -------------------------------
//----------------------------------------------------------------------------

//---------------------------- Конструктор: ----------------------------------

TSoftTimer::TSoftTimer(uint32_t t)
{
  Autoreload = 0;
  Oneshot = 0;
  Event = (t == 0)? 1 : 0;
  Interval = t;
  StartCount = TSysTimer::Counter;
}

//-------------------------------- Старт: ------------------------------------

void TSoftTimer::Start()
{
  Event = 0;
  StartCount = TSysTimer::Counter;
}

void TSoftTimer::Start(uint32_t t)
{
  Interval = t;
  Event = (t == 0)? 1 : 0;
  StartCount = TSysTimer::Counter;
}

//------------------------- Установка интервала: -----------------------------

void TSoftTimer::SetInterval_ms(uint32_t t)
{
  Interval = t;
}

void TSoftTimer::SetInterval_sec(uint32_t t)
{
  Interval = t * 1000;
}

void TSoftTimer::SetInterval_min(uint32_t t)
{
  Interval = t * 60000;
}

void TSoftTimer::SetInterval_hrs(uint32_t t)
{
  Interval = t * 3600000;
}

//---------------------- Принудительное переполнение: ------------------------

void TSoftTimer::Force(void)
{
  StartCount = TSysTimer::Counter - Interval;
}

//------------------------- Чтение переполнения: -----------------------------

bool TSoftTimer::Over(void)
{
  bool event = TSysTimer::Counter - StartCount >= Interval;
  if(event)
  {
    if(Oneshot && Event) event = 0;
    Event = 1;  
    if(Autoreload)
    {
      StartCount = TSysTimer::Counter; //перезапуск
      Event = 0;  
    }
  }
  if(!Oneshot && !Autoreload) event = Event;
  return(event);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
