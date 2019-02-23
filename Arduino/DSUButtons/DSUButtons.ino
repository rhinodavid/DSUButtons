#include "mcp_can.h"
#include <SPI.h>
#include <StackArray.h>

typedef struct
{
  int pinNumber;
  unsigned long offTime;
} ledIndicator;

typedef enum
{
  laneWarnShort,
  laneWarnLong,
  followDistanceShort,
  followDistanceLong,
} buttonPress;

//IO Pins
const int FOLLOW_DISTANCE_SWITCH_PIN = 3;
const int LANE_WARN_SWITCH_PIN = 4;

ledIndicator laneWarnIndicator = {9, 0};
ledIndicator followDistanceIndicator = {8, 0};

const unsigned int LED_DURATION_SHORT = 333; // ms
const unsigned int LED_DURATION_LONG = 750;  // ms

//CAN
const int SPI_CS_PIN = 10;
const int CAN_SEND_ID = 0x203; // 515 in base 10
MCP_CAN CAN(SPI_CS_PIN);
unsigned long canInterval = 100;         // ms; the time we need to wait in between attempts to send messages
unsigned long buttonSampleInterval = 25; // ms; the time in between button push samples
unsigned long previousCanTime = 0;
unsigned long previousButtonSampleTime = 0;
unsigned long currentTime = 0;
/*
 * CAN Message
 * Format is:
 * BO_ 515 DSU_BUTTONS: 8 XXX
 *  SG_ ACC_DISTANCE : 1|2@0+ (1,0) [1|3] "" XXX
 *  SG_ LANE_WARNING : 8|1@0+ (1,0) [0|1] "" XXX
 *  SG_ ACC_SLOW : 16|1@0+ (1,0) [0|1] "" XXX
 */
unsigned char canMessageBuffer[3] = {0x02, 0x01, 0x00};
StackArray<buttonPress> pendingButtonPresses;

unsigned int laneWarnPressSamples = 0;
unsigned int followDistancePressSamples = 0;

//Logic variables
int followingDistanceState = HIGH;
int laneWarnState = HIGH;
unsigned int lane100mscount = 0;
unsigned int dist100mscount = 0;
unsigned int laneWarnLedOffTime = 0;       // ms
unsigned int followDistanceLedOffTime = 0; // ms

void checkLedForShutoff(ledIndicator *led, unsigned long currentTime /* ms */)
{
  if (currentTime > led->offTime)
  {
    digitalWrite(led->pinNumber, LOW);
  }
}

void turnOnLed(ledIndicator *led, unsigned long currentTime /* ms */, unsigned int onDuration /* ms */)
{
  digitalWrite(led->pinNumber, HIGH);
  led->offTime = currentTime + onDuration;
}

void sendCanMessage()
{
  CAN.sendMsgBuf(CAN_SEND_ID, 0, 3, canMessageBuffer);
}

void setup()
{
  pinMode(FOLLOW_DISTANCE_SWITCH_PIN, INPUT_PULLUP);
  pinMode(LANE_WARN_SWITCH_PIN, INPUT_PULLUP);
  pinMode(followDistanceIndicator.pinNumber, OUTPUT);
  pinMode(laneWarnIndicator.pinNumber, OUTPUT);

  digitalWrite(followDistanceIndicator.pinNumber, LOW);
  digitalWrite(laneWarnIndicator.pinNumber, LOW);

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

  // blink the LEDs
  digitalWrite(followDistanceIndicator.pinNumber, HIGH);
  delay(100);
  digitalWrite(followDistanceIndicator.pinNumber, LOW);
  digitalWrite(laneWarnIndicator.pinNumber, HIGH);
  delay(100);
  digitalWrite(followDistanceIndicator.pinNumber, HIGH);
  digitalWrite(laneWarnIndicator.pinNumber, LOW);
  delay(100);
  digitalWrite(followDistanceIndicator.pinNumber, LOW);

  sendCanMessage();
}

void loop()
{
  currentTime = millis(); // grab current time

  checkLedForShutoff(&followDistanceIndicator, currentTime);
  checkLedForShutoff(&laneWarnIndicator, currentTime);

  // sample buttons
  if (currentTime - previousButtonSampleTime >= buttonSampleInterval)
  {
    followingDistanceState = digitalRead(FOLLOW_DISTANCE_SWITCH_PIN);
    laneWarnState = digitalRead(LANE_WARN_SWITCH_PIN);

    if (followingDistanceState == LOW)
    {
      followDistancePressSamples++;
    }
    else
    {
      if (followDistancePressSamples > 10)
      {
        pendingButtonPresses.push(followDistanceLong);
      }
      else if (followDistancePressSamples > 2)
      {
        pendingButtonPresses.push(followDistanceShort);
      }
      followDistancePressSamples = 0;
    }

    if (laneWarnState == LOW)
    {
      laneWarnPressSamples++;
    }
    else
    {
      if (laneWarnPressSamples > 10)
      {
        pendingButtonPresses.push(laneWarnLong);
      }
      else if (laneWarnPressSamples > 2)
      {
        pendingButtonPresses.push(laneWarnShort);
      }

      laneWarnPressSamples = 0;
    }
    previousButtonSampleTime = currentTime;
  }

  // handle pending putton presses
  if (currentTime - previousCanTime >= canInterval)
  {
    bool canUpdated = false;
    while (!pendingButtonPresses.isEmpty())
    {
      buttonPress press = pendingButtonPresses.pop();
      switch (press)
      {
      case followDistanceShort:
        turnOnLed(&followDistanceIndicator, currentTime, LED_DURATION_SHORT);
        if (canMessageBuffer[0] == 0x02)
        {
          canMessageBuffer[0] = 0x03;
          Serial.println("Follow distance set 3");
        }
        else if (canMessageBuffer[0] == 0x03)
        {
          canMessageBuffer[0] = 0x01;
          Serial.println("Follow distance set 1");
        }
        else if (canMessageBuffer[0] == 0x01)
        {
          canMessageBuffer[0] = 0x02;
          Serial.println("Follow distance set 2");
        }
        canUpdated = true;
        break;
      case followDistanceLong:
        turnOnLed(&followDistanceIndicator, currentTime, LED_DURATION_LONG);
        // not doing anything else right now
        break;
      case laneWarnShort:
      {
        turnOnLed(&laneWarnIndicator, currentTime, LED_DURATION_SHORT);
        if (canMessageBuffer[1] == 0x01)
        {
          canMessageBuffer[1] = 0x00;
          Serial.println("Lane warn set 0");
        }
        else if (canMessageBuffer[1] == 0x00)
        {
          canMessageBuffer[1] = 0x01;
          Serial.println("Lane warn set 1");
        }
      }
        canUpdated = true;
        break;
      case laneWarnLong:
        turnOnLed(&laneWarnIndicator, currentTime, LED_DURATION_LONG);
        // not doing anything else right now
        break;
      }
      if (canUpdated)
        sendCanMessage();
    }
    previousCanTime = currentTime;
  }
}
