//----------------------------------------------------------------------------

//Модуль управления оборудованием, заголовочный файл

//----------------------------------------------------------------------------

#ifndef ANALOG_H
#define ANALOG_H

//----------------------------------------------------------------------------

#include "systimer.h"
#include "therm.h"
#include "fan.h"
#include "data.h"
#include "ditherdac.h"
#include "overadc.h"

//------------------------------- Константы: ---------------------------------

//Предельные параметры:

#define V_SCALE_MIN  10.00 //наименьшая шкала напряжения, В
#define I_SCALE_MIN  1.000 //наименьшая шкала тока, А
#define P_SCALE_MIN    1.0 //наименьшая шкала мощности, Вт
#define V_SCALE_NOM  36.00 //номинальная шкала напряжения, В
#define I_SCALE_NOM  4.000 //номинальная шкала тока, А
#define P_SCALE_NOM  100.0 //номинальная шкала мощности, Вт
#define V_SCALE_MAX  99.99 //наибольшая шкала напряжения, В
#define I_SCALE_MAX  9.999 //наибольшая шкала тока, А
#define P_SCALE_MAX  999.9 //наибольшая шкала мощности, Вт
#define V_DEFAULT     5.00 //напряжение по умолчанию, В
#define I_DEFAULT    1.000 //ток по умолчанию, А

//Разрешение:

#define V_RES         0.01 //разрешение по напряжению, В
#define I_RES        0.001 //разрешение по току, А
#define P_RES          0.1 //разрешение по мощности, Вт

//Перевод естественных величин (вольты, амперы, ватты)
//в их внутренне представление:

#define VOLTS2VAL(x) ((uint16_t)(x / V_RES + 0.5))
#define AMPS2VAL(x)  ((uint16_t)(x / I_RES + 0.5))
#define WATTS2VAL(x) ((uint16_t)(x / P_RES + 0.5))

//Делитель для перевода VI в P:

#define VI2P ((uint32_t)(P_RES / (V_RES * I_RES)))

//Характеристики аналогового тракта:

#define V_REF         3.30 //опорное напряжение, В
#define R28           1500 //верхний резистор делителя COM, Ом
#define R29           49.9 //нижний резистор делителя COM, Ом
#define R70          20000 //резисторы R70 = R71 диффусилителя напряжения, Ом
#define R61         100000 //резисторы R61 = R62 диффусилителя напряжения, Ом
#define R57          10000 //резисторы R57 = R58 диффусилителя напряжения, Ом
#define R63          10000 //резисторы R63 = R64 диффусилителя тока, Ом
#define R59         100000 //резисторы R59 = R60 диффусилителя тока, Ом
#define R67R72          75 //датчик тока R67 || R72, мОм

#define V_COM    (V_REF * R29 / (R28 + R29))

#define DP_I         0.005 //порог индикации DP, A
#define ZERO_V        0.00 //загружаемое напряжение при OUT OFF, В
#define ZERO_I       0.001 //загружаемый ток при OUT OFF, А

#define DP_VAL AMPS2VAL(DP_I)
#define ZV_VAL VOLTS2VAL(ZERO_V)
#define ZI_VAL AMPS2VAL(ZERO_I)

//Для успешной калибровки нужно:
//V_COM + V_DAC_FS < V_REF * 0.95
//V_COM + I_DAC_FS < V_REF * 0.95
//Для получения максимальной точности желательно:
//V_COM + V_DAC_FS > V_REF * 0.75
//V_COM + I_DAC_FS > V_REF * 0.75
//Для выбранного выходного напряжения и тока это достигается правильным
//выбором резисторов диффусилителей напряжения и тока, а также датчика тока.

#define V_DAC_FS (V_SCALE_NOM / ((R70 + R61) / R57))
#define I_DAC_FS (I_SCALE_NOM * R67R72 / 1000 * R59 / R63)

//Коды значений:

//Коды значений:

#define VMIN VOLTS2VAL(V_SCALE_MIN)
#define IMIN AMPS2VAL(I_SCALE_MIN)
#define PMIN WATTS2VAL(P_SCALE_MIN)
#define VNOM VOLTS2VAL(V_SCALE_NOM)
#define INOM AMPS2VAL(I_SCALE_NOM)
#define PNOM WATTS2VAL(P_SCALE_NOM)
#define VMAX VOLTS2VAL(V_SCALE_MAX)
#define IMAX AMPS2VAL(I_SCALE_MAX)
#define PMAX WATTS2VAL(P_SCALE_MAX)
#define VDEF VOLTS2VAL(V_DEFAULT)
#define IDEF AMPS2VAL(I_DEFAULT)

//краткие синонимы полной шкалы АЦП и ЦАП:

#define ADCM ADC_MAX_CODE
#define DACM DAC_MAX_CODE

//Параметры калибровки:

enum CalibData_t
{
  CAL_VP1,  //калибровка V, точка 1
  CAL_VC1,  //калибровка V, код 1
  CAL_VP2,  //калибровка V, точка 2
  CAL_VC2,  //калибровка V, код 2
  CAL_IP1,  //калибровка I, точка 1
  CAL_IC1,  //калибровка I, код 1
  CAL_IP2,  //калибровка I, точка 2
  CAL_IC2,  //калибровка I, код 2
  CAL_STR,  //сохранение калибровки
  CAL_VM1,  //калибровка MeterV, код 1
  CAL_VM2,  //калибровка MeterV, код 2
  CAL_IM1,  //калибровка MeterI, код 1
  CAL_IM2,  //калибровка MeterI, код 2
  CAL_CNT
};

//Пределы для калибровочных точек:

#define VP1_MIN         0.01 //минимальное значение точки 1 по напряжению, В
#define VP1_MAX         8.99 //максимальное значение точки 1 по напряжению, В
#define VP2_MIN         9.00 //минимальное значение точки 2 по напряжению, В
#define VP2_MAX  V_SCALE_NOM //максимальное значение точки 2 по напряжению, В
#define IP1_MIN        0.001 //минимальное значение точки 1 по току, А
#define IP1_MAX        0.899 //максимальное значение точки 1 по току, А
#define IP2_MIN        0.900 //минимальное значение точки 2 по току, А
#define IP2_MAX  I_SCALE_NOM //максимальное значение точки 2 по току, А

//Коды пределов для калибровочных точек:

#define VP1L  VOLTS2VAL(VP1_MIN)
#define VP1H  VOLTS2VAL(VP1_MAX)
#define VP2L  VOLTS2VAL(VP2_MIN)
#define VP2H  VOLTS2VAL(VP2_MAX)
#define IP1L  AMPS2VAL(IP1_MIN)
#define IP1H  AMPS2VAL(IP1_MAX)
#define IP2L  AMPS2VAL(IP2_MIN)
#define IP2H  AMPS2VAL(IP2_MAX)

//Промежуточные константы для расчета номинальных калибровочных коэффициентов:

#define V_DELTA    1.00 //ном. расстояние калиб. точек от краев шкалы, В
#define I_DELTA   0.100 //ном. расстояние калиб. точек от краев шкалы, А

#define DAC_LSB   (V_REF / DAC_MAX_CODE)             //0.0000503663 (example)
#define COM_DAC   (V_COM / DAC_LSB)                  //2109.457
#define V_OUT_RES (DAC_LSB * V_SCALE_NOM / V_DAC_FS) //0.0006043956
#define I_OUT_RES (DAC_LSB * I_SCALE_NOM / I_DAC_FS) //0.000067155

#define ADC_LSB   (V_REF / ADC_MAX_CODE)             //0.0000503663
#define COM_ADC   (V_COM / ADC_LSB)                  //2109.457
#define V_IN_RES  (ADC_LSB * V_SCALE_NOM / V_DAC_FS) //0.0006043956
#define I_IN_RES  (ADC_LSB * I_SCALE_NOM / I_DAC_FS) //0.000067155

//Номинальные калибровочные коэффициенты:

#define VPD1 ((uint16_t)(V_DELTA / V_RES + 0.5))
#define VPD2 ((uint16_t)((V_SCALE_NOM - V_DELTA) / V_RES + 0.5))
#define IPD1 ((uint16_t)(I_DELTA / I_RES + 0.5))
//#define IPD2 ((uint16_t)((I_SCALE_NOM - I_DELTA) / I_RES + 0.5))
#define IPD2 ((uint16_t)(1.900 / I_RES + 0.5)) //для уменьшения нагрева шунта

#define VCD1 ((uint16_t)(VPD1 * V_RES / V_OUT_RES + COM_DAC + 0.5)) //3764
#define VCD2 ((uint16_t)(VPD2 * V_RES / V_OUT_RES + COM_DAC + 0.5)) //60019
#define ICD1 ((uint16_t)(IPD1 * I_RES / I_OUT_RES + COM_DAC + 0.5)) //3599
#define ICD2 ((uint16_t)(IPD2 * I_RES / I_OUT_RES + COM_DAC + 0.5)) //30402

#define VMD1 ((uint16_t)(VPD1 * V_RES / V_IN_RES  + COM_ADC + 0.5)) //3764
#define VMD2 ((uint16_t)(VPD2 * V_RES / V_IN_RES  + COM_ADC + 0.5)) //60019
#define IMD1 ((uint16_t)(IPD1 * I_RES / I_IN_RES  + COM_ADC + 0.5)) //3599
#define IMD2 ((uint16_t)(IPD2 * I_RES / I_IN_RES  + COM_ADC + 0.5)) //30402

//DAC_I при калибровке V и DAC_V при калибровке I загружается
//в кодах (1/8 шкалы), чтобы избежать влияния текущей калибровки:

#define DAC_CAL_CODE (DAC_MAX_CODE / 8)

//параметры измерителей:

#define ADC_TUPD 320 //период обновления измеренных значений, мс
#define CVCC_DEL 120 //задержка выхода из CV/CC, мс
#define FIR_POINTS (ADC_TUPD * ADC_FS / OVER_N / 1000)
#define ADC_PIN_V PIN7
#define ADC_PIN_I PIN6
#define ADC_CH_V 1
#define ADC_CH_I 0
#define DAC_CH_V 1
#define DAC_CH_I 0

//Режимы работы измерителей:

enum MeterMode_t { METER_AVG, METER_PKH, METER_PKL };

#define HOLD_TIME      1000 //время удержания пиковых показаний, мс
#define PVG_PER          50 //максимальный период импульсов PVG, мс
#define OUT_BLINK_TIME  200 //время гашения LED OUT при работе таймера, мс

//Состояние CV/CC:

enum CvCcState_t
{
  PS_UNREG = 0,
  PS_CV    = 1,
  PS_CC    = 2,
  PS_CVCC  = 3
};

//Состояние защиты:

enum PrState_t
{
  PR_OK    = 0,
  PR_OVP   = 1,
  PR_OCP   = 2,
  PR_OPP   = 4,
  PR_OTP   = 8
};

//----------------------------------------------------------------------------
//----------------------------- Класс TScaler: -------------------------------
//----------------------------------------------------------------------------

class TScaler
{
private:
  int64_t Kx;
  int64_t Sx;
public:
  TScaler(void) {};
  void Calibrate(uint16_t p1, uint16_t c1,
                 uint16_t p2, uint16_t c2);
  uint16_t CodeToValue(uint16_t code);
  uint16_t ValueToCode(uint16_t value);
};

//----------------------------------------------------------------------------
//------------------------- Шаблонный класс TDac: ----------------------------
//----------------------------------------------------------------------------

template<uint8_t DacN>
class TDac : public TScaler
{
private:
  TDitherDac<DacN> Dac;
  uint16_t Code;
  uint16_t ZeroCode;
  bool On;
public:
  TDac(void);
  void SetCode(uint16_t c);
  void SetZero(uint16_t z);
  void SetValue(uint16_t v);
  void OnOff(bool on);
};

//------------------------- Реализация методов: ------------------------------

template<uint8_t DacN>
TDac<DacN>::TDac(void) : On(0), Code(0)
{
  Dac.Init();
}

template<uint8_t DacN>
void TDac<DacN>::SetCode(uint16_t c)
{
  Code = c;
  if(On) Dac = Code;
}

template<uint8_t DacN>
void TDac<DacN>::SetZero(uint16_t z)
{
  ZeroCode = ValueToCode(z);
  if(!On) Dac = ZeroCode;
}

template<uint8_t DacN>
void TDac<DacN>::SetValue(uint16_t v)
{
  Code = ValueToCode(v);
  if(On) Dac = Code;
}

template<uint8_t DacN>
void TDac<DacN>::OnOff(bool on)
{
  On = on;
  Dac = On? Code : ZeroCode;
}

//----------------------------------------------------------------------------
//------------------------- Шаблонный класс TAdc: ----------------------------
//----------------------------------------------------------------------------

template<uint8_t AdcN, uint8_t AdcPin>
class TAdc : public TScaler
{
private:
  TOverAdc<AdcN, AdcPin> Adc;
  uint32_t FirCode;
  uint16_t FirPointer;
  bool ReadyFlag;
  uint16_t HoldTime;
  char Mode;
public:
  TAdc(void);
  void Execute(void);
  void SetMode(char m);
  bool FastUpdate;
  uint16_t FastCode;
  uint16_t FastValue;
  bool Query(void);
  bool Ready(void);
  void Sync(void);
  uint16_t Code;
  uint16_t Value;
};

//------------------------- Реализация методов: ------------------------------

template<uint8_t AdcN, uint8_t AdcPin>
TAdc<AdcN, AdcPin>::TAdc(void)
{
  Adc.Init();
  Mode = METER_AVG;
  HoldTime = 0;
  FastUpdate = 0;
  FirCode = 0;
  FirPointer = 0;
  FastCode = 0;
  FastValue = 0;
  ReadyFlag = 0;
  Code = 0;
  Value = 0;
}

template<uint8_t AdcN, uint8_t AdcPin>
void TAdc<AdcN, AdcPin>::SetMode(char m)
{
  Mode = m;
  HoldTime = 0;
}

template<uint8_t AdcN, uint8_t AdcPin>
void TAdc<AdcN, AdcPin>::Execute(void)
{
  if(Adc.Ready())
  {
    FastCode = Adc;
    FastValue = CodeToValue(FastCode);
    if(HoldTime) HoldTime--;
    FirCode += FastCode;
    if(++FirPointer == FIR_POINTS)
    {
      Code = (FirCode + FIR_POINTS / 2) / FIR_POINTS;
      if(Mode == METER_AVG)
      {
        Value = CodeToValue(Code);
      }
      FirCode = FirPointer = 0;
      ReadyFlag = 1;
    }
    if(Mode == METER_PKH)
    {
      if(FastValue >= Value)
      {
        Value = FastValue;
        HoldTime = HOLD_TIME;
      }
      else
      {
        if(!HoldTime)
          Value = FastValue;
      }
    }
    if(Mode == METER_PKL)
    {
      if(FastValue <= Value)
      {
        Value = FastValue;
        HoldTime = HOLD_TIME;
      }
      else
      {
        if(!HoldTime)
          Value = FastValue;
      }
    }
    FastUpdate = 1;
  }
  else
  {
    FastUpdate = 0;
  }
}

template<uint8_t AdcN, uint8_t AdcPin>
inline bool TAdc<AdcN, AdcPin>::Query(void)
{
  return(ReadyFlag);
}

template<uint8_t AdcN, uint8_t AdcPin>
inline bool TAdc<AdcN, AdcPin>::Ready(void)
{
  if(ReadyFlag)
  {
    ReadyFlag = 0;
    return(1);
  }
  return(0);
}

template<uint8_t AdcN, uint8_t AdcPin>
void TAdc<AdcN, AdcPin>::Sync(void)
{
  FirCode = FirPointer = 0;
  ReadyFlag = 0;
}

//----------------------------------------------------------------------------
//----------------------------- Класс TAnalog: -------------------------------
//----------------------------------------------------------------------------

class TAnalog
{
private:
  TGpio<PORTB, PIN0> Pin_CC;
  TGpio<PORTB, PIN1> Pin_ON;
  TGpio<PORTA, PIN12> Pin_PVG;
  TFan *Fan;
  TTherm *Therm;
  int16_t Temp;
  bool Out;
  char ProtSt;
  char CvCcSt;
  char CvCcPre;
  void Protection(void);
  void Supervisor(void);
  void CvCcControl(void);
  void ThermalControl(void);
  void OffTimer(void);
  TSoftTimer *CCTimer;
  TSoftTimer *CVTimer;
  TSoftTimer *OvpTimer;
  TSoftTimer *OcpTimer;
  TSoftTimer *OppTimer;
  TSoftTimer *OutBlinkTimer;
public:
  TAnalog(void);
  TParamList *CalibData;
  TAdc<ADC_CH_V, ADC_PIN_V> *AdcV;
  TAdc<ADC_CH_I, ADC_PIN_I> *AdcI;
  TDac<DAC_CH_V> *DacV;
  TDac<DAC_CH_I> *DacI;
  void TrimParamsLimits(void);
  void Execute(void);
  void CalibAll(void);
  void CalibDacV(void);
  void CalibDacI(void);
  void CalibAdcV(char p = CAL_STR);
  void CalibAdcI(char p = CAL_STR);
  uint16_t DP_Code;
  uint16_t OffTime;
  void OutControl(bool on);
  bool OutState(void);
  char GetProtSt(void);
  void ClrProtSt(void);
  char GetCvCcSt(void);
  bool IsCV(void);
  bool IsCC(void);
  bool TempUpdate;
  int16_t GetTemp(void);
  char GetSpeed(void);
};

//----------------------------------------------------------------------------

extern TAnalog *Analog;

//----------------------------------------------------------------------------

#endif
