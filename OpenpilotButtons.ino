#include <Arduino.h>
#include <SPI.h>
#include <StackArray.h>

#include "mcp_can.h" // https://github.com/Flori1989/MCP2515_lib

typedef struct
{
  int pinNumber;
  unsigned long offTime;
} ledIndicator;

typedef enum
{
  accTimeGapButtonPressShort,
  accTimeGapButtonPressLong,
} buttonPress;

//IO Pins
const int ACC_TIME_GAP_SWITCH_PIN = 3; // connected to the car's cruise control follow setting button

ledIndicator accTimeGapButtonIndicatorLed = {/* pin number: */ 8, /* init off time: */ 0};

const unsigned int LED_DURATION_SHORT = 250; // ms; how long to keep the led lit
const unsigned int LED_DURATION_LONG = 750;  // ms; how long to keep the led lit

//CAN
const int SPI_CS_PIN = 10;     // used to communicate with the CAN shield
const int CAN_SEND_ID = 0x203; // 515 in base 10
MCP_CAN CAN(SPI_CS_PIN);
unsigned long canInterval = 100;            // ms; the time we need to wait in between attempts to send messages
unsigned long buttonSampleInterval = 20;    // ms; the time in between button push samples
unsigned long previousCanTime = 0;          // ms; the last time we sent a CAN message. Used to keep the CAN interval
unsigned long previousButtonSampleTime = 0; // ms; the last time we sampled for button presses
unsigned long currentTime = 0;              // ms
/*
 * CAN Message
 * OpenDBC format is:
 * BO_ 515 OPENPILOT_BUTTONS: 8 XXX
 *  SG_ ACC_TIME_GAP : 1|2@0+ (1,0) [0|1] "" XXX
 */
unsigned char canMessageBuffer[1] = {0x00};
const int ACC_TIME_GAP_CAN_POSITION = 0;
StackArray<buttonPress> pendingButtonPresses;

unsigned int accTimeGapButtonPressSamples = 0;

int accTimeGapButtonState = HIGH; // pulled to ground (LOW) when the button is pressed

/**
 * Turn on the LED at the pin number specified in `led`. Reset the pin off time in `led` based on the
 * current time and the duration the LED should be on.
 */
void turnOnLed(ledIndicator *led, unsigned long currentTime /* ms */, unsigned int onDuration /* ms */)
{
  digitalWrite(led->pinNumber, HIGH);
  led->offTime = currentTime + onDuration;
}

/**
 * Check to see if it is passed the time specified in the shut off time of `led` and if so
 * shut it off.
 */
void checkLedForShutoff(ledIndicator *led, unsigned long currentTime /* ms */)
{
  if (currentTime > led->offTime)
  {
    digitalWrite(led->pinNumber, LOW);
  }
}

void sendCanMessage()
{
  CAN.sendMsgBuf(CAN_SEND_ID, 0, 1, canMessageBuffer);
}

void setup()
{
  pinMode(ACC_TIME_GAP_SWITCH_PIN, INPUT_PULLUP);
  pinMode(accTimeGapButtonIndicatorLed.pinNumber, OUTPUT);

  digitalWrite(accTimeGapButtonIndicatorLed.pinNumber, LOW);

  Serial.begin(115200);

  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV2);

  delay(5000); // allow the CAN shield to boot up

  Serial.println("Initializing CAN Shield");
  while (CAN_OK != CAN.begin(CAN_500KBPS)) // init can bus : baudrate = 500                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            00k
  {
    Serial.println("CAN BUS Shield ininitialization failed\nRetrying...");
    delay(500);
  }
  Serial.println("CAN BUS Shield initialized");

  // blink the LED
  digitalWrite(accTimeGapButtonIndicatorLed.pinNumber, HIGH);
  delay(50);
  for (int i = 0; i < 9; i++)
  {
    digitalWrite(accTimeGapButtonIndicatorLed.pinNumber, LOW);
    delay(50);
    digitalWrite(accTimeGapButtonIndicatorLed.pinNumber, HIGH);
    delay(50);
  }
  digitalWrite(accTimeGapButtonIndicatorLed.pinNumber, LOW);

  sendCanMessage();
}

void loop()
{
  currentTime = millis();
  checkLedForShutoff(&accTimeGapButtonIndicatorLed, currentTime);

  // sample buttons
  if (currentTime - previousButtonSampleTime >= buttonSampleInterval)
  {
    accTimeGapButtonState = digitalRead(ACC_TIME_GAP_SWITCH_PIN);

    if (accTimeGapButtonState == LOW)
    {
      accTimeGapButtonPressSamples++;
    }
    else
    {
      if (accTimeGapButtonPressSamples > 12)
      {
        pendingButtonPresses.push(accTimeGapButtonPressLong);
      }
      else if (accTimeGapButtonPressSamples > 2)
      {
        pendingButtonPresses.push(accTimeGapButtonPressShort);
      }
      accTimeGapButtonPressSamples = 0;
    }

    previousButtonSampleTime = currentTime;
  }

  // handle pending putton presses and send CAN message
  if (currentTime - previousCanTime >= canInterval)
  {
    while (!pendingButtonPresses.isEmpty())
    {
      buttonPress press = pendingButtonPresses.pop();
      switch (press)
      {
      case accTimeGapButtonPressShort:
        turnOnLed(&accTimeGapButtonIndicatorLed, currentTime, LED_DURATION_SHORT);
        // we send a zero out in the ACC Time Gap postion continuously, then if we detect
        // a short press of the ACC time gap control button we switch to sending 1s, and
        // visa versa
        if (canMessageBuffer[ACC_TIME_GAP_CAN_POSITION] == 0x00)
        {
          canMessageBuffer[ACC_TIME_GAP_CAN_POSITION] = 0x01;
          Serial.println("Time gap signal set HIGH");
        }
        else
        {
          canMessageBuffer[ACC_TIME_GAP_CAN_POSITION] = 0x00;
          Serial.println("Time gap signal set LOW");
        }
        break;
      case accTimeGapButtonPressLong:
        turnOnLed(&accTimeGapButtonIndicatorLed, currentTime, LED_DURATION_LONG);
        // not doing anything else right now, just flash the LED
        break;
      }
    }
    sendCanMessage();
    previousCanTime = currentTime;
  }
}
