//----------------------------------------------------------------------------

//Модуль реализации меню

//----------------------------------------------------------------------------

#include "main.h"
#include "menu.h"
#include "sound.h"
#include "encoder.h"
#include "eeprom.h"
#include "control.h"
#include "analog.h"

//------------------------------- Константы: ---------------------------------

#define TIMEOUT_SPLASH  2000 //время индикации splash, мс
#define TIMEOUT_MAIN    5000 //задержка возврата из установки V и I, мс
#define TIMEOUT_SETUP  10000 //задержка возврата из меню, мс
#define TIMEOUT_LOCK    1000 //время индикации Lock

//----------------------------------------------------------------------------
//--------------------- Абстрактный класс TMenuItem: -------------------------
//----------------------------------------------------------------------------

//------------------------- Загрузка параметра: ------------------------------

void TMenuItem::LoadParam(TParam* p)
{
  Par = p;
  //параметры, которые не должны сохраняться,
  //инициализируются номинальными значениями (кроме PAR_TIM):
  if(!Par->Savable() && (Par->Type != PT_TIM))
    Par->Value = Par->Nom;
  Display->Blink(BLINK_NO);
  Par->ShowName();
  Par->ShowValue();
}

//------------------------- Вход в редактирование: ---------------------------

void TMenuItem::EditEnter(void)
{
  Edit = 1;
  Par->ShowValue();
  if(Par->Type == PT_TIM)
    Display->Blink(BLINK_TIM);
      else if(Par->Type == PT_V || Par->Type == PT_PRV)
        Display->Blink(BLINK_V);
          else Display->Blink(BLINK_I);
  BackupV = Par->Value;
}

//------------------------ Выход из редактирования: --------------------------

void TMenuItem::EditExit(void)
{
  if(Edit)
  {
    Edit = 0;
    Display->Blink(BLINK_NO);
    Sound->High();
    Params->SaveToEeprom(ParIndex);
  }
}

//----------------- Выход из редактирования без сохранения: ------------------

void TMenuItem::EditEscape(void)
{
  if(Edit)
  {
    Edit = 0;
    Par->Value = BackupV;
    Display->Blink(BLINK_NO);
  }
  Sound->High();
}

//---------------------------- Событие таймера: ------------------------------

void TMenuItem::OnTimer(void)
{
  EditEscape();
  MnuIndex = MNU_MAIN;
}

//----------------------------------------------------------------------------
//--------------------------- Класс TMenuSplash: -----------------------------
//----------------------------------------------------------------------------

//-------------------------- Инициализация меню: -----------------------------

void TMenuSplash::Init(void)
{
  Edit = 0;
  Display->SetPos(0, 0);
  Display->PutString(DEVICE_NAME);
  Timeout = TIMEOUT_SPLASH;
}

//-------------------------- Событие клавиатуры: -----------------------------

void TMenuSplash::OnKeyboard(KeyMsg_t &msg)
{
  MnuIndex = MNU_MAIN;
  msg = KBD_NOP;
}

//-------------------------- Событие энкодера: -------------------------------

void TMenuSplash::OnEncoder(int8_t &step)
{
  Sound->Beep();
  MnuIndex = MNU_MAIN;
  step = ENC_NOP;
}

//----------------------------------------------------------------------------
//--------------------------- Класс TMenuError: ------------------------------
//----------------------------------------------------------------------------

//-------------------------- Инициализация меню: -----------------------------

void TMenuError::Init(void)
{
  Edit = 0;
  Display->SetPos(0, 0);
  Display->PutString("Err-");
  Display->SetPos(1, 0);
  Display->PutString("E-");
  Display->PutChar((ParIndex >> 4) & 0x0F);
  Display->PutChar(ParIndex & 0x0F);
  Timeout = 0; //не выходить по таймеру из меню Error
}

//-------------------------- Событие клавиатуры: -----------------------------

void TMenuError::OnKeyboard(KeyMsg_t &msg)
{
  MnuIndex = MNU_MAIN;
  msg = KBD_NOP;
}

//-------------------------- Событие энкодера: -------------------------------

void TMenuError::OnEncoder(int8_t &step)
{
  Sound->Beep();
  MnuIndex = MNU_MAIN;
  step = ENC_NOP;
}

//----------------------------------------------------------------------------
//--------------------------- Класс TMenuMain: -------------------------------
//----------------------------------------------------------------------------

//-------------------------- Инициализация меню: -----------------------------

void TMenuMain::Init(void)
{
  ParIndex = 0; //внешний индекс параметра здесь не используется
  Edit = 0;
  Edited = 0;
  Dnp = 0;
  ForceV = 1;
  ForceI = 1;
  Data->SetVI();
  Display->Blink(BLINK_NO);
  Timeout = TIMEOUT_MAIN;
}

//---------------------------- Обновление меню: ------------------------------

void TMenuMain::Execute(void)
{
  //индикация P:
  bool Power = Data->SetupData->Items[PAR_POW]->Value == ON;
  if(Power && ((Analog->AdcV->Query() && Analog->AdcI->Query()) || ForceV))
  {
    uint32_t p = (uint32_t)Analog->AdcV->Value * Analog->AdcI->Value / 100;
    Analog->AdcV->Sync();
    Analog->AdcI->Sync();
    if(Edit && ParIndex == PAR_V)
    {
      Display->SetPos(1, 0);
      Display->PutIntF(p, 4, 3 + AUTO_SCALE);
    }
    else if(Edit && ParIndex == PAR_I)
    {
      Display->SetPos(0, 0);
      Display->PutIntF(p, 4, 3 + AUTO_SCALE);
    }
    else
    {
      Display->SetPos(0, 0);
      Display->PutString(" P- ");
      Display->SetPos(1, 0);
      Display->PutIntF(p, 4, 3 + AUTO_SCALE);
    }
    ForceV = 0; ForceI = 0;
  }
  //индикация V:
  //если новое значение АЦП готово и V не редактируется,
  //то требуется отображать значение V
  if(!Power && (Analog->AdcV->Ready() || ForceV) && !(Edit && ParIndex == PAR_V))
  {
       //если включен режим отображения измеренного значения
    if((Data->SetupData->Items[PAR_GET]->Value == ON) ||
       //или находимся в CC,
       (Analog->IsCC()) ||
       //или выключен режим отображения установленного значения
       //и находимся в CV
       ((Data->SetupData->Items[PAR_SET]->Value == OFF) &&
       (Analog->IsCV())))
    {
      //то отображается измеренное значение:
      Display->SetPos(0, 0);
      Display->PutIntF(Analog->AdcV->Value, 4, 2);
    }
    else
    {
      //иначе отображается установленное значение:
      Params->Items[PAR_V]->ShowValue();
    }
    ForceV = 0;
  }
  //индикация I:
  //если I не редактируется, то требуется отображать значение I
  if(!Power && Analog->AdcI->FastUpdate && !(Edit && ParIndex == PAR_I))
  {
    //проверка отрицательного тока:
    bool dnp = Analog->AdcI->FastCode < Analog->DP_Code;
    if(!dnp && Dnp) Analog->AdcI->Sync();
    Dnp = dnp;
    if(Dnp)
    {
      //если включен выход или включен DP,
      if(Analog->OutState() ||
        (Data->SetupData->Items[PAR_DNP]->Value == ON))
      {
        //отображаются символы "dnP":
        Display->SetPos(1, 0);
        Display->PutString(" dnP");
      }
      else
      {
        //если выбран режим предпросмотра значения I,
        if(Data->SetupData->Items[PAR_PRC]->Value == ON)
        {
          //отображается установленное значение:
          Params->Items[PAR_I]->ShowValue();
        }
        //иначе отображается нулевое значение I:
        else
        {
          Display->SetPos(1, 0);
          Display->PutIntF(0, 4, 3);
        }
      }
    }
    //обновление с каждым циклом АЦП:
    else if(Analog->AdcI->Ready() || ForceI)
    {
         //если включен режим отображения измеренного значения
      if((Data->SetupData->Items[PAR_GET]->Value == ON) ||
         //или находимся в CV,
         (Analog->IsCV()) ||
         //или выключен режим отображения установленного значения
         //и находимся в CC
         ((Data->SetupData->Items[PAR_SET]->Value == OFF) &&
         (Analog->IsCC())))
      {
        //то отображается измеренное значение:
        Display->SetPos(1, 0);
        Display->PutIntF(Analog->AdcI->Value, 4, 3);
      }
      else
      {
        //если выбран режим предпросмотра значения I или выход включен,
        if((Data->SetupData->Items[PAR_PRC]->Value == ON) || Analog->OutState())
        {
          //отображается установленное значение:
          Params->Items[PAR_I]->ShowValue();
        }
        else
        {
          //иначе отображается нулевое значение I:
          Display->SetPos(1, 0);
          Display->PutIntF(0, 4, 3);
        }
      }
    }
    ForceI = 0;
  }
}

//-------------------------- Событие клавиатуры: -----------------------------

void TMenuMain::OnKeyboard(KeyMsg_t &msg)
{
  //управление заблокировано:
  if(Data->SetupData->Items[PAR_LOCK]->Value == ON)
  {
    if((msg == KBD_SETV) ||
       (msg == KBD_SETI) ||
       (msg == KBD_FINE) ||
       (msg == KBD_ENC))
    {
      ParIndex = PAR_LOCK;
      MnuIndex = MNU_SETUP; //переход в меню MNU_SETUP
      msg = KBD_NOP;
      return;
    }
  }
  if(msg == KBD_SETV)
  {
    if(Edit && ParIndex == PAR_V)
    {
      EditExit();
    }
    else
    {
      EditExit();
      ParIndex = PAR_V;
      Par = Params->Items[ParIndex];
      EditEnter();
      Edited = 1;
    }
    msg = KBD_NOP;
    return;
  }
  if(msg == KBD_SETI)
  {
    if(Edit && ParIndex == PAR_I)
    {
      EditExit();
    }
    else
    {
      EditExit();
      ParIndex = PAR_I;
      Par = Params->Items[ParIndex];
      EditEnter();
      Edited = 1;
    }
    msg = KBD_NOP;
    return;
  }
  if(msg == KBD_OUT) //кнопка OUT ON/OFF - выход из редактирования
  {
    EditExit();
    return; //код кнопки не сбрасывется
  }
  if(msg == KBD_ENC)
  {
    if(!Edit)
    {
      //Короткое нажатие энкодера -
      //альтернативный вход в редактирование V:
      ParIndex = PAR_V;
      Par = Params->Items[ParIndex];
      EditEnter();
      Edited = 0;
    }
    else
    {
      EditExit();
      //вход в режим редактирования I, если V не регулировалось:
      if(ParIndex == PAR_V && !Edited)
      {
        ParIndex = PAR_I;
        Par = Params->Items[ParIndex];
        EditEnter();
        Sound->Beep();
      }
    }
    msg = KBD_NOP;
    return;
  }
  //Вход в меню SETUP по удержанию энкодера:
  //меню - дополнительный функционал устройста, его
  //нужно отделить от основного функционала.
  if(msg == KBD_ENCH)
  {
    Sound->Beep();
    ParIndex = PAR_ESC;
    MnuIndex = MNU_SETUP;
    msg = KBD_NOP;
    return;
  }
  if(msg == KBD_SETVH)
  {
    EditEscape();
    ParIndex = PAR_OVP;  //активный параметр PAR_OVP
    MnuIndex = MNU_PROT; //переход в меню MNU_PROT
    msg = KBD_NOP;
    return;
  }
  if(msg == KBD_SETIH)
  {
    EditEscape();
    ParIndex = PAR_OCP;
    MnuIndex = MNU_PROT;
    msg = KBD_NOP;
    return;
  }
  if(msg == KBD_SETVI)
  {
    Data->SetupData->Items[PAR_POW]->Value =
    !Data->SetupData->Items[PAR_POW]->Value;
    Data->SetupData->SaveToEeprom(PAR_POW);
    msg = KBD_NOP;
    return;
  }
}

//-------------------------- Событие энкодера: -------------------------------

void TMenuMain::OnEncoder(int8_t &step)
{
  //управление заблокировано:
  if(Data->SetupData->Items[PAR_LOCK]->Value == ON)
  {
    step = ENC_NOP;
    return;
  }
  if(Edit)
  {
    //редактирование параметра:
    if(Par->Edit(step))
    {
      Par->ShowValue();
      step = ENC_NOP;
    }
    if(Data->SetupData->Items[PAR_CON]->Value == OFF)
    {
      if(ParIndex == PAR_V)
        Analog->DacV->SetValue(Par->Value); //загрузка DAC_V
      if(ParIndex == PAR_I)
        Analog->DacI->SetValue(Par->Value); //загрузка DAC_I
    }
  }
  else
  {
    //вход в редактирование:
    if((Data->SetupData->Items[PAR_TRC]->Value == TRCON) ||
       ((Data->SetupData->Items[PAR_TRC]->Value == TRCAUTO) && !Analog->OutState()))
    {
      ParIndex = PAR_V;
      Par = Params->Items[ParIndex];
      EditEnter();
      Sound->Beep();
      step = ENC_NOP;
    }
    step = ENC_NOP; //нет звука выключенного энкодера
  }
  Edited = 1;
}

//---------------------------- Событие таймера: ------------------------------

void TMenuMain::OnTimer(void)
{
  if(Edit)
  {
    if(Data->SetupData->Items[PAR_CON]->Value == OFF)
      EditExit();
        else { EditEscape(); Par->ShowValue(); }
  }
}

//------------------------ Выход из редактирования: --------------------------

void TMenuMain::EditExit(void)
{
  if(Edit)
  {
    if(ParIndex == PAR_V)
    {
      Edit = 0;
      Display->Blink(BLINK_NO);
      Sound->High();
      Data->SaveV(); //сохранение V в кольцевом буфере EEPROM
    }
    else
    {
      TMenuItem::EditExit();
    }
    if(ParIndex == PAR_V)
      Analog->DacV->SetValue(Par->Value); //загрузка DAC_V
    if(ParIndex == PAR_I)
      Analog->DacI->SetValue(Par->Value); //загрузка DAC_I
  }
  ForceV = 1; ForceI = 1;
}

//----------------------------------------------------------------------------
//--------------------------- Класс TMenuPreset: -----------------------------
//----------------------------------------------------------------------------

//-------------------------- Инициализация меню: -----------------------------

//ParIndex = 0..(PRESETS - 1) - чтение пресета (Edit = 0)
//ParIndex = PRESETS..(2 * PRESETS - 1) - сохранение пресета (Edit = 1)

void TMenuPreset::Init(void)
{
  if(ParIndex < PRESETS) { Edit = 0; } //чтение пресета
    else { Edit = 1; ParIndex -= PRESETS; }  //запись пресета
  EditEnter();
  Timeout = TIMEOUT_SETUP;
}

//-------------------------- Событие клавиатуры: -----------------------------

void TMenuPreset::OnKeyboard(KeyMsg_t &msg)
{
  if(msg == KBD_ENC)
  {
    if(ParIndex == PRESETS)
    {
      EditEscape();
    }
    else
    {
      EditExit();
    }
    Sound->High();
    MnuIndex = MNU_MAIN;
    msg = KBD_NOP;
    return;
  }
  if((msg == KBD_SETV) || (msg == KBD_SETI))
  {
    EditEscape();
    Sound->High();
    MnuIndex = MNU_MAIN;
    msg = KBD_NOP;
    return;
  }
  if(msg == KBD_OUT)
  {
    EditExit();
    Sound->High();
    MnuIndex = MNU_MAIN;
    msg = KBD_NOP;
    return;
  }
}

//-------------------------- Событие энкодера: -------------------------------

void TMenuPreset::OnEncoder(int8_t &step)
{
  if(step > 0 && ParIndex < PRESETS)
  {
    ParIndex++;
    if(ParIndex < PRESETS)
    {
      EditEscape();
      EditEnter();
    }
    else
    {
      LoadParam(Data->SetupData->Items[PAR_ESC]);
      Display->Blink(BLINK_NO);
    }
    step = ENC_NOP;
  }
  if(step < 0 && ParIndex > 0)
  {
    ParIndex--;
    EditEscape();
    EditEnter();
    step = ENC_NOP;
  }
}

//------------------------- Вход в редактирование: ---------------------------

void TMenuPreset::EditEnter(void)
{
  BackupV = Params->Items[PAR_V]->Value;
  BackupI = Params->Items[PAR_I]->Value;
  Data->ReadPreset(ParIndex);
  Display->Blink(BLINK_VI);
  Params->Items[PAR_V]->ShowValue();
  Params->Items[PAR_I]->ShowValue();
}

//------------------------ Выход из редактирования: --------------------------

void TMenuPreset::EditExit(void)
{
  if(Edit)
  {
    Params->Items[PAR_V]->Value = BackupV;
    Params->Items[PAR_I]->Value = BackupI;
    Data->SavePreset(ParIndex);
  }
  else
  {
    Data->OutOn = 0; //выключение выхода при чтении пресета
  }
}

//----------------- Выход из редактирования без сохранения: ------------------

void TMenuPreset::EditEscape(void)
{
  Params->Items[PAR_V]->Value = BackupV;
  Params->Items[PAR_I]->Value = BackupI;
}

//----------------------------------------------------------------------------
//--------------------------- Класс TMenuSetup: ------------------------------
//----------------------------------------------------------------------------

//-------------------------- Инициализация меню: -----------------------------

void TMenuSetup::Init(void)
{
  Edit = 0;
  Force = 1;
  if(ParIndex == PAR_ESC)
    ParIndex = ActiveIndex;
  LoadParam(Params->Items[ParIndex]);
  if((ParIndex == PAR_LOCK) && (Par->Value == ON))
    Timeout = TIMEOUT_LOCK;
      else Timeout = TIMEOUT_SETUP;
}

//-------------------- Обновление показаний температуры: ---------------------

void TMenuSetup::Execute(void)
{
  if(ParIndex == PAR_HST)
  {
    if(Analog->TempUpdate || Force)
    {
      Par->Value = Analog->GetTemp();
      Par->ShowValue();
      Force = 0;
    }
    return;
  }
  if(ParIndex == PAR_TIM && !Edit)
  {
    if(TSysTimer::SecTick || Force)
    {
      Par->Value = Analog->OffTime;
      Par->ShowValue();
      Force = 0;
    }
    return;
  }
}

//-------------------------- Событие клавиатуры: -----------------------------

void TMenuSetup::OnKeyboard(KeyMsg_t &msg)
{
  //если LOCK:
  if((Data->SetupData->Items[PAR_LOCK]->Value == ON) && !Edit)
  {
    if(msg == KBD_ENC)
    {
      msg = KBD_NOP;
      return;
    }
    if((msg == KBD_SETV) ||
       (msg == KBD_SETI) ||
       (msg == KBD_FINE))
    {
      Sound->High();
      MnuIndex = MNU_MAIN;
      msg = KBD_NOP;
      return;
    }
    if(msg == KBD_ENCH)
    {
      Sound->Beep();
      Timeout = TIMEOUT_SETUP;
      EditEnter();
      msg = KBD_NOP;
      return;
    }
  }
  //нажатие энкодера:
  if(msg == KBD_ENC)
  {
    //enter call preset menu:
    if(ParIndex == PAR_CALL)
    {
      ParIndex = 0;
      MnuIndex = MNU_PRESET;
      msg = KBD_NOP;
      return;
    }
    //enter store preset menu:
    if(ParIndex == PAR_STOR)
    {
      ParIndex = PRESETS;
      MnuIndex = MNU_PRESET;
      msg = KBD_NOP;
      return;
    }
    //load defaults:
    if(ParIndex == PAR_DEF && Par->Value == YES)
    {
      Display->Off();
      Data->Apply(ParIndex);
      Display->On();
      Sound->High();
      MnuIndex = MNU_MAIN;
      msg = KBD_NOP;
      return;
    }
    //enter calibration menu:
    if(ParIndex == PAR_CAL && Par->Value == YES)
    {
      ParIndex = 0;
      MnuIndex = MNU_CALIB;
      msg = KBD_NOP;
      return;
    }
    //escape menu:
    if(ParIndex == PAR_ESC)
    {
      Sound->High();
      MnuIndex = MNU_MAIN;
      msg = KBD_NOP;
      return;
    }
    //выход, если редактирование запрещено:
    if(Par->Min == Par->Max) return;
    //вход в редактирование:
    if(!Edit)
    {
      EditEnter();
      Display->Blink(BLINK_NC);
    }
    //выход из редактирования:
    else
    {
      EditExit();
      Data->Apply(ParIndex);
      //если LOCK, то выход в главное меню:
      if((ParIndex == PAR_LOCK) && (Par->Value == ON))
        MnuIndex = MNU_MAIN;
    }
    msg = KBD_NOP;
    return;
  }
  if((msg == KBD_SETV) || (msg == KBD_SETI) || (msg == KBD_ENCH))
  {
    EditExit();
    Data->Apply(ParIndex);
    Sound->High();
    MnuIndex = MNU_MAIN;
    msg = KBD_NOP;
    return;
  }
  if(msg == KBD_OUT)
  {
    EditExit();
    Data->Apply(ParIndex);
    if(Analog->GetProtSt() != PR_OK) msg = KBD_NOP; //сброс кода кнопки
    return; //код кнопки не сбрасывается
  }
}

//-------------------------- Событие энкодера: -------------------------------

void TMenuSetup::OnEncoder(int8_t &step)
{
  //управление заблокировано:
  if((Data->SetupData->Items[PAR_LOCK]->Value == ON) &&
     !(Edit && (ParIndex == PAR_LOCK)))
  {
    step = ENC_NOP; //нет звука выключенного энкодера
    return;
  }
  //редактирование параметра:
  if(Edit)
  {
    if(Par->Edit(step))
    {
      Par->ShowValue();
      step = ENC_NOP;
    }
  }
  //переход к другому параметру:
  else
  {
    if(step > 0 && ParIndex < Params->ItemsCount - 1)
    {
      LoadParam(Params->Items[++ParIndex]);
      ActiveIndex = ParIndex;
      step = ENC_NOP;
    }
    if(step < 0 && ParIndex > 0)
    {
      LoadParam(Params->Items[--ParIndex]);
      ActiveIndex = ParIndex;
      step = ENC_NOP;
    }
  }
  if(ParIndex == PAR_HST)
  {
    Force = 1;
  }
}

//---------------------------- Событие таймера: ------------------------------

void TMenuSetup::OnTimer(void)
{
  if(ParIndex == PAR_TIM)
  {
    if(Edit)
    {
      EditEscape();
      Par->ShowValue();
    }
    return;
  }
  if(ParIndex != PAR_HST)
  {
    EditEscape();
    MnuIndex = MNU_MAIN;
    return;
  }
}

//----------------------------------------------------------------------------
//---------------------------- Класс TMenuProt: ------------------------------
//----------------------------------------------------------------------------

//-------------------------- Инициализация меню: -----------------------------

void TMenuProt::Init(void)
{
  //возможны следующие значения ParIndex:
  //PAR_OVP, PAR_OCP, PAR_OPP, PAR_OTP.
  //При срабатывании защиты к ParIndex добавляется PROT_FLAG.
  Prot = ParIndex & PROT_FLAG;
  ParIndex &= ~PROT_FLAG;
  Temp = Data->SetupData->Items[PAR_HST];
  Force = 1;
  LoadParam(Params->Items[ParIndex]);
  if(!Prot)
  {
    EditEnter();
  }
  else
  {
    Edit = 0;
    Display->Blink((ParIndex == PAR_OTP)? BLINK_V : BLINK_VI);
  }
  Timeout = TIMEOUT_SETUP;
}

//---------------------------- Обновление меню: ------------------------------

void TMenuProt::Execute(void)
{
  if(!Edit)
  {
    char ProtSt = Analog->GetProtSt();
    //автоматический выход (например, при сбросе защиты с PC):
    if((ParIndex == PAR_OVP && !(ProtSt & PR_OVP)) ||
       (ParIndex == PAR_OCP && !(ProtSt & PR_OCP)) ||
       (ParIndex == PAR_OPP && !(ProtSt & PR_OPP)) ||
       (ParIndex == PAR_OTP && !(ProtSt & PR_OTP)))
    {
      Sound->High();
      MnuIndex = MNU_MAIN;
      return;
    }
    //обновление показаний температуры:
    if(ParIndex == PAR_OTP && (Analog->TempUpdate || Force))
    {
      Temp->Value = Analog->GetTemp();
      Temp->ShowValue();
      Force = 0;
    }
  }
}

//-------------------------- Событие клавиатуры: -----------------------------

void TMenuProt::OnKeyboard(KeyMsg_t &msg)
{
  if(msg == KBD_ENC)
  {
    if(!Edit)
    {
      EditEnter();
    }
    else
    {
      EditExit();
      Analog->ClrProtSt();
      MnuIndex = MNU_MAIN; //выход
    }
    msg = KBD_NOP;
    return;
  }
  if(msg == KBD_SETV)
  {
    EditExit();
    if(ParIndex != PAR_OVP && ParIndex != PAR_OTP)
    {
      ParIndex = PAR_OVP;
      LoadParam(Params->Items[ParIndex]);
      EditEnter();
    }
    else
    {
      Analog->ClrProtSt();
      MnuIndex = MNU_MAIN; //выход
    }
    msg = KBD_NOP;
    return;
  }
  if(msg == KBD_SETI)
  {
    EditExit();
    if(ParIndex != PAR_OCP && ParIndex != PAR_OTP)
    {
      ParIndex = PAR_OCP;
      LoadParam(Params->Items[ParIndex]);
      EditEnter();
    }
    else
    {
      Analog->ClrProtSt();
      MnuIndex = MNU_MAIN; //выход
    }
    msg = KBD_NOP;
    return;
  }
  if(msg == KBD_SETVI)
  {
    EditExit();
    if(ParIndex != PAR_OPP && ParIndex != PAR_OTP)
    {
      ParIndex = PAR_OPP;
      LoadParam(Params->Items[ParIndex]);
      EditEnter();
    }
    else
    {
      Analog->ClrProtSt();
      MnuIndex = MNU_MAIN; //выход
    }
    msg = KBD_NOP;
    return;
  }
  if(msg == KBD_OUT)
  {
    EditExit();
    Analog->ClrProtSt();
    MnuIndex = MNU_MAIN; //выход
    msg = KBD_NOP; //сброс кода кнопки
    return;
  }
}

//-------------------------- Событие энкодера: -------------------------------

void TMenuProt::OnEncoder(int8_t &step)
{
  if(Edit)
  {
    if(Par->Edit(step))
    {
      Par->ShowValue();
      step = ENC_NOP;
    }
  }
}

//---------------------------- Событие таймера: ------------------------------

void TMenuProt::OnTimer(void)
{
  if(Edit)
  {
    EditEscape();
    if(Prot)
    {
      if(ParIndex == PAR_OTP)
      {
        Temp->Value = Analog->GetTemp();
        Temp->ShowValue();
        Display->Blink(BLINK_V);
      }
      else
      {
        Par->ShowValue();
        Display->Blink(BLINK_VI);
      }
    }
    else
    {
      MnuIndex = MNU_MAIN;
    }
  }
}

//----------------------------------------------------------------------------
//---------------------------- Класс TMenuTop: -------------------------------
//----------------------------------------------------------------------------

//-------------------------- Инициализация меню: -----------------------------

void TMenuTop::Init(void)
{
  Edit = 0;
  LoadParam(Params->Items[ParIndex]);
  Timeout = TIMEOUT_SETUP;
}

//-------------------------- Событие клавиатуры: -----------------------------

void TMenuTop::OnKeyboard(KeyMsg_t &msg)
{
  //для входа в меню MNU_TOP кнопку надо отпустить
  //при появлении индикации меню,
  //удержание любой кнопки - выход в MNU_MAIN:
  if(msg & KBD_HOLD)
  {
    EditEscape();
    MnuIndex = MNU_MAIN;
    msg = KBD_NOP;
    return;
  }
  if(msg == KBD_SETV)
  {
    if(!Edit)
    {
      EditEnter();
    }
    else
    {
      EditExit();
      Data->TrimParamsLimits();
      if(ParIndex != PAR_MAXV)
      {
        ParIndex = PAR_MAXV;
        LoadParam(Params->Items[ParIndex]);
        EditEnter();
      }
      else
      {
        MnuIndex = MNU_MAIN;
      }
    }
    msg = KBD_NOP;
    return;
  }
  if(msg == KBD_SETI)
  {
    if(!Edit)
    {
      EditEnter();
    }
    else
    {
      EditExit();
      Data->TrimParamsLimits();
      if(ParIndex != PAR_MAXI)
      {
        ParIndex = PAR_MAXI;
        LoadParam(Params->Items[ParIndex]);
        EditEnter();
      }
      else
      {
        MnuIndex = MNU_MAIN;
      }
    }
    msg = KBD_NOP;
    return;
  }
  if(msg == KBD_SETVI)
  {
    if(!Edit)
    {
      EditEnter();
    }
    else
    {
      EditExit();
      Data->TrimParamsLimits();
      if(ParIndex != PAR_MAXP)
      {
        ParIndex = PAR_MAXP;
        LoadParam(Params->Items[ParIndex]);
        EditEnter();
      }
      else
      {
        MnuIndex = MNU_MAIN;
      }
    }
    msg = KBD_NOP;
    return;
  }
  if(msg == KBD_ENC)
  {
    EditExit();
    Data->TrimParamsLimits();
    if(ParIndex < Params->ItemsCount - 1)
    {
      LoadParam(Params->Items[++ParIndex]);
      EditEnter();
    }
    else
    {
      MnuIndex = MNU_MAIN;
    }
    msg = KBD_NOP;
    return;
  }
  if(msg == KBD_OUT)
  {
    EditExit();
    MnuIndex = MNU_MAIN;
    msg = KBD_NOP;
    return;
  }
}

//-------------------------- Событие энкодера: -------------------------------

void TMenuTop::OnEncoder(int8_t &step)
{
  if(Edit)
  {
    if(Par->Edit(step))
    {
      Par->ShowValue();
      step = ENC_NOP;
    }
  }
}

//----------------------------------------------------------------------------
//--------------------------- Класс TMenuCalib: ------------------------------
//----------------------------------------------------------------------------

//-------------------------- Инициализация меню: -----------------------------

void TMenuCalib::Init(void)
{
  Edit = 1;
  Timeout = 0;                        //нет выхода по таймеру
  UpdateCV = 0;
  UpdateCI = 0;
  Analog->OutControl(0);              //выключение выхода
  Analog->TrimParamsLimits();         //коррекция значений согласно Top
  LoadParam(Params->Items[ParIndex]);
  Display->Blink(BLINK_V);
}

//-------------------------- Событие энкодера: -------------------------------

void TMenuCalib::OnEncoder(int8_t &step)
{
  //редактирование параметра:
  if(Edit)
  {
    if(Par->Edit(step))
    {
      Par->ShowValue();
      step = ENC_NOP;
    }
  }
  //коррекция кода при редактировании точек калибровки:
  if(ParIndex == CAL_VP1)
    Params->Items[CAL_VC1]->Value = Analog->DacV->ValueToCode(Par->Value);
  if(ParIndex == CAL_VP2)
    Params->Items[CAL_VC2]->Value = Analog->DacV->ValueToCode(Par->Value);
  if(ParIndex == CAL_IP1)
    Params->Items[CAL_IC1]->Value = Analog->DacI->ValueToCode(Par->Value);
  if(ParIndex == CAL_IP2)
    Params->Items[CAL_IC2]->Value = Analog->DacI->ValueToCode(Par->Value);
  //загрузка DAC:
  if(Par->Type == PT_VC)
    { Analog->DacV->SetCode(Par->Value); UpdateCV = 1; }
  if(Par->Type == PT_IC)
    { Analog->DacI->SetCode(Par->Value); UpdateCI = 1; }
}

//-------------------------- Событие клавиатуры: -----------------------------

void TMenuCalib::OnKeyboard(KeyMsg_t &msg)
{
  if(msg == KBD_SETV)
  {
    if(ParIndex > 0)
    {
      Apply(ParIndex);
      ParIndex--;       //ParIndex = CAL_VP1;
      LoadParam(Params->Items[ParIndex]);
      if(ParIndex == CAL_IC2)
        Params->Items[CAL_IP2]->ShowValue();
      msg = KBD_NOP;
    }
  }
  if(msg == KBD_SETI)
  {
    if(ParIndex < CAL_STR)
    {
      Apply(ParIndex);
      ParIndex++;       //ParIndex = CAL_IP1;
      LoadParam(Params->Items[ParIndex]);
      msg = KBD_NOP;
    }
  }
  if(msg == KBD_ENC)
  {
    if(ParIndex < CAL_STR) //параметры измерителя не участвуют
    {
      Apply(ParIndex);
      ParIndex++;
      LoadParam(Params->Items[ParIndex]);
      msg = KBD_NOP;
    }
    else //запрос сохранения
    {
      if(Par->Value == YES)
      {
        Display->Off();
        Analog->CalibData->SaveToEeprom();
        Display->On();
      }
      else if(Par->Value == DEFAULT)
      {
        Display->Off();
        Analog->CalibData->LoadDefaults();
        Analog->CalibData->SaveToEeprom();
        Display->On();
        Analog->CalibAll();
      }
      else
      {
        Analog->CalibData->ReadFromEeprom();
        Analog->CalibAll();
      }
      Sound->High();
      MnuIndex = MNU_MAIN;
      msg = KBD_NOP;
    }
  }
  //пост-обработка обработанных кнопок:
  if(msg == KBD_NOP)
  {
    //включение мигания редактируемой величины:
    if((Par->Type == PT_V) ||
       (Par->Type == PT_IC))
      Display->Blink(BLINK_V);

    if((Par->Type == PT_I) ||
       (Par->Type == PT_VC) ||
       (Par->Type == PT_NYDEF))
      Display->Blink(BLINK_I);

    //загрузка DAC и включение выхода:
    if(Par->Type == PT_VC)
    {
      Analog->ClrProtSt();
      Analog->DacV->SetCode(Par->Value);
      Analog->DacI->SetCode(DAC_CAL_CODE);
      Analog->OutControl(1); //включение выхода
    }
    else if (Par->Type == PT_IC)
    {
      Analog->ClrProtSt();
      Analog->DacV->SetCode(DAC_CAL_CODE);
      Analog->DacI->SetCode(Par->Value);
      Analog->OutControl(1); //включение выхода
    }
    else
    {
      Analog->OutControl(0); //выключение выхода
    }
    return;
  }
  //удержание кнопки энкодера - выход без сохранения:
  if(msg == KBD_ENCH)
  {
    Analog->OutControl(0); //выключение выхода
    Analog->CalibData->ReadFromEeprom();
    Analog->CalibAll();
    Sound->High();
    MnuIndex = MNU_MAIN;
    msg = KBD_NOP;
    return;
  }
  if(msg == KBD_OUT)
  {
    if((Par->Type != PT_IC) && (Par->Type != PT_VC))
      msg = KBD_ERROR; //запрет операции
    return;
  }
}

//------------------------- Применение калибровки: ---------------------------

//Измеритель калибруется только в том случае,
//если значения C1 или C2 менялись.
//Проверяются режимы CC/CV и OUT ON/OFF.
//Если режим неправильный, восстанавливается
//старое значение точки и коэффициента.

void TMenuCalib::Apply(char p)
{
  //V -> C1:
  if(p == CAL_VC1)
  {
    if(Analog->OutState() && Analog->IsCV())
    {
      //применение нового коэффициента:
      Analog->CalibDacV();
      if(UpdateCV) Analog->CalibAdcV(CAL_VM1);
    }
    else
    {
      //восстановление старых значений:
      Analog->CalibData->ReadFromEeprom(CAL_VP1);
      Analog->CalibData->ReadFromEeprom(CAL_VC1);
      Analog->CalibDacV();
      Sound->Bell(); //звук ошибки
    }
  }
  //V -> C2:
  if(p == CAL_VC2)
  {
    if(Analog->OutState() && Analog->IsCV())
    {
      //применение нового коэффициента:
      Analog->CalibDacV();
      if(UpdateCV) Analog->CalibAdcV(CAL_VM2);
    }
    else
    {
      //восстановление старых значений:
      Analog->CalibData->ReadFromEeprom(CAL_VP2);
      Analog->CalibData->ReadFromEeprom(CAL_VC2);
      Analog->CalibDacV();
      Sound->Bell(); //звук ошибки
    }
  }
  //I -> C1:
  if(p == CAL_IC1)
  {
    if(Analog->OutState() && Analog->IsCC())
    {
      //применение нового коэффициента:
      Analog->CalibDacI();
      if(UpdateCI) Analog->CalibAdcI(CAL_IM1);
    }
    else
    {
      //восстановление старых значений:
      Analog->CalibData->ReadFromEeprom(CAL_IP1);
      Analog->CalibData->ReadFromEeprom(CAL_IC1);
      Analog->CalibDacV();
      Sound->Bell(); //звук ошибки
    }
  }
  //I -> C2:
  if(p == CAL_IC2)
  {
    if(Analog->OutState() && Analog->IsCC())
    {
      //применение нового коэффициента:
      Analog->CalibDacI();
      if(UpdateCI) Analog->CalibAdcI(CAL_IM2);
    }
    else
    {
      //восстановление старых значений:
      Analog->CalibData->ReadFromEeprom(CAL_IP2);
      Analog->CalibData->ReadFromEeprom(CAL_IC2);
      Analog->CalibDacV();
      Sound->Bell(); //звук ошибки
    }
  }
}

//----------------------------------------------------------------------------
//---------------------------- Класс TMenuItems: -----------------------------
//----------------------------------------------------------------------------

//----------------------------- Конструктор: ---------------------------------

TMenuItems::TMenuItems(char max) : TList(max)
{
  AddItem(new TMenuSplash(NULL));             //MNU_SPLASH
  AddItem(new TMenuError(NULL));              //MNU_ERROR
  AddItem(new TMenuMain(Data->MainData));     //MNU_MAIN
  AddItem(new TMenuSetup(Data->SetupData));   //MNU_SETUP
  AddItem(new TMenuPreset(Data->MainData));   //MNU_PRESET
  AddItem(new TMenuProt(Data->SetupData));    //MNU_PROT
  AddItem(new TMenuTop(Data->TopData));       //MNU_TOP
  AddItem(new TMenuCalib(Analog->CalibData)); //MNU_CALIB
}

//---------------------- Переход в меню по индексу: --------------------------

void TMenuItems::SelectMenu(Menu_t mnu, char par)
{
  SelectedMenu = Items[mnu];
  SelectedMenu->MnuIndex = mnu;
  SelectedMenu->ParIndex = par;
  SelectedMenu->Init();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
