//----------------------------------------------------------------------------

//Модуль релизации протокола Wake, заголовочный файл

//----------------------------------------------------------------------------

#ifndef WAKE_H
#define WAKE_H

//----------------------------- Константы: -----------------------------------

//Коды ошибок:

#define ERR_NO      0 //no error
#define ERR_TX      1 //Rx/Tx error
#define ERR_BU      2 //device busy error
#define ERR_RE      3 //device not ready error
#define ERR_PA      4 //parameters value error
#define ERR_NR      5 //no replay
#define ERR_NC      6 //no carrier

//Коды стандартных команд:

#define CMD_NOP     0 //нет операции
#define CMD_ERR     1 //ошибка приема пакета
#define CMD_ECHO    2 //эхо
#define CMD_INFO    3 //чтение информации об устройстве
#define CMD_SETADDR 4 //установка адреса
#define CMD_GETADDR 5 //чтение адреса

//------------------------ Класс протокола WAKE: -----------------------------

class TWake
{
private:
  enum WPnt_t
  {
    PTR_ADD,      //смещение в буфере для адреса
    PTR_CMD,      //смещение в буфере для кода команды
    PTR_LNG,      //смещение в буфере для длины пакета
    PTR_DAT       //смещение в буфере для данных
  };
  enum WStuff_t
  {
    FEND  = 0xC0, //Frame END
    FESC  = 0xDB, //Frame ESCape
    TFEND = 0xDC, //Transposed Frame END
    TFESC = 0xDD  //Transposed Frame ESCape
  };
  enum WState_t
  {
    WST_IDLE,     //состояние ожидания
    WST_ADD,      //прием адреса
    WST_CMD,      //прием команды
    WST_LNG,      //прием длины пакета
    WST_DATA,     //прием/передача данных
    WST_CRC,      //прием/передача CRC
    WST_DONE      //состояние готовности
  };
  
  char Addr;      //адрес устройства
  char RxState;   //состояние процесса приема
  bool RxStuff;   //признак стаффинга при приеме
  char *RxPtr;    //указатель буфера приема
  char *RxEnd;    //значение указателя конца буфера приема
  char RxCount;   //количество принятых байт
  char *RxData;   //буфер приема

  char TxState;   //состояние процесса передачи
  bool TxStuff;   //признак стаффинга при передаче
  char *TxPtr;    //указатель буфера передачи
  char *TxEnd;    //значение указателя конца буфера передачи
  char TxCount;   //количество передаваемых байт
  char *TxData;   //буфер передачи
  
  char Frame;
  void Do_Crc8(char b, char *crc); //вычисление контрольной суммы
protected:
  void Rx(char data);  //прием байта
  bool Tx(char &data); //передача байта
public:
  TWake(char frame);
  char GetCmd(void);      //возвращает текущий код команды
  char GetRxCount(void);  //возвращает количество принятых байт
  void SetRxPtr(char p);  //устанавливает указатель буфера приема
  char GetRxPtr(void);    //читает указатель буфера приема
  char GetByte(void);     //читает байт из буфера приема
  int16_t GetWord(void);  //читает слово из буфера приема
  int32_t GetDWord(void); //читает двойное слово из буфера приема
  void GetData(char *d, char count); //читает данные из буфера приема

  void SetTxPtr(char p);  //устанавливает указатель буфера передачи
  char GetTxPtr(void);    //читает указатель буфера передачи
  void AddByte(char b);   //помещает байт в буфер передачи
  void AddWord(int16_t w);   //помещает слово в буфер передачи
  void AddDWord(int32_t dw); //помещает двойное слово в буфер передачи
  void AddData(char *d, char count); //помещает данные в буфер передачи
  void TxStart(char cmd, char &data); //начало передачи пакета
  bool AskTxEnd(void);    //определение конца передачи пакета
};

//----------------------------------------------------------------------------

#endif
