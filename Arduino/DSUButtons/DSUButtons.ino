#include <SPI.h>
#include "mcp_can.h"


//IO Pins
const int fdistance = 3;
const int lanewarn = 4;
const int relay1 = 5;
const int relay2 = 6;

//CAN
const int SPI_CS_PIN = 10;
unsigned char flagRecv = 0;
unsigned char len = 3;
unsigned char buf[8];
unsigned long interval=100; // the time we need to wait
unsigned long previousMillis=0; // millis() returns an unsigned long.
//unsigned char stmp[5] = {0, 0, 0, 0, 0};
unsigned char JOEL_ID[2] = {0x02, 0x01};
unsigned char stmp[8] = {0x00, 0x64, 0x00, 0x28, 0x00, 0x00, 0x00, 0x00};
unsigned char NEW_MSG_1[8] = {0};
MCP_CAN CAN(SPI_CS_PIN);
int canSendId = 0x203;

//Logic variables
int fdistanceState = 1;
int lanewarnState = 1;
int lane100mscount = 0;
int dist100mscount = 0;
int relaystate1 = 1; // 0 = DSU off
int relaystate2 = 1; // 0 = remote off

////SERIAL INPUT (UNUSED)
//char receivedChar;
 
void setup()
{ 
  //GPIO
  pinMode(fdistance, INPUT_PULLUP);
  pinMode(lanewarn, INPUT_PULLUP);
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);

  digitalWrite(relay1, relaystate1); //Remember Relay is Backward - HIGH means off
  digitalWrite(relay2, relaystate2);

  //Serial 
  Serial.begin(115200);
  
 //SPI (CAN)
  SPI.begin();  
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  delay(1000);
  delay(1000);
  delay(1000);
  delay(1000);
  delay(1000);
  delay(1000);
  delay(1000);
  delay(1000);
  delay(1000);
  delay(1000);  
 //CAN
    while (CAN_OK != CAN.begin(CAN_500KBPS))              // init can bus : baudrate = 1000                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            00k
    {
        Serial.println("CAN BUS Shield init fail");
        Serial.println(" Init CAN BUS Shield again");
        delay(100);
    }
    Serial.println("CAN BUS Shield init ok!");

//  attachInterrupt(2, MCP2515_ISR, FALLING); // start interrupt for can
  CAN.init_Mask(0, 0, 0xFFF); // there are 2 mask in mcp2515, you need to set both of them
  CAN.init_Mask(1, 0, 0xFFF);
  CAN.init_Filt(0, 0, 0xFFF);
  CAN.init_Filt(1, 0, 0xFFF);
  CAN.init_Filt(2, 0, 0xFFF);
  CAN.init_Filt(3, 0, 0xFFF);
  CAN.init_Filt(4, 0, 0xFFF);
  CAN.init_Filt(5, 0, 0xFFF);

}

////SERIAL INPUT (unused)
//void recvOneChar() {
//    if (Serial.available() > 0) {
//        receivedChar = Serial.read();
//    }
//}

//MAIN LOOP
void loop()
{

  fdistanceState = digitalRead(fdistance);
  //Serial.println(fdistanceState);
  lanewarnState = digitalRead(lanewarn);  

  //CAN Write SUB-LOOP
  // Only want to send messages every 100 ms
  unsigned long currentMillis = millis(); // grab current time
  //check if "interval" time has passed (100 milliseconds)
  if ((unsigned long)(currentMillis - previousMillis) >= interval) {

    if (fdistanceState == LOW) {
      dist100mscount++;
    } else {
      dist100mscount = 0;
    }
    if ((fdistanceState == LOW) && (dist100mscount > 1)) {
       if (JOEL_ID[0] == 0x02)
       {
          JOEL_ID[0] = 0x03;
          dist100mscount = 0;
       }
       else if (JOEL_ID[0] == 0x03)
       {
          JOEL_ID[0] = 0x01;
          dist100mscount = 0;
       }
       else if (JOEL_ID[0] == 0x01)
       {
          JOEL_ID[0] = 0x02;
          dist100mscount = 0;
       }
    }
    if (lanewarnState == LOW) {
      lane100mscount++;
    } else {
      lane100mscount = 0;
    }
    if ((lanewarnState == LOW) && (lane100mscount > 1)) {
       if (JOEL_ID[1] == 0x01)
       {
          JOEL_ID[1] = 0x00;
          lane100mscount = 0;
       }
       else if (JOEL_ID[1] == 0x00)
       {
          JOEL_ID[1] = 0x01;
          lane100mscount = 0;
       }
    }
    CAN.sendMsgBuf(canSendId, 0, 2, JOEL_ID);
    //Serial.println(dist100mscount);
    if ((fdistanceState == LOW) && (dist100mscount = 50)) {
      //CAN.sendMsgBuf(canSendId, 0, 2, stmp);
      dist100mscount = 0;
      if (relaystate1 == 0){
        relaystate1 = 1;
        Serial.println("relay1=1");
        digitalWrite(relay1, relaystate1);
        
      }
      else if (relaystate1 == 1) {
        relaystate1 = 0;
        Serial.println("relay1=0");
        digitalWrite(relay1, relaystate1);
      }
    }
    if ((lanewarnState == LOW) && (lane100mscount = 10)) {
      //CAN.sendMsgBuf(canSendId, 0, 2, stmp);
      if (relaystate2 == 0){
        relaystate2 = 1;
        Serial.println("relay2=1");
        digitalWrite(relay2, relaystate2);
        
      }
      else if (relaystate2 == 1) {
        relaystate2 = 0;
        Serial.println("relay2=0");
        digitalWrite(relay2, relaystate2);
      }
    else if ((lanewarnState == HIGH) && (relaystate2 == 1))
        relaystate2 = 0;
        lane100mscount = 0;
        Serial.println("relay2=0");
        digitalWrite(relay2, relaystate2);
    }

    previousMillis = millis();
  }


  if(flagRecv)                   // check if CAN receive is enabled
      {
        int CANID;
        CAN.readMsgBuf(&len, buf);    // read data,  len: data length, buf: data buf
        CANID=CAN.getCanId();
        if (CANID != canSendId) {
          Serial.println("\r\n------------------------------------------------------------------");
          Serial.print("Get Data From id: ");
          Serial.println(CANID);
         for(int i = 0; i<len; i++)    // print the data
          {
              Serial.print("0x");
             Serial.print(buf[i], HEX);
             Serial.print("\t");
          }
          Serial.println();
        }
      }
}
