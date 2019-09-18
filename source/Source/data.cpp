//----------------------------------------------------------------------------

//Модуль данных

//----------------------------------------------------------------------------

#include "main.h"
#include "data.h"
#include "analog.h"
#include "sound.h"
#include "encoder.h"

//----------------------------------------------------------------------------
//----------------------------- Класс TParam: --------------------------------
//----------------------------------------------------------------------------

//----------------------------- Конструктор: ---------------------------------

TParam::TParam(ParType_t type, const char *s,
               uint16_t min, uint16_t nom, uint16_t max) :
               Type(type), Min(min), Nom(nom), Max(max)
{
  for(char i = 0; i < DIGS + 1; i++)
    Name[i] = *s++;
  Value = Nom; //начальная инициализация, дальше прочитается из EEPROM
}

//------------------------- Вывод имени параметра: ---------------------------

void TParam::ShowName(void)
{
  if(Name[0]) //если пустая строка, то она не выводится
  {
    char line = 0;
    if(Type == PT_V || Type == PT_PRV || Type == PT_VC) line = 1;
    Display->SetPos(line, 0);
    Display->PutString(Name);
  }
}

//----------------------- Вывод значения параметра: --------------------------

void TParam::ShowValue(void)
{
  Display->SetPos((Type == PT_V || Type == PT_PRV)? 0 : 1, 0);
  switch(Type)
  {
  case PT_V:
  case PT_PRV:   Display->PutIntF(Value, 4, 2);
                 break;
  case PT_I:
  case PT_PRI:   Display->PutIntF(Value, 4, 3);
                 break;
  case PT_P:
  case PT_PRP:   Display->PutIntF(Value, 4, 1);
                 break;
  case PT_PRE:   if(Value == 0) Display->PutString("CALL");
                 if(Value == 1) Display->PutString("Stor");
                 break;
  case PT_OFFON: if(Value == 0) Display->PutString(" OFF");
                 if(Value == 1) Display->PutString(" On ");
                 break;
  case PT_FALN:  if(Value == 0) Display->PutString(" OFF");
                 if(Value == 1) Display->PutString(" AL ");
                 if(Value == 2) Display->PutString(" On ");
                 break;
  case PT_APHPL: if(Value == 0) Display->PutString(" AG ");
                 if(Value == 1) Display->PutString(" PH ");
                 if(Value == 2) Display->PutString(" PL ");
                 break;
  case PT_DEL:   Display->PutIntF(Value, 4, 0);
                 break;
  case PT_T:     Display->PutIntF(Value, 3, 1);
                 Display->PutChar('*');
                 break;
  case PT_FIRM:  Display->PutIntF(Value, 4, 2);
                 break;
  case PT_NY:    if(Value == 0) Display->PutString(" nO ");
                 if(Value == 1) Display->PutString(" YES");
                 break;
  case PT_NYDEF: if(Value == 0) Display->PutString(" nO ");
                 if(Value == 1) Display->PutString(" YES");
                 if(Value == 2) Display->PutString(" dEF");
                 break;
  case PT_TIM:   uint8_t h = Value / 3600;
                 uint16_t v = Value % 3600;
                 uint8_t m = v / 60;
                 uint8_t s = v % 60;
                 Display->SetPos(0, 2);
                 Display->PutChar(h / 10); Display->PutChar(h % 10 + POINT);
                 Display->PutChar(m / 10); Display->PutChar(m % 10 + POINT);
                 Display->PutChar(s / 10); Display->PutChar(s % 10);
                 break;
  }
  //если для параметра защиты Value >= Max,
  //вместо значения выводится надпись OFF:
  if((Type == PT_PRV || Type == PT_PRI || Type == PT_PRP) && Value >= Max)
  {
    Display->SetPos((Type == PT_PRV)? 0 : 1, 0);
    Display->PutString(" OFF");
  }
}

//-------------- Проверка, сохраняется ли параметр в EEPROM: -----------------

bool TParam::Savable(void)
{
  return((Type != PT_NY) && (Type != PT_PRE) &&
         (Type != PT_FIRM) && (Type != PT_NYDEF) && (Type != PT_TIM));
}

//------------------ Ограничение диапазона параметра: ------------------------


bool TParam::Validate(void)
{
  if(Value <= Min) { Value = Min; return(1); }
  if(Value >= Max) { Value = Max; return(1); }
  return(0); //если значение было изменено, возвращает true
}

//--------------------- Редактирование параметра: ----------------------------

bool TParam::Edit(int16_t step)
{
  //если Min = Max, редактирование параметра запрещено:
  if(Min == Max) return(0);
  //для малых значений всегда единичный шаг:
  if(step && Max < 10) step = (step < 0)? -1 : 1;
  //коррекция шага для редактирования времени:
  if((Type == PT_TIM) && (step < -1 || step > 1)) step *= 6;
  if(step < 0 && Value <= Min) return(0);
  if(step > 0 && Value >= Max) return(0);
  Validate();
  //выравнивание на шаг:
  int16_t rem = Value % step;
  Value = Value - rem + step;
  if(rem && step < 0) Value = Value - step;
  Validate();
  return(1);
}

//----------------------------------------------------------------------------
//--------------------------- Класс TParamList: ------------------------------
//----------------------------------------------------------------------------

//-------------------- Загрузка значений по умолчанию: -----------------------

void TParamList::LoadDefaults(void)
{
  for(char i = 0; i < ItemsCount; i++)
    Items[i]->Value = Items[i]->Nom;
}

//---------------------- Чтение параметра из EEPROM: -------------------------

void TParamList::ReadFromEeprom(char n)
{
  if(Items[n]->Savable())
  {
    if(EeSection->Valid)
    {
      Items[n]->Value = EeSection->Read(n);
      Items[n]->Validate();
    }
    else
    {
      Items[n]->Value = Items[n]->Nom;
      SaveToEeprom(n);
    }
  }
}

//------------------ Чтение списка параметров из EEPROM: ---------------------

void TParamList::ReadFromEeprom(void)
{
  for(char i = 0; i < ItemsCount; i++)
    ReadFromEeprom(i);
  if(!EeSection->Valid)
    EeSection->Validate();
}

//--------------------- Сохранение параметрa в EEPROM: -----------------------

void TParamList::SaveToEeprom(char n)
{
  if(Items[n]->Savable())
    EeSection->Update(n, Items[n]->Value);
}

//----------------- Сохранение списка параметров в EEPROM: -------------------

void TParamList::SaveToEeprom(void)
{
  for(char i = 0; i < ItemsCount; i++)
    SaveToEeprom(i);
  EeSection->Validate();
}

//----------------------------------------------------------------------------
//------------------------------ Класс TData: --------------------------------
//----------------------------------------------------------------------------

//----------------------------- Конструктор: ---------------------------------

TData::TData(void)
{
  //Параметры в TList должны добавляться в той последовательности, в которой
  //индексы описаны в enum. Можно, конечно, было сделать сортировку списка
  //по тэгу или поиск Item по тэгу, но пока это оставил на будущее.
  TopData = new TParamList(PARS_TOP);
  //                          Type     Name    Min   Nom   Max
  TopData->AddItem(new TParam(PT_V,   "-tOP", VMIN, VNOM, VMAX)); //PAR_MAXV
  TopData->AddItem(new TParam(PT_I,   "tOP-", IMIN, INOM, IMAX)); //PAR_MAXI
  TopData->AddItem(new TParam(PT_P,   "tPP-", PMIN, PNOM, PMAX)); //PAR_MAXP
  TopData->EeSection = new TEeSection(TopData->ItemsCount);
  TopData->ReadFromEeprom();

  MainData = new TParamList(PARS_MAIN);
  //                           Type     Name   Min   Nom   Max
  MainData->AddItem(new TParam(PT_V,     "",    0, VDEF, VMAX)); //PAR_V
  MainData->AddItem(new TParam(PT_I,     "",    0, IDEF, IMAX)); //PAR_I
  MainData->AddItem(new TParam(PT_OFFON, "",    0,    0,    1)); //PAR_FINE
  MainData->EeSection = new TEeSection(MainData->ItemsCount);
  MainData->ReadFromEeprom();

  SetupData = new TParamList(PARS_SETUP);
  //                            Type       Name  Min  Nom  Max
  SetupData->AddItem(new TParam(PT_PRE,   "PrE-",  0,   0,   0));   //PAR_CALL
  SetupData->AddItem(new TParam(PT_PRE,   "PrE-",  1,   1,   1));   //PAR_STOR
  SetupData->AddItem(new TParam(PT_OFFON, " Lc-",  0,   0,   1));   //PAR_LOCK
  SetupData->AddItem(new TParam(PT_PRV,   "-OUP",  0, VNOM, VMAX)); //PAR_OVP
  SetupData->AddItem(new TParam(PT_PRI,   "OCP-",  0, INOM, IMAX)); //PAR_OCP
  SetupData->AddItem(new TParam(PT_PRP,   "OPP-",  0, PNOM, PMAX)); //PAR_OPP
  SetupData->AddItem(new TParam(PT_DEL,   "dEL-",  0,   0, DMAX));  //PAR_DEL
  SetupData->AddItem(new TParam(PT_T,     "OtP-",  TMIN, TNOM, TMAX)); //PAR_OTP
  SetupData->AddItem(new TParam(PT_T,     "FnL-",  TMIN, TFNL, TMAX)); //PAR_FNL
  SetupData->AddItem(new TParam(PT_T,     "FnH-",  TMIN, TFNH, TMAX)); //PAR_FNH
  SetupData->AddItem(new TParam(PT_T,     "HSt-",  0,   0,   0));   //PAR_HST
  SetupData->AddItem(new TParam(PT_TIM,   "t-",    0,   0, TIMMAX));   //PAR_TIM
  SetupData->AddItem(new TParam(PT_FALN,  "trc-",  0,   2,   2));   //PAR_TRC
  SetupData->AddItem(new TParam(PT_OFFON, "Con-",  0,   0,   1));   //PAR_CON
  SetupData->AddItem(new TParam(PT_OFFON, " P- ",  0,   0,   1));   //PAR_POW
  SetupData->AddItem(new TParam(PT_OFFON, "SEt-",  0,   1,   1));   //PAR_SET
  SetupData->AddItem(new TParam(PT_OFFON, "GEt-",  0,   0,   1));   //PAR_GET
  SetupData->AddItem(new TParam(PT_APHPL, "APU-",  0,   0,   2));   //PAR_APV
  SetupData->AddItem(new TParam(PT_APHPL, "APC-",  0,   0,   2));   //PAR_APC
  SetupData->AddItem(new TParam(PT_OFFON, "PrC-",  0,   0,   1));   //PAR_PRC
  SetupData->AddItem(new TParam(PT_OFFON, "dnP-",  0,   0,   1));   //PAR_DNP
  SetupData->AddItem(new TParam(PT_OFFON, "Out-",  0,   0,   1));   //PAR_OUT
  SetupData->AddItem(new TParam(PT_FALN,  "Snd-",  0,   2,   2));   //PAR_SND
  SetupData->AddItem(new TParam(PT_OFFON, "Enr-",  0,   0,   1));   //PAR_ENR
  SetupData->AddItem(new TParam(PT_OFFON, "SPL-",  0,   1,   1));   //PAR_SPL
  SetupData->AddItem(new TParam(PT_FIRM,  "InF-",  0, VER,   0));   //PAR_INF
  SetupData->AddItem(new TParam(PT_NY,    "dEF-",  0,   0,   1));   //PAR_DEF
  SetupData->AddItem(new TParam(PT_NY,    "CAL-",  0,   0,   1));   //PAR_CAL
  SetupData->AddItem(new TParam(PT_NY,    "ESC-",  1,   1,   1));   //PAR_ESC
  SetupData->EeSection = new TEeSection(SetupData->ItemsCount);
  SetupData->ReadFromEeprom();

  //чтение сохраненного значения V из кольцевого буфера:
  Ring = new TRingSection(RING_V);
  ReadV();
  //инициализация пресетов:
  PresetV = new TEeSection(PRESETS);
  PresetI = new TEeSection(PRESETS);
  InitPresets();
  //коррекция пределов согласно параметрам Top:
  TrimParamsLimits();
}

//---------- Коррекция пределов параметров согласно MAX_V и MAX_I: -----------

void TData::TrimParamsLimits(void)
{
  MainData->Items[PAR_V]->Max = TopData->Items[PAR_MAXV]->Value;
  MainData->Items[PAR_V]->Validate();
  MainData->Items[PAR_I]->Max = TopData->Items[PAR_MAXI]->Value;
  MainData->Items[PAR_I]->Validate();
  SetupData->Items[PAR_OVP]->Max = TopData->Items[PAR_MAXV]->Value;
  SetupData->Items[PAR_OVP]->Validate();
  SetupData->Items[PAR_OCP]->Max = TopData->Items[PAR_MAXI]->Value;
  SetupData->Items[PAR_OCP]->Validate();
  SetupData->Items[PAR_OPP]->Max = TopData->Items[PAR_MAXP]->Value;
  SetupData->Items[PAR_OPP]->Validate();
}

//-------------------------- Применение параметра: ---------------------------

void TData::Apply(char par)
{
  if(par < PARS_SETUP)
  {
    uint16_t val = SetupData->Items[par]->Value;
    switch(par)
    {
    case PAR_TIM: Analog->OffTime = val; break;
    case PAR_APV: Analog->AdcV->SetMode(val); break;
    case PAR_APC: Analog->AdcI->SetMode(val); break;
    case PAR_DNP: Analog->OutControl(Analog->OutState()); break;
    case PAR_OUT: if(val == ON) Data->SaveV(); break;
    case PAR_SND: Sound->SetMode((SndMode_t)val); break;
    case PAR_ENR: Encoder->Rev = val; break;
    case PAR_DEF: if(val == YES)
                  {
                    SetupData->LoadDefaults();
                    SetupData->SaveToEeprom();
                  }
                  break;
    }
  }
}

//---------------------- Применение всех параметров: -------------------------

void TData::ApplyAll(void)
{
  //используется только в конструкторе TControl
  for(char i = 0; i < PARS_SETUP; i++)
    if((i != PAR_DNP) && (i != PAR_OUT)) Apply(i);
}

//------------------- Чтение V + OUT ON/OFF из EEPROM: -----------------------

inline void TData::ReadV(void)
{
  if(Ring->Valid)
  {
    uint16_t v = Ring->Read();
    MainData->Items[PAR_V]->Value = v & ~ON_FLAG;
    if(SetupData->Items[PAR_OUT]->Value == ON)
      OutOn = v & ON_FLAG;
        else OutOn = 0;
    MainData->Items[PAR_V]->Validate();
  }
  else
  {
    MainData->Items[PAR_V]->Value = MainData->Items[PAR_V]->Nom;
    OutOn = 0;
    Ring->Update(MainData->Items[PAR_V]->Value);
    Ring->Validate();
  }
}

//------------------ Сохранение V + OUT ON/OFF в EEPROM: ---------------------

void TData::SaveV(void)
{
  uint16_t v = MainData->Items[PAR_V]->Value;
  OutOn = Analog->OutState();
  if(OutOn) v |= ON_FLAG;
  Display->Off();
  Ring->Update(v);
  Display->On();
}

//--------------------------- Установка VI: ----------------------------------

void TData::SetVI(void)
{
  Analog->DacV->SetValue(MainData->Items[PAR_V]->Value); //загрузка DAC_V
  Analog->DacI->SetValue(MainData->Items[PAR_I]->Value); //загрузка DAC_I
  Analog->OutControl(OutOn); //OUT and DP ON/OFF
}

//---------------------- Инициализация пресетов: -----------------------------

inline void TData::InitPresets(void)
{
  static const uint16_t PRE_V_INIT[PRESETS] =
    {330, 500, 900, 1000, 1200, 1500, 1800, 2400, 2700, 3600 };

  static const uint16_t PRE_I_INIT = 1000;

  if(!(PresetV->Valid && PresetI->Valid))
  {
    for(char i = 0; i < PRESETS; i++)
    {
      PresetV->Update(i, PRE_V_INIT[i]);
      PresetI->Update(i, PRE_I_INIT);
    }
    PresetV->Validate();
    PresetI->Validate();
  }
}

//--------------------- Чтение пресета из EEPROM: ----------------------------

void TData::ReadPreset(char n)
{
  if(n < PRESETS)
  {
    MainData->Items[PAR_V]->Value = PresetV->Read(n);
    MainData->Items[PAR_I]->Value = PresetI->Read(n);
    MainData->Items[PAR_V]->Validate();
    MainData->Items[PAR_I]->Validate();
  }
}

//-------------------- Сохранение пресета в EEPROM: --------------------------

void TData::SavePreset(char n)
{
  //в пресетах не хранится признак OUT ON/OFF
  if(n < PRESETS)
  {
    Display->Off();
    PresetV->Update(n, MainData->Items[PAR_V]->Value);
    PresetI->Update(n, MainData->Items[PAR_I]->Value);
    Display->On();
  }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
