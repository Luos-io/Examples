
#include <Arduino.h>

#ifdef __cplusplus
 extern "C" {
#endif

#include <cmd.h>
#include <gate.h>
#include <luos.h>

char *ReceiveData;
uint16_t ReceiveDataCounter;
void json_receive(void);

#ifdef __cplusplus
}
#endif

void setup() {
  Serial.begin(1000000);
  while (!Serial) {
  ; // wait for serial port to connect. Needed for native USB port only
  }
  Luos_Init();
  Gate_Init();
  ReceiveData = get_json_buf();
}

void loop() {
  Luos_Loop();
  json_receive();
  Gate_Loop();
}

void json_send(char *json)
{
  Serial.write(json);
}

void json_receive(void)
{
  while (Serial.available() > 0) 
  {
    *ReceiveData = Serial.read();
    ReceiveDataCounter++;
    if(*ReceiveData == '\r')
    {
      check_json(ReceiveDataCounter-1);
      ReceiveData = get_json_buf();
      ReceiveDataCounter = 0;
      break;
    }
    else
    {
      ReceiveData++;
    }
  }
}
