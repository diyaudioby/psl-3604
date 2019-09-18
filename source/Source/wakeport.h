//----------------------------------------------------------------------------

//Модуль порта с протоколом Wake, заголовочный файл

//----------------------------------------------------------------------------

#ifndef WAKEPORT_H
#define WAKEPORT_H

#include "wake.h"

//----------------------------------------------------------------------------
//--------------------------- Класс TWakePort --------------------------------
//----------------------------------------------------------------------------

extern "C" void USART1_IRQHandler(void);

class TWakePort : public TWake
{
private:
  TGpio<PORTA, PIN9> Pin_TXD; 
  TGpio<PORTA, PIN10> Pin_RXD; 
  static TWakePort *Wp;
  friend void USART1_IRQHandler(void);
protected:
public:
  TWakePort(uint32_t baud, char frame);
  void StartTx(char cmd);
};

//----------------------------------------------------------------------------

#endif
