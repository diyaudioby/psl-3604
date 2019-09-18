//----------------------------------------------------------------------------

//Модуль данных, заголовочный файл

//----------------------------------------------------------------------------

#ifndef DATA_H
#define DATA_H

//----------------------------------------------------------------------------

#include "display.h"
#include "eeprom.h"

//------------------------------- Константы: ---------------------------------

#define PRESETS 10 //количество пресетов
#define RING_V 160 //размер кольцевого буфера V

#define DMAX   999 //макс. задаваемое значение задержки, мс

#define TMIN   200 //мин. задаваемое значение температуры, x0.1°C
#define TMAX   999 //макс. задаваемое значение температуры, x0.1°C
#define TNOM   600 //номинальная температура защиты OTP, x0.1°C
#define DTAL    30 //разность для порога индикации перегрева, x0.1°C
#define TFNL   450 //ном. температура включения вентилятора, x0.1°C
#define TFNH   550 //ном. температура полной скорости вентилятора, x0.1°C

#define TIMMAX (18 * 60 * 60) //максимальное время таймера 18 часов

//Параметры пределов:
//(в TList должны добавляться в такой же последовательности)

enum TopPars_t
{
  PAR_MAXV, //максимальное напряжение
  PAR_MAXI, //максимальный ток
  PAR_MAXP, //максимальная мощность
  PARS_TOP
};

//Основные параметры:
//(в TList должны добавляться в такой же последовательности)

enum MainPars_t
{
  PAR_V,    //установленное напряжение
  PAR_I,    //установленный ток
  PAR_FINE, //состояние FINE (OFF/ON)
  PARS_MAIN
};

//Параметры установок:
//(в TList должны добавляться в такой же последовательности)

enum SetupData_t
{
  PAR_CALL, //Call preset
  PAR_STOR, //Store preset
  PAR_LOCK, //Lock controls
  PAR_OVP,  //OVP threshold
  PAR_OCP,  //OCP threshold
  PAR_OPP,  //OPP threshold
  PAR_DEL,  //OVP/OCP delay
  PAR_OTP,  //OTP threshold
  PAR_FNL,  //Fan start temperature
  PAR_FNH,  //Fan full speed temperature
  PAR_HST,  //Heatsink measured temperature
  PAR_TIM,  //Timer
  PAR_TRC,  //Track (OFF/AUTOLOCK/ON)
  PAR_CON,  //Confirm (OFF/ON)
  PAR_POW,  //Display power (OFF/ON)
  PAR_SET,  //Display setpoint when regulated (OFF/ON)
  PAR_GET,  //Always display maesured values(OFF/ON)
  PAR_APV,  //Display average/peak V (AVERAGE/PEAK HIGH/PEAK LOW)
  PAR_APC,  //Display average/peak I (AVERAGE/PEAK HIGH/PEAK LOW)
  PAR_PRC,  //Current preview (OFF/ON)
  PAR_DNP,  //Down programmer (OFF/ON)
  PAR_OUT,  //Restore out state (OFF/ON)
  PAR_SND,  //Sound (OFF/ALARM/ON)
  PAR_ENR,  //Encoder reverse (OFF/ON)
  PAR_SPL,  //Splash screen (OFF/ON)
  PAR_INF,  //Firmware version info
  PAR_DEF,  //Load defaults
  PAR_CAL,  //Calibration (NO/YES/DEFAULT)
  PAR_ESC,  //Escape menu (NO/YES)
  PARS_SETUP
};

enum ParType_t //тип параметра
{
  PT_V,     //напряжение, x0.01 V
  PT_PRV,   //напряжение защиты, x0.01 V
  PT_I,     //ток, x0.001 A
  PT_PRI,   //ток защиты, x0.001 A
  PT_P,     //мощность, x0.1 W
  PT_PRP,   //мощность защиты, x0.1 W
  PT_VC,    //код напряжения
  PT_IC,    //код тока
  PT_PRE,   //CALL/STORE (NOSAVE)
  PT_OFFON, //OFF/ON
  PT_FALN,  //OFF/ALARM/ON
  PT_APHPL, //AVERAGE/PEAK HIGH/PEAK LOW
  PT_DEL,   //задержка, мс
  PT_T,     //температура, x0.1°C
  PT_FIRM,  //Firmware Version (NOSAVE)
  PT_NY,    //вывод текста (NOSAVE)
  PT_NYDEF, //NO/YES/DEFAULT (NOSAVE)
  PT_TIM    //время таймера
};

enum OffOn_t { OFF, ON };
enum Track_t { TRCOFF, TRCAUTO, TRCON };
enum NoYes_t { NO, YES, DEFAULT };

#define PROT_FLAG 0x80 //флаг срабатывания защиты
#define ON_FLAG 0x8000 //флаг включения выхода
#define VER ((uint16_t)(VERSION * 100 + 0.5))

//----------------------------------------------------------------------------
//----------------------------- Класс TParam: --------------------------------
//----------------------------------------------------------------------------

class TParam
{
private:
  char Name[DIGS + 1];
public:
  char Type;
  TParam(ParType_t type, const char *s,
         uint16_t min, uint16_t nom, uint16_t max);
  uint16_t Min;
  uint16_t Nom;
  uint16_t Max;
  uint16_t Value;
  void ShowName(void);
  void ShowValue(void);
  bool Savable(void);
  bool Validate(void);
  bool Edit(int16_t step);
};

//----------------------------------------------------------------------------
//-------------------------- Шаблонный класс TList: --------------------------
//----------------------------------------------------------------------------

template<class T>
class TList
{
private:
  char ItemsMax;
public:
  TList(char max);
  T** Items;
  char ItemsCount;
  void AddItem(T *t);
};

//-------------------------- Реализация методов: -----------------------------

template<class T>
TList<T>::TList(char max)
{
  ItemsMax = max;
  Items = new T*[ItemsMax];
  ItemsCount = 0;
}

template<class T>
void TList<T>::AddItem(T *t)
{
  if(ItemsCount < ItemsMax)
    Items[ItemsCount++] = t;
}

//----------------------------------------------------------------------------
//--------------------------- Класс TParamList: ------------------------------
//----------------------------------------------------------------------------

class TParamList : public TList<TParam>
{
private:
public:
  TParamList(char max) : TList(max) {};
  TEeSection *EeSection;
  void LoadDefaults(void);
  void ReadFromEeprom(char n);
  void ReadFromEeprom(void);
  void SaveToEeprom(char n);
  void SaveToEeprom(void);
};

//----------------------------------------------------------------------------
//----------------------------- Класс TData: ---------------------------------
//----------------------------------------------------------------------------

class TData
{
public:
  TData(void);
  TParamList *TopData;
  TParamList *MainData;
  TParamList *SetupData;
  TEeSection *PresetV;
  TEeSection *PresetI;
  TRingSection *Ring;
  bool OutOn;
  void SetVI(void);
  void Apply(char par);
  void ApplyAll(void);
  void TrimParamsLimits(void);
  void ReadV(void);
  void SaveV(void);
  void InitPresets(void);
  void ReadPreset(char n);
  void SavePreset(char n);
};

//----------------------------------------------------------------------------

extern TData *Data;

//----------------------------------------------------------------------------

#endif
