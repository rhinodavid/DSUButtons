#include <SPI.h>
#include "mcp_can.h"

//IO Pins
const int fdistance = 3;
const int lanewarn = 4;
const int relay1 = 5;
const int relay2 = 6;

//CAN
const int SPI_CS_PIN = 10;
unsigned char len = 3;
unsigned char buf[8];
unsigned long interval = 100;     // the time we need to wait
unsigned long previousMillis = 0; // millis() returns an unsigned long.
//unsigned char stmp[5] = {0, 0, 0, 0, 0};
unsigned char JOEL_ID[3] = {0x02, 0x01, 0x00};
unsigned char stmp[8] = {0x00, 0x64, 0x00, 0x28, 0x00, 0x00, 0x00, 0x00};
unsigned char NEW_MSG_1[8] = {0};
MCP_CAN CAN(SPI_CS_PIN);
int canSendId = 0x203;

//Logic variables
int fdistanceState = 1;
int lanewarnState = 1;
int lane100mscount = 0;
int dist100mscount = 0;
int lane100mscount2 = 0;
int dist100mscount2 = 0;
int relaystate1 = 1; // 0 = DSU off
int relaystate2 = 1; // 0 = remote off

void setup()
{
  //GPIO
  pinMode(fdistance, INPUT_PULLUP);
  pinMode(lanewarn, INPUT);
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);

  digitalWrite(relay1, relaystate1); //Remember Relay is Backward - HIGH means off
  digitalWrite(relay2, relaystate2);

  //Serial
  Serial.begin(115200);

  //SPI (CAN)
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV2);

  delay(9000);

  //CAN
  while (CAN_OK != CAN.begin(CAN_500KBPS)) // init can bus : baudrate = 500                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            00k
  {
    Serial.println("CAN BUS Shield init fail");
    Serial.println("\tInit CAN BUS Shield again");
    delay(100);
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

  //CAN Write SUB-LOOP
  // Only want to send messages every 100 ms
  unsigned long currentMillis = millis(); // grab current time
  //check if "interval" time has passed (100 milliseconds)
  if ((unsigned long)(currentMillis - previousMillis) >= interval)
  {
    if (fdistanceState == LOW)
    {
      dist100mscount++;
      dist100mscount2++;
    }
    else
    {
      dist100mscount = 0;
      dist100mscount2 = 0;
    }
    if ((fdistanceState == LOW) && (dist100mscount > 2))
    {
      dist100mscount = 0;
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

    if (lanewarnState == LOW)
    {
      lane100mscount++;
      lane100mscount2++;
    }
    else
    {
      lane100mscount = 0;
      lane100mscount2 = 0;
      relaystate2 = 1;
      Serial.println("Relay 2 set HIGH");
      digitalWrite(relay2, relaystate2);
    }
    if ((lanewarnState == LOW) && (lane100mscount > 2))
    {
      lane100mscount = 0;
      if (JOEL_ID[1] == 0x01)
      {
        JOEL_ID[1] = 0x00;
      }
      else if (JOEL_ID[1] == 0x00)
      {
        JOEL_ID[1] = 0x01;
      }
    }
    CAN.sendMsgBuf(canSendId, 0, 3, JOEL_ID);

    if ((fdistanceState == LOW) && (dist100mscount2 == 50))
    {
      dist100mscount2 = 0;
      if (relaystate1 == 0)
      {
        relaystate1 = 1;
        Serial.println("Relay 1 set HIGH");
        digitalWrite(relay1, relaystate1);
      }
      else if (relaystate1 == 1)
      {
        relaystate1 = 0;
        Serial.println("Relay 1 set LOW");
        digitalWrite(relay1, relaystate1);
      }
    }
    if ((lanewarnState == LOW) && (lane100mscount2 > 5))
    {
      relaystate2 = 0;
      Serial.println("Relay 2 set LOW");
      digitalWrite(relay2, relaystate2);
      if (fdistanceState == LOW)
      {
        lane100mscount2 = 0;
        if (JOEL_ID[2] == 0x01)
        {
          JOEL_ID[2] = 0x00;
          Serial.println("Lane Warning set 0");
        }
        else if (JOEL_ID[2] == 0x00)
        {
          JOEL_ID[2] = 0x01;
          Serial.println("Lane Warning set 1");
        }
      }
    }
    previousMillis = millis();
  }
}
