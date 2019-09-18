//----------------------------------------------------------------------------

//Реализация шаблонного класса TGpio

//----------------------------------------------------------------------------

#ifndef GPIO_H
#define	GPIO_H

//-------------------------- Базовые адреса портов: --------------------------

#ifdef GPIOA_BASE
 #define PORTA GPIOA_BASE
#endif
#ifdef GPIOB_BASE
 #define PORTB GPIOB_BASE
#endif
#ifdef GPIOC_BASE
 #define PORTC GPIOC_BASE
#endif
#ifdef GPIOD_BASE
 #define PORTD GPIOD_BASE
#endif
#ifdef GPIOE_BASE
 #define PORTE GPIOE_BASE
#endif
#ifdef GPIOF_BASE
 #define PORTF GPIOF_BASE
#endif
#ifdef GPIOG_BASE
 #define PORTG GPIOG_BASE
#endif

//----------------------------- Номера пинов: --------------------------------

#define PIN0  0
#define PIN1  1
#define PIN2  2
#define PIN3  3
#define PIN4  4
#define PIN5  5
#define PIN6  6
#define PIN7  7
#define PIN8  8
#define PIN9  9
#define PIN10 10
#define PIN11 11
#define PIN12 12
#define PIN13 13
#define PIN14 14
#define PIN15 15

//----------------------------- Режимы пинов: --------------------------------

enum PinMode_t
{
  IN_ANALOG  = 0x00, //Analog input        (CNF=00 MODE=00)
  IN_FLOAT   = 0x04, //Floating input      (CNF=01 MODE=00)
  IN_PULL    = 0x08, //Pull up/down input  (CNF=10 MODE=00)
  OUT_PP_10M = 0x01, //PP output 10MHz     (CNF=00 MODE=01)
  OUT_PP_2M  = 0x02, //PP output  2MHz     (CNF=00 MODE=10)
  OUT_PP_50M = 0x03, //PP output 50MHz     (CNF=00 MODE=11)
  OUT_OD_10M = 0x05, //OD output 10MHz     (CNF=01 MODE=01)
  OUT_OD_2M  = 0x06, //OD output  2MHz     (CNF=01 MODE=10)
  OUT_OD_50M = 0x07, //OD output 50MHz     (CNF=01 MODE=11)
  AF_PP_10M  = 0x09, //Alt. func. PP 10MHz (CNF=10 MODE=01)
  AF_PP_2M   = 0x0a, //Alt. func. PP  2MHz (CNF=10 MODE=10)
  AF_PP_50M  = 0x0b, //Alt. func. PP 50MHz (CNF=10 MODE=11)
  AF_OD_10M  = 0x0d, //Alt. func. OD 10MHz (CNF=11 MODE=01)
  AF_OD_2M   = 0x0e, //Alt. func. OD 2MHz  (CNF=11 MODE=10)
  AF_OD_50M  = 0x0f  //Alt. func. OD 50MHz (CNF=11 MODE=11)
};

//------------------ Подтяжки/начальные состояния пинов: ---------------------

enum PinPull_t
{
  PULL_DN = 0, //Pull down     (ODR.x = 0)
  PULL_UP = 1, //Pull up       (ODR.x = 1)
  OUT_LO  = 0, //Out init low  (ODR.x = 0)
  OUT_HI  = 1  //Out init high (ODR.x = 1)
};

//----------------------------------------------------------------------------
//------------------------ Шаблонный класс TGpio: ----------------------------
//----------------------------------------------------------------------------

template<uint32_t Port, uint8_t Pin>
class TGpio
{
public:
  TGpio() {};
  static void Init(PinMode_t Mode);
  static void Init(PinMode_t Mode, PinPull_t Pull);
  static void Set();
  static void Clr();
  static bool Get();
  void operator = (bool Value);
  operator bool();
};

//------------------------- Реализация методов: ------------------------------

template<uint32_t Port, uint8_t Pin, bool = Pin >= 8>
struct GpioInit
{
  inline static void Init(PinMode_t Mode)
  {
    reinterpret_cast<GPIO_TypeDef*>(Port)->CRH &= ~(0x0f << ((Pin - 8) * 4));
    reinterpret_cast<GPIO_TypeDef*>(Port)->CRH |= Mode << ((Pin - 8) * 4);
  }
};

template<uint32_t Port, uint8_t Pin>
struct GpioInit<Port, Pin, 0>
{
  inline static void Init(PinMode_t Mode)
  {
    reinterpret_cast<GPIO_TypeDef*>(Port)->CRL &= ~(0x0f << (Pin * 4));
    reinterpret_cast<GPIO_TypeDef*>(Port)->CRL |= Mode << (Pin * 4);
  }
};

template<uint32_t Port, uint8_t Pin>
inline void TGpio<Port, Pin>::Init(PinMode_t Mode)
{
  GpioInit<Port, Pin>::Init(Mode);
}

template<uint32_t Port, uint8_t Pin>
inline void TGpio<Port, Pin>::Init(PinMode_t Mode, PinPull_t Pull)
{
  Pull? Set() : Clr();
  GpioInit<Port, Pin>::Init(Mode);
}

template<uint32_t Port, uint8_t Pin>
inline void TGpio<Port, Pin>::Set()
{
  reinterpret_cast<GPIO_TypeDef*>(Port)->BSRR = 1 << Pin;
}

template<uint32_t Port, uint8_t Pin>
inline void TGpio<Port, Pin>::Clr()
{
  reinterpret_cast<GPIO_TypeDef*>(Port)->BRR = 1 << Pin;
}

template<uint32_t Port, uint8_t Pin>
inline bool TGpio<Port, Pin>::Get()
{
  return(reinterpret_cast<GPIO_TypeDef*>(Port)->IDR & 1 << Pin);
}

template<uint32_t Port, uint8_t Pin>
inline void TGpio<Port, Pin>::operator = (bool Value)
{
  if(Value) reinterpret_cast<GPIO_TypeDef*>(Port)->BSRR = 1 << Pin;
    else reinterpret_cast<GPIO_TypeDef*>(Port)->BRR = 1 << Pin;
}

template<uint32_t Port, uint8_t Pin>
inline TGpio<Port, Pin>::operator bool()
{
  return(reinterpret_cast<GPIO_TypeDef*>(Port)->IDR & 1 << Pin);
}

//----------------------------------------------------------------------------

#endif
