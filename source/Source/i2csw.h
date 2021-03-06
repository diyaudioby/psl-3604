//----------------------------------------------------------------------------

//������ ����������� ���������� ���� I2C, ������������ ����

//----------------------------------------------------------------------------

#ifndef I2CSW_H
#define I2CSW_H

//----------------------------- ���������: -----------------------------------

#define I2C_RD       1 //������� ������ ������ �� ���� I2C
#define I2C_ACK      1 //������� �������� ACK
#define I2C_NACK     0 //������� �������� NACK

//----------------------------------------------------------------------------
//----------------------------- ����� TI2Csw: --------------------------------
//----------------------------------------------------------------------------

class TI2Csw
{
private:
  static TGpio<PORTB, PIN8> Pin_SCL; 
  static TGpio<PORTB, PIN9> Pin_SDA;
  static void BitDelay(void);
public:
  static void Init(void);
  static void Free(void);
  static void Start(void);
  static bool Write(char data);
  static char Read(bool ack);
  static void Stop(void);
};

//----------------------------------------------------------------------------

#endif
