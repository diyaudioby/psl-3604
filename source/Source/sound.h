//----------------------------------------------------------------------------

//Модуль генерации звуковых сигналов, заголовочный файл

//----------------------------------------------------------------------------

#ifndef SOUND_H
#define SOUND_H

//----------------------------------------------------------------------------

#include "systimer.h"

//------------------------------- Константы: ---------------------------------

enum SndMode_t //режим генерации звука
{
  SND_OFF,
  SND_ALARM,
  SND_ON
};

//----------------------------------------------------------------------------
//----------------------------- Класс TSound: --------------------------------
//----------------------------------------------------------------------------

class TSound
{
private:
  TGpio<PORTB, PIN13> Pin_SND; 
  TSoftTimer *SoundTimer;
  SndMode_t SoundMode;
public:
  TSound(void);
  void Execute(void);
  void Off(void);
  void SetMode(SndMode_t m);
  void PlayNormal(uint16_t f, uint16_t d);
  void PlayAlarm(uint16_t f, uint16_t d);
  //Normal group:
  void Beep(void);
  void Tick(void);
  void High(void);
  void Click(void);
  void Bell(void);
  //Alarm group:
  void Alarm(void);
  void Alert(void);
  void ABell(void);
};

//----------------------------------------------------------------------------

extern TSound *Sound;

//----------------------------------------------------------------------------

#endif
