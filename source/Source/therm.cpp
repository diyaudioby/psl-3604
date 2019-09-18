//----------------------------------------------------------------------------

//Модуль поддержки термометра DS18B20

//----------------------- Используемые ресурсы: ------------------------------

//Используются внешний термометр DS18B20 (в 12-разрядном режиме),
//который подключен к соединенным выводам USART2_TX (PA2) и
//USART2_RX (PA3). Дополнительно к этим выводам подключен подтягивающий
//резистор 4.7 кОм на +3.3 В.
//Обмен по шине 1-Wire производится с использованием USART2,
//который аппаратно формирует импульс сброса и тайм-слоты.
//Чтение/запись данных в USART2 производится по прерываниям.
//Требуется одно прерывание на бит, причем время реакции не лимитировано.
//В принципе, можно использовать DMA, но можно и не использовать :)

//Если выбрать режим "single-wire half-duplex mode" для USART2, то
//для реализации шины 1-Wire достаточно одного вывода USART2_TX (PA2).
//Но поскольку на печатной плате уже есть соединение PA2 - PA3,
//этот режим здесь не используется (строчка закомментирована).
//Нужно заметить, что реализация приема-передачи через разные пины может
//быть иногда полезной, например, при реализации опторазвязки интерфейса.

//Очень скверно, что в контроллере с такой богатой периферией не нашлось
//места хотя бы для парочки аппаратных портов 1-Wire. Эмуляция этого
//интерфейса с помощью USART хотя формально требования стандарта и
//соблюдает, но тайминги работы с шиной получаются далеко не оптимальные.
//Их не исправить даже свободным выбором baud rate, что позволяет сделать
//прекрасный дробный делитель USART в STM32 (можно еще эмулировать 1-Wire
//с помощью таймера, но это требует несколько высокоприоритетных прерываний
//на каждый бит). Спасает одно - в данном случае на порту 1-Wire
//нет длинной линии.

//----------------------------------------------------------------------------

#include "main.h"
#include "therm.h"

//------------------------------ Константы: ----------------------------------

#define BR_RESET    10417 //скорость порта для формирования RESET
#define BR_TSLOT   166667 //скорость порта для формирования TIME SLOT
#define CONVERT_TM    800 //время преобразования температуры, мс

#define BRR_RESET (APB1_CLOCK + BR_RESET / 2) / BR_RESET;
#define BRR_TSLOT (APB1_CLOCK + BR_TSLOT / 2) / BR_TSLOT;

//----------------------------------------------------------------------------
//--------------------- Абстрактный класс TOwpAction: ------------------------
//----------------------------------------------------------------------------

//----------------------------- Конструктор: ---------------------------------

TOwpAction::TOwpAction(char data)
{
  Data = data;
  Result = OWP_NONE;
  Action = OWP_NONE;
}

//--------------------- Проверка завершения операции: ------------------------

void TOwpAction::Execute(void)
{
  if((Action == OWP_READY) ||
     (Action == OWP_FAIL))
  {
    Value = DataRd;
    Result = Action;
    Action = OWP_NONE;
  }
}

//-------------------------- Прерывание USART2: ------------------------------

volatile OwpAct_t TOwpAction::Action;
char TOwpAction::BitCounter;
char TOwpAction::DataRd;
char TOwpAction::DataWr;

void USART2_IRQHandler(void)
{
  //прерывание USART по завершению передачи:
  if(USART2->SR & USART_SR_TC)
  {
    //очистка флага прерывания:
    USART2->SR &= ~USART_SR_TC;
    //выполняется сброс:
    if(TOwpAction::Action == OWP_RESET)
    {
      TOwpAction::DataRd = USART2->DR;
      //импульс presence находится в одном из трех битов D4..D6:
      if(((TOwpAction::DataRd & 0x70) != 0x70) &&
         //бит D7 должен быть единичным, иначе это замыкание линии
         //на землю, а не присутствие устройства:
         ((TOwpAction::DataRd & 0x80) == 0x80))
        TOwpAction::Action = OWP_READY;
          else TOwpAction::Action = OWP_FAIL;
    }
    //выполняется чтение/запись:
    if(TOwpAction::Action == OWP_RW)
    {
      TOwpAction::DataRd >>= 1;
      //считывается бит D1 (а не D0), так как при этом момент опроса
      //лежит ближе всего к отметке 15 мкс после начала тайм-слота:
      TOwpAction::DataRd |= ((USART2->DR & 2)? 0x80 : 0x00);
      if(++TOwpAction::BitCounter < 8)
      {
        TOwpAction::DataWr >>= 1;
        USART2->DR = (TOwpAction::DataWr & 1)? 0xFF : 0x00;
      }
      else
      {
        TOwpAction::Action = OWP_READY;
      }
    }
  }
}

//----------------------------------------------------------------------------
//---------------------------- Класс TOwpReset: ------------------------------
//----------------------------------------------------------------------------

//---------------------------- Запуск операции: ------------------------------

void TOwpReset::Start(void)
{
  DataWr = Data;
  USART2->BRR = BRR_RESET;
  USART2->DR = DataWr;
  Result = OWP_NONE;
  Action = OWP_RESET;
}

//----------------------------------------------------------------------------
//----------------------------- Класс TOwpRW: --------------------------------
//----------------------------------------------------------------------------

//---------------------------- Запуск операции: ------------------------------

void TOwpRW::Start(void)
{
  DataWr = Data;
  USART2->BRR = BRR_TSLOT;
  USART2->DR = (DataWr & 1)? 0xFF : 0x00;
  BitCounter = 0;
  Result = OWP_NONE;
  Action = OWP_RW;
}

//----------------------------------------------------------------------------
//---------------------------- Класс TOwpTask: -------------------------------
//----------------------------------------------------------------------------

//----------------------------- Конструктор: ---------------------------------

TOwpTask::TOwpTask(char maxact)
{
  MaxActions = maxact;
  Actions = new TOwpAction*[MaxActions];
  ActCount = 0;
  State = OWP_NONE;
}

//------------------------ Добавление операции: ------------------------------

void TOwpTask::AddAction(TOwpAction *act)
{
  if(ActCount < MaxActions)
    Actions[ActCount++] = act;
}

//---------------- Запуск последовательности операций: -----------------------

void TOwpTask::Start(void)
{
  Index = 0;
  Error = 0;
  State = OWP_ACT;
  Actions[Index]->Start();
}

//-------------------- Процесс выполнения операций: --------------------------

void TOwpTask::Execute(void)
{
  if(State == OWP_ACT)
  {
    if(Actions[Index]->Result == OWP_READY)
    {
      Index++;
      if(Index == ActCount)
        State = OWP_READY;
          else Actions[Index]->Start();
    }
    else if(Actions[Index]->Result == OWP_FAIL)
    {
      Error = 1;
      State = OWP_READY;
    }
    else
    {
      Actions[Index]->Execute();
    }
  }
}

//------------ Проверка завершения последовательности операций: --------------

bool TOwpTask::Done(void)
{
  if(State == OWP_READY)
  {
    State = OWP_NONE;
    return(1);
  }
  return(0);
}

//----------------------------- Чтение ошибки: -------------------------------

bool TOwpTask::Fail(void)
{
  return(Error);
}

//----------------------------------------------------------------------------
//----------------------------- Класс TTherm: --------------------------------
//----------------------------------------------------------------------------

//----------------------------- Конструктор: ---------------------------------

TTherm::TTherm(void)
{
  //настройка портов:
  Pin_OWPO.Init(AF_OD_2M);
  Pin_OWPI.Init(IN_FLOAT);
  //настройка USART2:
  RCC->APB1ENR |= RCC_APB1ENR_USART2EN; //включение тактирования USART2
  USART2->BRR = BRR_RESET;
  //USART2->CR3 = USART_CR3_HDSEL; //работа через один пин USART2_TX (PA2)
  USART2->CR1 =
    USART_CR1_RE |   //разрешение приемника
    USART_CR1_TE |   //разрешение передатчика
    USART_CR1_TCIE | //разрешение прерывания по концу передачи
    USART_CR1_UE;    //разрешение USART2
  //настройка прерываний:
  NVIC_SetPriority(USART2_IRQn, 15);
  NVIC_EnableIRQ(USART2_IRQn);
  ThermTimer = new TSoftTimer(CONVERT_TM);
  ThermTimer->Oneshot = 1;

  OwpStartTherm = new TOwpTask(3);
  OwpStartTherm->AddAction(new TOwpReset());  //RESET
  OwpStartTherm->AddAction(new TOwpRW(0xCC)); //SKIP ROM
  OwpStartTherm->AddAction(new TOwpRW(0x44)); //START CONVERSION

  OwpReadTherm = new TOwpTask(5);
  OwpReadTherm->AddAction(new TOwpReset());   //RESET
  OwpReadTherm->AddAction(new TOwpRW(0xCC));  //SKIP ROM
  OwpReadTherm->AddAction(new TOwpRW(0xBE));  //READ SCRATCHPAD
  OwpReadTherm->AddAction(new TOwpRW());      //READ TL
  OwpReadTherm->AddAction(new TOwpRW());      //READ TH

  Update = 0;
  Value = TEMP_FAIL;
  OwpStartTherm->Start();          //запуск команды START
}

//-------------- Выполнение процесса измерения температуры: ------------------

void TTherm::Execute(void)
{
  OwpStartTherm->Execute();        //выполнение команды START
  if(OwpStartTherm->Done())        //если команда START выполнена,
    ThermTimer->Start(CONVERT_TM); //запуск таймера
  if(ThermTimer->Over())           //если интервал истек,
    OwpReadTherm->Start();         //запуск команды READ
  OwpReadTherm->Execute();         //выполнение команды READ
  if(OwpReadTherm->Done())         //если команда READ выполнена,
  {
    if((OwpStartTherm->Fail()) ||  //если ошибка,
       (OwpReadTherm->Fail()))
      Value = TEMP_FAIL;           //код ошибки температуры
        else Value = CalculateT(); //преобразование температуры
    Update = 1;                    //установка флага обновления
    OwpStartTherm->Start();        //запуск команды START
  }
  else
  {
    Update = 0;
  }
}

//---------------------- Вычисление температуры: -----------------------------

int16_t TTherm::CalculateT(void)
{
  char tl = OwpReadTherm->Actions[3]->Value;
  char th = OwpReadTherm->Actions[4]->Value;
  int16_t temp = 10 * (tl | (th << 8)) / 16;
  if(temp < TEMP_MIN) temp = TEMP_MIN;
  if(temp > TEMP_MAX) temp = TEMP_MAX;
  return(temp);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
