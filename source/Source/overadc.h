//----------------------------------------------------------------------------

//Реализация шаблонного класса TOverAdc

//----------------------- Используемые ресурсы: ------------------------------

//Шаблонный класс TOverAdc реализует Oversampled ADC на основе встроенного
//АЦП, эффективная разрядность которого повышается с помощью оверсэмплинга.
//Всего могут быть созданы 2 экземпляра этого класса с логическими
//номерами 0 и 1. АЦП используется в режиме преобразования инжектированной
//группы каналов, состоящей из 2-х каналов. Запуск преобразования группы
//осуществляется событием TIM2 TRGO. Результаты преобразования сохраняются
//в памяти в виде масива с помощью DMA. Для логического АЦП 0 используется
//канал DMA 5, для логического АЦП 1 - канал DMA 7. DMA работает в однократном
//режиме, после заполнения массива пересылка останавливается и устанавливается
//флаг готовности данных DMA_ISR_TCIF5 или DMA_ISR_TCIF7. После этого
//результат может быть считан с помощью функции AdcGetCode, каторая
//автоматически запускает следующий цикл накопления.
//Считанный код преобразован в формат с разрядностью ADC_RES.

//----------------------------------------------------------------------------

#ifndef OVERADC_H
#define OVERADC_H

//------------------------------- Константы: ---------------------------------

#define ADC_NR        12 //Native ADC Resolution, bits
#define ADC_RES       16 //Oversampled ADC Resolution, bits
#define ADC_FS        100000 //Sampling frequency, Hz
#define ADC_MAX_CODE  ((1 << ADC_RES) - (1 << (ADC_RES - ADC_NR)))
#define OVER_N        100 //Oversampling ratio

//Коды времени сэмплирования:

enum smp_t
{
  SMP1T5,
  SMP7T5,
  SMP13T5,
  SMP28T5,
  SMP41T5,
  SMP55T5,
  SMP71T5,
  SMP239T5
};

//----------------------------------------------------------------------------
//------------------------ Шаблонный класс TOverAdc: -------------------------
//----------------------------------------------------------------------------

//AdcN = 0, 1 - логический номер АЦП
//AdcPin = 0..9 - номер входного пина АЦП (номер канала) 

template<uint8_t AdcN, uint8_t AdcPin>
class TOverAdc
{
private:
  TGpio<PORTA, AdcPin> Pin_ADC;
  uint16_t Samples[OVER_N];
public:
  TOverAdc(void) {};
  void Init(void);
  bool Ready(void);
  operator uint16_t();
};

//---------------------------- Инициализация: --------------------------------

template<uint8_t AdcN, uint8_t AdcPin>
void TOverAdc<AdcN, AdcPin>::Init(void)
{
  Pin_ADC.Init(IN_ANALOG);            //настройка порта ADC
  RCC->CFGR |= RCC_CFGR_ADCPRE_DIV4;  //прескалер АЦП (APB2 / 4)
  RCC->APB2ENR |= RCC_APB2ENR_ADC1EN; //включение тактирования ADC
  
  ADC1->SMPR2 |= ADC_SMPR2_SMP0_0 * SMP13T5 << AdcPin * 3; //sample time
  
  ADC1->JSQR |=
    ADC_JSQR_JL_0      * 1 |          //2 conversions in inj. group
    ADC_JSQR_JSQ3_0    * AdcPin << AdcN * 5; //inj. channel select
  
  ADC1->CR1 =
    ADC_CR1_AWDEN      * 0 |          //reg. analog watchdog disable
    ADC_CR1_JAWDEN     * 0 |          //inj. analog watchdog disable
    ADC_CR1_DISCNUM_0  * 0 |          //no disc. mode channels
    ADC_CR1_JDISCEN    * 0 |          //inj. disc. mode disable
    ADC_CR1_DISCEN     * 0 |          //reg. disc. mode disable
    ADC_CR1_JAUTO      * 1 |          //auto injected group conversion 
    ADC_CR1_AWDSGL     * 0 |          //watchdog single scan channel disable
    ADC_CR1_SCAN       * 1 |          //scan mode
    ADC_CR1_JEOCIE     * 0 |          //inj. channels interrupt disable 
    ADC_CR1_AWDIE      * 0 |          //analog watchdog interrupt disable
    ADC_CR1_EOCIE      * 0 |          //EOC interrupt disable 
    ADC_CR1_AWDCH_0    * 0;           //analog watchdog channel
  
  ADC1->CR2 =
    ADC_CR2_TSVREFE    * 0 |          //temp. sensor disable
    ADC_CR2_SWSTART    * 0 |          //reg. start conversion  
    ADC_CR2_JSWSTART   * 0 |          //inj. start conversion
    ADC_CR2_EXTTRIG    * 0 |          //reg. ext. start
    ADC_CR2_EXTSEL_0   * 0 |          //reg. ext. event select
    ADC_CR2_JEXTTRIG   * 1 |          //inj. ext. start
    ADC_CR2_JEXTSEL_0  * 2 |          //inj. ext. event select - TIM2 TRGO
    ADC_CR2_ALIGN      * 0 |          //right data alignment
    ADC_CR2_DMA        * 0 |          //DMA disable
    ADC_CR2_RSTCAL     * 0 |          //calibration reset
    ADC_CR2_CAL        * 0 |          //calibration
    ADC_CR2_CONT       * 0 |          //continuous conversion
    ADC_CR2_ADON       * 1;           //ADC enable

  ADC1->CR2 |= ADC_CR2_RSTCAL;        //ADC calibration reset
  while(ADC1->CR2 & ADC_CR2_RSTCAL);
  ADC1->CR2 |= ADC_CR2_CAL;           //ADC calibration
  while(ADC1->CR2 & ADC_CR2_CAL);
  
  RCC->APB1ENR |= RCC_APB1ENR_TIM2EN; //включение тактирования TIM2
  TIM2->CR1 &= ~TIM_CR1_CEN;          //запрещение таймера
  TIM2->PSC = 0;                      //загрузка прескалера
  TIM2->ARR = (SYSTEM_CORE_CLOCK / ADC_FS) - 1; //период таймера
  
  TIM2->CR2 =
    TIM_CR2_TI1S       * 0 |
    TIM_CR2_MMS_0      * 2 |          //Update -> TRGO
    TIM_CR2_CCDS       * 0;           //DMA request when CC event
    
  if(AdcN == 0) //сравнение констант, оптимизатор уберет ненужное
  {
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    DMA1_Channel5->CPAR = (uint32_t)&ADC1->JDR1; //periph. address
    DMA1_Channel5->CMAR = (uint32_t)&Samples;    //memory address
    DMA1_Channel5->CNDTR = OVER_N;               //buffer size
    
    DMA1_Channel5->CCR =
      DMA_CCR5_MEM2MEM * 0 |          //memory to memory off
      DMA_CCR5_PL_0    * 2 |          //high priority
      DMA_CCR5_MSIZE_0 * 1 |          //mem. size 16 bit
      DMA_CCR5_PSIZE_0 * 1 |          //periph. size 16 bit
      DMA_CCR5_MINC    * 1 |          //memory increment enable
      DMA_CCR5_PINC    * 0 |          //periph. increment disable
      DMA_CCR5_CIRC    * 0 |          //circular mode disable
      DMA_CCR5_DIR     * 0 |          //direction - from periph.
      DMA_CCR5_TEIE    * 0 |          //transfer error interrupt disable
      DMA_CCR5_HTIE    * 0 |          //half transfer interrupt disable
      DMA_CCR5_TCIE    * 0 |          //transfer complete interrupt disable
      DMA_CCR5_EN      * 1;           //DMA enable
    
    TIM2->CCR1 = TIM2->ARR / 2;       //CC1 register load
    TIM2->DIER |= TIM_DIER_CC1DE;     //CC1 DMA request enable
  }
  if(AdcN == 1) //сравнение констант, оптимизатор уберет ненужное
  {
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    DMA1_Channel7->CPAR = (uint32_t)&ADC1->JDR2; //periph. address
    DMA1_Channel7->CMAR = (uint32_t)&Samples;    //memory address
    DMA1_Channel7->CNDTR = OVER_N;               //buffer size
    
    DMA1_Channel7->CCR =
      DMA_CCR7_MEM2MEM * 0 |          //memory to memory off
      DMA_CCR7_PL_0    * 2 |          //high priority
      DMA_CCR7_MSIZE_0 * 1 |          //mem. size 16 bit
      DMA_CCR7_PSIZE_0 * 1 |          //periph. size 16 bit
      DMA_CCR7_MINC    * 1 |          //memory increment enable
      DMA_CCR7_PINC    * 0 |          //periph. increment disable
      DMA_CCR7_CIRC    * 0 |          //circular mode disable
      DMA_CCR7_DIR     * 0 |          //direction - from periph.
      DMA_CCR7_TEIE    * 0 |          //transfer error interrupt disable
      DMA_CCR7_HTIE    * 0 |          //half transfer interrupt disable
      DMA_CCR7_TCIE    * 0 |          //transfer complete interrupt disable
      DMA_CCR7_EN      * 1;           //DMA enable
    
    TIM2->CCR2 = TIM2->ARR;           //CC2 register load
    TIM2->DIER |= TIM_DIER_CC2DE;     //CC2 DMA request enable
  }
  TIM2->CR1 |= TIM_CR1_CEN;           //timer 2 enable
}

//------------------------ Чтение готовности ADC: ----------------------------

template<uint8_t AdcN, uint8_t AdcPin>
inline bool TOverAdc<AdcN, AdcPin>::Ready(void)
{
  return(DMA1->ISR & (AdcN? DMA_ISR_TCIF7 : DMA_ISR_TCIF5));
}

//------------------------- Чтение данных ADC: -------------------------------

template<uint8_t AdcN, uint8_t AdcCh>
inline TOverAdc<AdcN, AdcCh>::operator uint16_t()
{
  int32_t Avg = 0;
  for(int16_t i = 0; i < OVER_N; i++)
    Avg += Samples[i];
  if(AdcN == 0) //сравнение констант, оптимизатор уберет ненужное
  {
    DMA1_Channel5->CCR &= ~DMA_CCR5_EN; //DMA disable
    DMA1_Channel5->CNDTR = OVER_N;      //buffer size
    DMA1->IFCR = DMA_IFCR_CTCIF5;       //flag clear
    DMA1_Channel5->CCR |= DMA_CCR5_EN;  //DMA enable
  }
  if(AdcN == 1) //сравнение констант, оптимизатор уберет ненужное
  {
    DMA1_Channel7->CCR &= ~DMA_CCR7_EN; //DMA disable
    DMA1_Channel7->CNDTR = OVER_N;      //buffer size
    DMA1->IFCR = DMA_IFCR_CTCIF7;       //flag clear
    DMA1_Channel7->CCR |= DMA_CCR7_EN;  //DMA enable
  }
  return((Avg * (1 << (ADC_RES - ADC_NR)) + OVER_N / 2) / OVER_N);
}

//----------------------------------------------------------------------------

#endif
