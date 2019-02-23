#include <SPI.h>
#include "mcp_can.h"

//IO Pins
const int FOLLOW_DISTANCE_SWITCH_PIN = 3;
const int FOLLOW_DISTANCE_LED_PIN = 8;
const int LANE_WARN_SWITCH_PIN = 4;
const int LANE_WARN_LED_PIN = 9;

const unsigned int LED_DURATION_SHORT = 250; // ms
const unsigned int LED_DURATION_LONG = 750;  // ms

//CAN
const int SPI_CS_PIN = 10;
const int CAN_SEND_ID = 0x203;
MCP_CAN CAN(SPI_CS_PIN);
unsigned long interval = 100; // ms; the time we need to wait in between attempts to send messages
unsigned long previousTime = 0;
unsigned long currentTime = 0;
unsigned char messageBuffer[3] = {0x02, 0x01, 0x00};

//Logic variables
int followingDistanceState = HIGH;
int laneWarnState = HIGH;
unsigned int lane100mscount = 0;
unsigned int dist100mscount = 0;
unsigned int laneWarnLedOffTime = 0;       // ms
unsigned int followDistanceLedOffTime = 0; // ms

void setup()
{
  pinMode(FOLLOW_DISTANCE_SWITCH_PIN, INPUT_PULLUP);
  pinMode(LANE_WARN_SWITCH_PIN, INPUT_PULLUP);
  pinMode(LANE_WARN_LED_PIN, OUTPUT);
  pinMode(FOLLOW_DISTANCE_LED_PIN, OUTPUT);

  digitalWrite(LANE_WARN_LED_PIN, LOW);
  digitalWrite(FOLLOW_DISTANCE_LED_PIN, LOW);

  Serial.begin(115200);

  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV2);

  delay(5000);

  Serial.println("Initializing CAN Shield");
  while (CAN_OK != CAN.begin(CAN_500KBPS)) // init can bus : baudrate = 500                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            00k
  {
    Serial.println("CAN BUS Shield init fail");
    Serial.println("\tInit CAN BUS Shield again");
    delay(500);
  }
  Serial.println("CAN BUS Shield initialized");

  CAN.init_Mask(0, 0, 0xFFF);
  CAN.init_Mask(1, 0, 0xFFF);
  CAN.init_Filt(0, 0, 0xFFF);
  CAN.init_Filt(1, 0, 0xFFF);
  CAN.init_Filt(2, 0, 0xFFF);
  CAN.init_Filt(3, 0, 0xFFF);
  CAN.init_Filt(4, 0, 0xFFF);
  CAN.init_Filt(5, 0, 0xFFF);
}

void checkLedForShutoff(int pinNumber, unsigned int currentTime /* ms */, unsigned int offTime /* ms */)
{
  if (currentTime > offTime)
  {
    digitalWrite(pinNumber, LOW);
  }
}

void turnOnLed(int pinNumber, int *offTimeVar, unsigned int currentTime /* ms */, unsigned int onDuration /* ms */)
{
  Serial.println("PIN ON");
  digitalWrite(pinNumber, HIGH);
  *offTimeVar = currentTime + onDuration;
}

void loop()
{
  followingDistanceState = digitalRead(FOLLOW_DISTANCE_SWITCH_PIN);
  laneWarnState = digitalRead(LANE_WARN_SWITCH_PIN);

  currentTime = millis(); // grab current time

  checkLedForShutoff(FOLLOW_DISTANCE_LED_PIN, currentTime, followDistanceLedOffTime);
  checkLedForShutoff(LANE_WARN_LED_PIN, currentTime, laneWarnLedOffTime);

  if (currentTime - previousTime >= interval)
  {
    if (followingDistanceState == LOW)
    {
      dist100mscount++;

      if (dist100mscount == 2)
      {
        turnOnLed(FOLLOW_DISTANCE_LED_PIN, &followDistanceLedOffTime, currentTime, LED_DURATION_SHORT);
        if (messageBuffer[0] == 0x02)
        {
          messageBuffer[0] = 0x03;
          Serial.println("Follow distance set 3");
        }
        else if (messageBuffer[0] == 0x03)
        {
          messageBuffer[0] = 0x01;
          Serial.println("Follow distance set 1");
        }
        else if (messageBuffer[0] == 0x01)
        {
          messageBuffer[0] = 0x02;
          Serial.println("Follow distance set 2");
        }
      }
    }
    else
    {
      dist100mscount = 0;
    }

    if (laneWarnState == LOW)
    {
      lane100mscount++;

      if (lane100mscount == 2)
      {
        turnOnLed(LANE_WARN_LED_PIN, &laneWarnLedOffTime, currentTime, LED_DURATION_SHORT);
        if (messageBuffer[1] == 0x01)
        {
          messageBuffer[1] = 0x00;
          Serial.println("ID1 set 0");
        }
        else if (messageBuffer[1] == 0x00)
        {
          messageBuffer[1] = 0x01;
          Serial.println("ID1 set 1");
        }
      }

      if (lane100mscount == 5)
      {
        if (followingDistanceState == LOW)
          turnOnLed(LANE_WARN_LED_PIN, &laneWarnLedOffTime, currentTime, LED_DURATION_LONG);
        {
          if (messageBuffer[2] == 0x01)
          {
            messageBuffer[2] = 0x00;
            Serial.println("ID2 set 0");
          }
          else if (messageBuffer[2] == 0x00)
          {
            messageBuffer[2] = 0x01;
            Serial.println("ID2 set 1");
          }
        }
      }
    }
    else
    {
      lane100mscount = 0;
    }

    CAN.sendMsgBuf(CAN_SEND_ID, 0, 3, messageBuffer);
    previousTime = currentTime;
  }
}
