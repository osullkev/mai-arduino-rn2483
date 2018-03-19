/*
 * CHECK THE RULES BEFORE USING THIS PROGRAM!
 *
 * CHANGE ADDRESS!
 * Change the device address, network (session) key, and app (session) key to the values
 * that are registered.
 * The appropriate line is "myLora.initOTAA(XXX);"
 *
 * Connect the RN2xx3 as follows:
 * RN2xx3 -- Arduino
 * Uart TX -- 10
 * Uart RX -- 11
 * Reset -- 12
 * Vcc -- 3.3V
 * Gnd -- Gnd
 *
 * If you use an Arduino with a free hardware serial port, you can replace
 * the line "rn2xx3 myLora(mySerial);"
 * with     "rn2xx3 myLora(SerialX);"
 * where the parameter is the serial port the RN2xx3 is connected to.
 * Remember that the serial port should be initialised before calling initTTN().
 * For best performance the serial port should be set to 57600 baud, which is impossible with a software serial port.
 * If you use 57600 baud, you can remove the line "myLora.autobaud();".
 *
 */
#include <maiRN2xx3.h>
#include <SoftwareSerial.h>
#include "networkDetails.h"
#include <stdlib.h> 

SoftwareSerial mySerial(10, 11); // RX, TX
const int buttonPin = 2;     // the number of the pushbutton pin
int seqNum = 1;

bool updateIndices[100];
int numOfUpdatePackets;
String lastIndexReceived = "";

////////////////////////////////////////////////////////
/////////////     Simulate LoRa RX         /////////////
////////////////////////////////////////////////////////
const byte numChars = 100;
char receivedChars[numChars];
boolean newData = false;
////////////////////////////////////////////////////////


//create an instance of the rn2xx3 library,
//giving the software serial as port to use
maiRN2xx3 myLora(mySerial);

int state = 1; // See technical diagrams

// the setup routine runs once when you press reset:
void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(57600);  //serial port to computer
  mySerial.begin(9600); //serial port to radio

  Serial.println(F("This test only sends uplink of opcode=3,4,5,6, i.e uplinks involved in the FW update process."));
  Serial.println(F("-------------------------------------------------"));

  initialize_radio();
  
  delay(2000);
}

void loop()
{
    stateMachine();

    countDown(20000, "Updating state in ...");

    ////////////////////////////////////////////////////////
    /////////////     Simulate LoRa RX         /////////////
    ////////////////////////////////////////////////////////
    //    recvWithStartEndMarkers();
    ////////////////////////////////////////////////////////
}

////////////////////////////////////////////////////////
/////////////     NODE STATUS UPDATE       /////////////
////////////////////////////////////////////////////////

void stateMachine()
{
  switch(state)
  {
    case 0:
    {
      Serial.println(F("[STATE: 0] - NORMAL OPERATION"));
      break;      
    }
    case 1:
    {
      Serial.println(F("[STATE: 1] - STATUS UPDATE"));
      transmitNodeStatus();      
      break;
    }
    case 2:
    {
      Serial.println(F("[STATE: 2] REQ FW UPDATE PACKET"));     
      requestFirmwareUpdatePacket();      
      break;
    }
    case 3:
    {
      Serial.println(F("[STATE: 3] REQ MISSING PACKETS"));  
      recoverMissingPackets();
      break;    
    }
    case 4:
    {
      Serial.println(F("[STATE: 4] COMPLETE UPDATE RECEIVED"));
      delay(3000);
      exit(0);
      break;
    }
  }
}

////////////////////////////////////////////////////////
/////////////     NODE STATUS FUNCTIONS    /////////////
////////////////////////////////////////////////////////

void transmitNodeStatus()
{
  transmitMessage("3", "01050622", false);  // v1.5.6 is out-of-date
}

void handleNotification(String packet)
{

  numOfUpdatePackets = hexStringtoInt(packet.substring(10,14));
  Serial.println("Num of packets: " + String(numOfUpdatePackets));
}

////////////////////////////////////////////////////////
/////////////     FW UPDATE FUNCTIONS      /////////////
////////////////////////////////////////////////////////

//void requestFirmwareUpdatePacket(String missingPackets){   
void requestFirmwareUpdatePacket(){   
  //transmitMessage("4", missingPackets, true);  
  transmitMessage("4", lastIndexReceived, true);  
}

//char handleUpdatePacket(String packet) 
char handleUpdatePacket()
{
  lastIndexReceived = padWithZeros(myLora.getRx().substring(5,8), 4);
  int index = hexStringtoInt(myLora.getRx().substring(5,8));
  Serial.print(F("PROCESSING FW PACKET: [INDEX:"));
  Serial.print(String(index) + "]");  
  updateIndices[index] = true;

  return myLora.getRx().charAt(4);
}

void recoverMissingPackets()
{
  String missingPackets = "";
  bool packetsMissing = false;
  
  for (int i = 1; i <= numOfUpdatePackets; i++)
  {
    if (updateIndices[i] == false)
    {
      packetsMissing = true;
      missingPackets += padWithZeros(String(i, HEX), 4);
    }
  }
  if (packetsMissing)
  {
    lastIndexReceived = packetsMissing;
    requestFirmwareUpdatePacket(); 
    //requestFirmwareUpdatePacket(missingPackets);      
  }
  else
  {
    Serial.print(F("[CHANGE STATE: "));
    Serial.print(String(state) + "------>");
    state = 4;    
    Serial.println(String(state)+"]");    
  }
  
}

////////////////////////////////////////////////////////
/////////////     DOWNLINK HANDLER         /////////////
////////////////////////////////////////////////////////

//void handleDownlink(String response)
void handleDownlink()
{
  char opcode = myLora.getRx().charAt(0);
  String dlSeqNum = myLora.getRx().substring(1,4);
//  String data = response.substring(4);
  Serial.print(F("RX: "));
  Serial.print(F("[OPCODE:"));
  Serial.print(opcode);
  Serial.print(F("][SEQNUM:"));
  Serial.print(dlSeqNum + "]");
//  Serial.print(F("][DATA:"));
//  Serial.println(data + "]");

  switch(opcode)
  {
    case '2':
    {
      Serial.print(F("[CHANGE STATE: "));
      Serial.print(String(state) + "------>");
      state = 0; //Move back to idle state
      Serial.println(String(state)+"]");
      break;
    }
    case '3':
    {
      Serial.print(F("[CHANGE STATE: "));
      Serial.print(String(state) + "------>");
      state = 0; //Move back to idle state
      Serial.println(String(state)+"]");
      break;
    }
    case '4':
    {
      Serial.print(F("[CHANGE STATE: "));
      Serial.print(String(state) + "------>");
      state = 2; //Move to Request FW Update Packets State.
      Serial.println(String(state)+"]");
      handleNotification(myLora.getRx().substring(4));      
      break;
    }
    case '5':
    {
      char flag = handleUpdatePacket();
      switch (flag)
      {
        case '0':
        {
          Serial.println(F("[MORE PACKETS AVAILABLE]"));
          state = 2;      
          break;
        }
        case '1':
        {
          Serial.print(F("[CHANGE STATE: "));
          Serial.print(String(state) + "------>");
          state = 3;      
          Serial.println(String(state)+"]");
          break;  
        }
        case '2':
        {
          Serial.println("Pausing FW Update process");
          break;
        }    
        case '3':
        {
          Serial.println("All packets sent");
          Serial.print(F("[CHANGE STATE: "));
          Serial.print(String(state) + "------>");
          state = 3;      
          Serial.println(String(state)+"]");
          break;
        }  
      }
      break;
    }
    default:
    {
      Serial.print(F("Unknown opcode received: "));
      Serial.println(opcode);
    }
  }  
}


////////////////////////////////////////////////////////
/////////////     UPLINK ASSEMBLERS        /////////////
////////////////////////////////////////////////////////

void transmitMessage(String opcode, String data, bool ack)
{
  String seqNum = getNextSeqNum();
  String payload = opcode + seqNum + data;
//  String command = ack ? "mac tx cnf 1 " : "mac tx uncnf 1 ";
  String command = "mac tx uncnf 1 ";
  Serial.print(F("TX: "));
  Serial.print(F("[OPCODE:"));
  Serial.print(opcode);
  Serial.print(F("][SEQNUM:"));
  Serial.print(seqNum);
  Serial.print(F("][DATA:"));
  Serial.println(data + "]");
  Serial.print(F("TX: "));
  Serial.println(payload);

  
  TX_RETURN_TYPE res = myLora.txCommand(command, payload, false);
//  logRN2483Response();
  
  switch(res) //blocking function
    {
      case TX_FAIL:
      {
        logRN2483Response();
        break;
      }
      case TX_SUCCESS:
      {
        logRN2483Response();
        incrementSeqNum();
        break;
      }
      case TX_WITH_RX:
      {
        Serial.println(F("Message received..."));
        handleDownlink();
        incrementSeqNum();
        break;
      }
      case TX_MAC_ERR:
      {
        Serial.println(F("MAC_ERR: Problem receiving downlink. Uplink may have been successful."));
        incrementSeqNum();
        break;
      }
      case TX_NOT_JOINED:
      {
        Serial.println(F("Not Joined ..."));
        break;
      }
      default:
      {
        Serial.println(F("Unknown response from TX function"));
        logRN2483Response();
        break;
      }
    }
    

    ////////////////////////////////////////////////////////
    /////////////     Simulate LoRa RX         /////////////
    ////////////////////////////////////////////////////////
    //incrementSeqNum();
    //handleSerialInput();
    
    ////////////////////////////////////////////////////////

 
}

////////////////////////////////////////////////////////
/////////////     UTILITY FUNCTIONS        /////////////
////////////////////////////////////////////////////////

void countDown(unsigned long t, String m){
  Serial.print(m);
  while(t > 0)
  {
    Serial.print(String(t/1000));
    Serial.print(F(","));
    t = t - 10000;
    delay(10000);
  }
  Serial.println();
}

int hexStringtoInt(String s)
{
  String workingString = s;
  long A = strtol(workingString.c_str(),NULL,16);
  return String(A).toInt();  
}

void incrementSeqNum()
{
  seqNum++;
}

String getNextSeqNum()
{
  return padWithZeros(String(seqNum, HEX), 3);
}

String padWithZeros(String s, int l)
{  
  while (s.length() < l){
     s = "0" + s;
  }
  return s;
}

void logRN2483Response()
{
  Serial.print(F("RN2483: ["));
  Serial.print(myLora.getRN2483Response());
  Serial.println(F("]"));

}


////////////////////////////////////////////////////////
/////////////     JOIN NETWORK - OTAA      /////////////
////////////////////////////////////////////////////////

void initialize_radio()
{
  networkDetails network("Network Name");
  //reset rn2483
  pinMode(12, OUTPUT);
  digitalWrite(12, LOW);
  delay(500);
  digitalWrite(12, HIGH);

  delay(100); //wait for the RN2xx3's startup message
  mySerial.flush();

  //Autobaud the rn2483 module to 9600. The default would otherwise be 57600.
  myLora.autobaud();

  //check communication with radio
  String hweui = myLora.hweui();
  while(hweui.length() != 16)
  {
    Serial.println(F("Communication with RN2xx3 unsuccesful. Power cycle the board."));
    Serial.println(hweui);
    delay(10000);
    hweui = myLora.hweui();
  }

  //print out the HWEUI so that we can register it via ttnctl
  Serial.println(F("When using OTAA, register this DevEUI: "));
  Serial.println(myLora.hweui());
  Serial.println(F("RN2xx3 firmware version:"));
  Serial.println(myLora.sysver());

  //configure your keys and join the network
  Serial.println(F("Trying to join Network"));
  bool join_result = false;

  int i = 1;
  Serial.println("Join Attempt: " + String(i));

  Serial.println(network.getAppEUI() + "-" + network.getAppKey());
  join_result = myLora.initOTAA(network.getAppEUI(), network.getAppKey());

  while(!join_result)
  {
    logRN2483Response();
    i++;
    Serial.println(F("Unable to join. Are your keys correct, and do you have Network coverage?"));
    countDown(20000, "Trying to join again in ...");
    Serial.println("Join Attempt: " + String(i));
    join_result = myLora.initOTAA(network.getAppEUI(), network.getAppKey());
  }
  logRN2483Response();
  Serial.println(F("Successfully joined Network"));
  delay(5000);

}


////////////////////////////////////////////////////////
/////////////     Simulate LoRa RX         /////////////
////////////////////////////////////////////////////////

void recvWithStartEndMarkers() {
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char rc;
 
 // if (Serial.available() > 0) {
    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();

        if (recvInProgress == true) {
            if (rc != endMarker) {
                receivedChars[ndx] = rc;
                ndx++;
                if (ndx >= numChars) {
                    ndx = numChars - 1;
                }
            }
            else {
                receivedChars[ndx] = '\0'; // terminate the string
                recvInProgress = false;
                ndx = 0;
                newData = true;
            }
        }

        else if (rc == startMarker) {
            recvInProgress = true;
        }
    }
}

void handleSerialInput() {
    if (newData == true) {
        Serial.print("This just in ... ");
        Serial.println(receivedChars);
        myLora.setRx(receivedChars);
        handleDownlink();
        newData = false;
    }
}
////////////////////////////////////////////////////////


