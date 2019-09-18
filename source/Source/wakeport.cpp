//----------------------------------------------------------------------------

//Модуль порта с протоколом Wake

//----------------------------------------------------------------------------

#include "main.h"
#include "wakeport.h"

//----------------------------------------------------------------------------
//--------------------------- Класс TWakePort --------------------------------
//----------------------------------------------------------------------------

//----------------------------- Конструктор: ---------------------------------

TWakePort::TWakePort(uint32_t baud, char frame) : TWake(frame)
{
  TWakePort::Wp = this;
  //настройка портов:
  Pin_TXD.Init(AF_PP_2M, OUT_HI);
  Pin_RXD.Init(IN_PULL, PULL_UP);
  //настройка USART2:
  RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
  USART1->BRR = APB2_CLOCK / baud;
  USART1->CR1 =
    USART_CR1_RE |     //разрешение приемника
    USART_CR1_TE |     //разрешение передатчика
    USART_CR1_RXNEIE | //разрешение прерывания RXNE
    USART_CR1_UE;      //разрешение USART2
  //настройка прерываний:
  NVIC_SetPriority(USART1_IRQn, 15);
  NVIC_EnableIRQ(USART1_IRQn);
}

//-------------------------- Прерывание USART1: ------------------------------

TWakePort* TWakePort::Wp;

void USART1_IRQHandler(void)
{
  //прерывание USART по приему:
  if(USART1->SR & USART_SR_RXNE)
  {
    TWakePort::Wp->Rx(USART1->DR);
  }
  //прерывание USART по передаче:
  if(USART1->SR & USART_SR_TXE)
  {
    char data;
    if(TWakePort::Wp->Tx(data))
      USART1->DR = data;
        else
          USART1->CR1 &= ~USART_CR1_TXEIE; //запрет прерывания TXE
  }
}

//--------------------------- Передача пакета: -------------------------------

void TWakePort::StartTx(char cmd)
{
  char data;
  TxStart(cmd, data);
  USART1->DR = data;
  USART1->CR1 |= USART_CR1_TXEIE; //разрешение прерывания TXE
}

//----------------------------------------------------------------------------
