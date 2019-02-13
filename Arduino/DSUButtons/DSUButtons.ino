#include <SPI.h>
#include "mcp_can.h"

//IO Pins
const int fdistance = 3;
const int lanewarn = 4;

//CAN
const int SPI_CS_PIN = 10;
MCP_CAN CAN(SPI_CS_PIN);
unsigned long interval = 100; // the time we need to wait in between attempts to send messages
unsigned long previousTime = 0;
unsigned long currentTime = 0;
unsigned char JOEL_ID[3] = {0x02, 0x01, 0x00};
int canSendId = 0x203;

//Logic variables
int fdistanceState = HIGH;
int lanewarnState = HIGH;
int lane100mscount = 0;
int dist100mscount = 0;

void setup()
{
  pinMode(fdistance, INPUT_PULLUP);
  pinMode(lanewarn, INPUT_PULLUP);

  Serial.begin(115200);

  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV2);

  delay(9000);

  Serial.println("Initializing CAN Shield");
  while (CAN_OK != CAN.begin(CAN_500KBPS)) // init can bus : baudrate = 500                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            00k
  {
    Serial.println("CAN BUS Shield init fail");
    Serial.println("\tInit CAN BUS Shield again");
    delay(500);
  }
  Serial.println("CAN BUS Shield initialized");

  CAN.init_Mask(0, 0, 0xFFF); // there are 2 mask in mcp2515, you need to set both of them
  CAN.init_Mask(1, 0, 0xFFF);
  CAN.init_Filt(0, 0, 0xFFF);
  CAN.init_Filt(1, 0, 0xFFF);
  CAN.init_Filt(2, 0, 0xFFF);
  CAN.init_Filt(3, 0, 0xFFF);
  CAN.init_Filt(4, 0, 0xFFF);
  CAN.init_Filt(5, 0, 0xFFF);
}

void loop()
{
  fdistanceState = digitalRead(fdistance);
  lanewarnState = digitalRead(lanewarn);

  currentTime = millis(); // grab current time
  //check if "interval" time has passed (100 milliseconds)
  if ((unsigned long)(currentTime - previousTime) >= interval)
  {
    if (fdistanceState == LOW)
    {
      dist100mscount++;

      if (dist100mscount == 2)
      {
        if (JOEL_ID[0] == 0x02)
        {
          JOEL_ID[0] = 0x03;
          Serial.println("Follow distance set 3");
        }
        else if (JOEL_ID[0] == 0x03)
        {
          JOEL_ID[0] = 0x01;
          Serial.println("Follow distance set 1");
        }
        else if (JOEL_ID[0] == 0x01)
        {
          JOEL_ID[0] = 0x02;
          Serial.println("Follow distance set 2");
        }
      }
    }
    else
    {
      dist100mscount = 0;
    }

    if (lanewarnState == LOW)
    {
      lane100mscount++;

      if (lane100mscount == 2)
      {
        if (JOEL_ID[1] == 0x01)
        {
          JOEL_ID[1] = 0x00;
          Serial.println("ID1 set 0");
        }
        else if (JOEL_ID[1] == 0x00)
        {
          JOEL_ID[1] = 0x01;
          Serial.println("ID1 set 1");
        }
      }

      if (lane100mscount == 5)
      {
        if (fdistanceState == LOW)
        {
          if (JOEL_ID[2] == 0x01)
          {
            JOEL_ID[2] = 0x00;
            Serial.println("ID2 set 0");
          }
          else if (JOEL_ID[2] == 0x00)
          {
            JOEL_ID[2] = 0x01;
            Serial.println("ID2 set 1");
          }
        }
      }
    }
    else
    {
      lane100mscount = 0;
    }

    CAN.sendMsgBuf(canSendId, 0, 3, JOEL_ID);
    previousTime = currentTime;
  }
}
