//----------------------------------------------------------------------------

//Модуль поддержки порта, заголовочный файл

//----------------------------------------------------------------------------

#ifndef PORT_H
#define PORT_H

#include "wakeport.h"
#include "data.h"

//----------------------------- Константы: -----------------------------------

#define BAUD_RATE       19200  //скорость обмена, бод
#define FRAME_SIZE         16  //максимальный размер фрейма, байт

#define PAR_COUNT          23  //количество параметров
#define PAR_NON           255  //индекс для отсутствующих параметров

//Индексы параметров для команд CMD_SET_PAR и CMD_GET_PAR:
const uint8_t ParIdx[PAR_COUNT] =
{
  PAR_LOCK, //Lock controls
  PAR_OVP,  //OVP threshold
  PAR_OCP,  //OCP threshold
  PAR_OPP,  //OPP threshold
  PAR_DEL,  //OVP/OCP delay
  PAR_OTP,  //OTP threshold
  PAR_FNL,  //Fan start temperature
  PAR_FNH,  //Fan full speed temperature
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
  PAR_TIM   //Timer interval
};

//----------------------------------------------------------------------------
//------------------------------ Класс TPort ---------------------------------
//----------------------------------------------------------------------------

class TPort
{
private:
public:
  TWakePort *WakePort;
  TPort(void);
  void Execute(void);
};

//----------------------------------------------------------------------------
//----------------------------- Коды команд: ---------------------------------
//----------------------------------------------------------------------------

#define CMD_SET_VI 6 //установка напряжения и тока

  //TX: word V, word I, byte S
  //RX: byte Err

  //V = 0..VMAX - напряжение, x0.01 В
  //I = 0..IMAX - ток, x0.001 А
  //S = 0 - выход выключен, 1 - выход включен
  //Err = ERR_NO

#define CMD_GET_VI 7 //чтение установленного напряжения и тока

  //TX:
  //RX: byte Err, word V, word I

  //V = 0..VMAX - напряжение, x0.01 В
  //I = 0..IMAX - ток, x0.001 А
  //Err = ERR_NO

#define CMD_GET_STAT 8 //чтение статуса источника

  //TX:
  //RX: byte Err, byte S

  //S.0 = 1 - выход включен
  //S.1 = 1 - CV
  //S.2 = 1 - CC
  //S.3 = 1 - OVP
  //S.4 = 1 - OCP
  //S.5 = 1 - OPP
  //S.6 = 1 - OTP
  //Err = ERR_NO

#define CMD_GET_VI_AVG 9 //чтение среднего измеренного напряжения и тока

  //TX:
  //RX: byte Err, word VA, word IA

  //VA = 0..VMAX - напряжение, x0.01 В
  //IA = 0..IMAX - ток, x0.001 А
  //Err = ERR_NO

#define CMD_GET_VI_FAST 10 //чтение мгновенного измеренного напряжения и тока

  //TX:
  //RX: byte Err, word VF, word IF

  //VF = 0..VMAX - напряжение, x0.01 В
  //IF = 0..IMAX - ток, x0.001 А
  //Err = ERR_NO

#define CMD_SET_VIP_MAX 11 //установка макс. напряжения, тока и мощности

  //TX: word VM, word IM, word PM
  //RX: byte Err

  //VM = 1000..9999 - напряжение, x0.01 В
  //IM = 1000..9999 - ток, x0.001 А
  //PM = 10..9999 - мощность, x0.1 Вт
  //Err = ERR_NO

#define CMD_GET_VIP_MAX 12 //чтение макс. напряжения, тока и мощности

  //TX:
  //RX: byte Err, word VM, word IM, word PM

  //VM = 1000..9999 - напряжение, x0.01 В
  //IM = 1000..9999 - ток, x0.001 А
  //PM = 10..9999 - мощность, x0.1 Вт
  //Err = ERR_NO

#define CMD_SET_PRE 13 //запись пресета

  //TX: byte N, word V, word I
  //RX: byte Err

  //N = 0..9 - номер пресета
  //V = 0..VMAX - напряжение, x0.01 В
  //I = 0..IMAX - ток, x0.001 А
  //Err = ERR_NO, ERR_PA

#define CMD_GET_PRE 14 //чтение пресета

  //TX: byte N
  //RX: byte Err, word V, word I

  //N = 0..9 - номер пресета
  //V = 0..VMAX - напряжение, x0.01 В
  //I = 0..IMAX - ток, x0.001 А
  //Err = ERR_NO, ERR_PA

#define CMD_SET_PAR 15 //установка параметра

  //TX: byte N, word P
  //RX: byte Err

  //N - номер параметра (см. таблицу параметров)
  //P - значение параметра (см. таблицу параметров)
  //Err = ERR_NO, ERR_PA

#define CMD_GET_PAR 16 //чтение параметра

  //TX: byte N
  //RX: byte Err, word P

  //N - номер параметра (см. таблицу параметров)
  //P - значение параметра (см. таблицу параметров)
  //Err = ERR_NO, ERR_PA

#define CMD_GET_FAN 17 //чтение скорости вентилятора и температуры

  //TX:
  //RX: byte Err, byte S, word T

  //S = 0..100 - скорость вентилятора, %
  //T = 0..999 - температура, x0.1°C
  //Err = ERR_NO

#define CMD_SET_DAC 18 //установка кода ЦАП

  //TX: word DACV, word DACI
  //RX: byte Err

  //DACV = 0..65520 - код ЦАП напряжения
  //DACI = 0..65520 - код ЦАП тока
  //Err = ERR_NO

#define CMD_GET_ADC 19 //чтение кода АЦП

  //TX:
  //RX: byte Err, word ADCV, word ADCI

  //ADCV = 0..65520 - код АЦП напряжения
  //ADCI = 0..65520 - код АЦП тока
  //Err = ERR_NO

#define CMD_SET_CAL 20 //установка калибровочного коэффициента

  //TX: byte N, word K
  //RX: byte Err

  //N - номер коэффициента (см. таблицу коэффициентов)
  //K - значение коэффициента (см. таблицу коэффициентов)
  //Err = ERR_NO, ERR_PA

#define CMD_GET_CAL 21 //чтение калибровочного коэффициента

  //TX: byte N
  //RX: byte Err, word K

  //N - номер коэффициента (см. таблицу коэффициентов)
  //K - значение коэффициента (см. таблицу коэффициентов)
  //Err = ERR_NO, ERR_PA

//----------------------------------------------------------------------------

#endif
