//----------------------------------------------------------------------------

//ћодуль общего управлени€

//----------------------------------------------------------------------------

#include "main.h"
#include "control.h"
#include "sound.h"
#include "data.h"
#include "analog.h"

//----------------------------- ѕеременные: ----------------------------------

TDisplay *Display;
TSound *Sound;
TEncoder *Encoder;
TData *Data;
TAnalog *Analog;

//----------------------------------------------------------------------------
//----------------------------  ласс TControl: -------------------------------
//----------------------------------------------------------------------------

//-----------------------------  онструктор: ---------------------------------

TControl::TControl(void)
{
  //начальна€ задержка
  //(щелчок выключател€ не должен совпадать
  //по времени с начальным beep):
  TSysTimer::Delay_ms(200);
  TEeprom::Init();
  Display = new TDisplay();
  Sound = new TSound();
  Encoder = new TEncoder();
  Keyboard = new TKeyboard();
  Analog = new TAnalog();
  Data = new TData();
  Menu = new TMenuItems(MENUS);
  MenuTimer = new TSoftTimer();
  MenuTimer->Oneshot = 1;
  //применение параметров:
  Data->ApplyAll();
  Display->LedFine = Data->MainData->Items[PAR_FINE]->Value;
  Sound->Beep(); //начальный beep

  if(TEeprom::Error)
  {
    //меню ошибки EEPROM:
    MnuIndex = MNU_ERROR;
    ParIndex = TEeprom::Error;
  }
  else
  {
    //основное меню:
    MnuIndex = MNU_MAIN; ParIndex = 0;
    //если разрешено, то меню SPLASH:
    if(Data->SetupData->Items[PAR_SPL]->Value)
    { MnuIndex = MNU_SPLASH; ParIndex = 0; }
    KeyMsg_t KeyMsg = Keyboard->Scan();
    if(KeyMsg == KBD_SETV)
    //удерживалась кнопка SET_V, установка MAXV:
    { MnuIndex = MNU_TOP; ParIndex = PAR_MAXV; }
    else if(KeyMsg == KBD_SETI)
    //удерживалась кнопка SET_I, установка MAXI:
    { MnuIndex = MNU_TOP; ParIndex = PAR_MAXI; }
    else if(KeyMsg == KBD_SETVI)
    //удерживалась кнопка SET_V + SET_I, установка MAXP:
    { MnuIndex = MNU_TOP; ParIndex = PAR_MAXP; }
  }
  //выбор нужного меню:
  Menu->SelectMenu(MnuIndex, ParIndex);
  //запуск таймера возврата из меню:
  MenuTimer->Start(Menu->SelectedMenu->Timeout);
}

//------------------------ ¬ыполнение управлени€: ----------------------------

void TControl::Execute(void)
{
  Display->Execute();
  Encoder->Execute();
  Keyboard->Execute();
  Sound->Execute();
  Analog->Execute();

  //переход в другое меню, если требуетс€
  if(Menu->SelectedMenu->MnuIndex != MnuIndex)
  {
    MnuIndex = Menu->SelectedMenu->MnuIndex;
    ParIndex = Menu->SelectedMenu->ParIndex;
    Menu->SelectMenu(MnuIndex, ParIndex);
    MenuTimer->Start(Menu->SelectedMenu->Timeout);
  }

  //обновление индикации в меню:
  Menu->SelectedMenu->Execute();

  //считывание энкодера и клавиатуры:
  KeyMsg_t KeyMsg = Keyboard->Message;
  int8_t Step = Encoder->Message;
  //учет режима FINE:
  if(!Data->MainData->Items[PAR_FINE]->Value)
    Step = Step * 10;

  //нажатие кнопки:
  if(KeyMsg != KBD_NOP)
  {
    if(!(KeyMsg & KBD_HOLD)) Sound->Beep();
    Menu->SelectedMenu->OnKeyboard(KeyMsg);
    MenuTimer->Start(Menu->SelectedMenu->Timeout);
  }

  //поворот энкодера:
  if(Step)
  {
    Menu->SelectedMenu->OnEncoder(Step);
    MenuTimer->Start(Menu->SelectedMenu->Timeout);
  }

  //переполнение таймера:
  if(MenuTimer->Over())
  {
    Menu->SelectedMenu->OnTimer();
  }

  //сервисы защиты и управлени€:
  ProtectionService(KeyMsg);
  FineSwitchService(KeyMsg);
  OutSwitchService(KeyMsg);

  //звук, если событи€ не обработаны:
  if((KeyMsg != KBD_NOP) && !(KeyMsg & KBD_HOLD))
    Sound->Bell();
  if(Step) Sound->Click();
  //сброс кодов клавиатуры и энкодера:
  Keyboard->Message = KBD_NOP;
  Encoder->Message = ENC_NOP;
}

//----------------------------------------------------------------------------
//------------------------- –еализаци€ сервисов: -----------------------------
//----------------------------------------------------------------------------

//---------------------------- —ервис защиты: --------------------------------

inline void TControl::ProtectionService(KeyMsg_t &KeyMsg)
{
  //из MNU_SETUP в меню защиты перехода нет:
  if(MnuIndex != MNU_SETUP && MnuIndex != MNU_CALIB)
  {
    char ProtSt = Analog->GetProtSt();
    //проверка срабатывани€ OTP:
    if(ProtSt & PR_OTP)
    {
      if(MnuIndex != MNU_PROT)
      {
        MnuIndex = MNU_PROT;
        ParIndex = PAR_OTP | PROT_FLAG;
        Menu->SelectMenu(MnuIndex, ParIndex);
        MenuTimer->Start(0);
      }
      if(KeyMsg == KBD_OUT)
        KeyMsg = KBD_ERROR;
    }
    //проверка срабатывани€ OCP:
    else if(ProtSt & PR_OCP)
    {
      if(MnuIndex != MNU_PROT)
      {
        MnuIndex = MNU_PROT;
        ParIndex = PAR_OCP | PROT_FLAG;
        Menu->SelectMenu(MnuIndex, ParIndex);
        MenuTimer->Start(0);
      }
    }
    //проверка срабатывани€ OVP:
    else if(ProtSt & PR_OVP)
    {
      if(MnuIndex != MNU_PROT)
      {
        MnuIndex = MNU_PROT;
        ParIndex = PAR_OVP | PROT_FLAG;
        Menu->SelectMenu(MnuIndex, ParIndex);
        MenuTimer->Start(0);
      }
    }
    //проверка срабатывани€ OPP:
    else if(ProtSt & PR_OPP)
    {
      if(MnuIndex != MNU_PROT)
      {
        MnuIndex = MNU_PROT;
        ParIndex = PAR_OPP | PROT_FLAG;
        Menu->SelectMenu(MnuIndex, ParIndex);
        MenuTimer->Start(0);
      }
    }
  }
}

//-------------------- —ервис управлени€ режимом FINE: -----------------------

inline void TControl::FineSwitchService(KeyMsg_t &KeyMsg)
{
  if(KeyMsg == KBD_FINE)
  {
    bool fine = Data->MainData->Items[PAR_FINE]->Value;
    fine = !fine;
    Display->LedFine = fine;
    Data->MainData->Items[PAR_FINE]->Value = fine;
    Data->MainData->SaveToEeprom(PAR_FINE);
    KeyMsg = KBD_NOP;
  }
}

//---------------------- —ервис управлени€ выходом: --------------------------

inline void TControl::OutSwitchService(KeyMsg_t &KeyMsg)
{
  if(KeyMsg == KBD_OUT)
  {
    Analog->OutControl(!Analog->OutState());
    if(Data->SetupData->Items[PAR_OUT]->Value == ON)
      Data->SaveV();
    KeyMsg = KBD_NOP;
  }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
