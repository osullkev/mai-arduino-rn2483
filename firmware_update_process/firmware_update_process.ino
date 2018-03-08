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

SoftwareSerial mySerial(10, 11); // RX, TX
const int buttonPin = 2;     // the number of the pushbutton pin
int seqNum = 1;

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
    
    countDown(30000, "Updating state in ...");
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
      Serial.println(F("In idle state = 0"));
      break;      
    }
    case 1:
    {
      Serial.println(F("Scheduled Node Status Update - In state = 1"));
      transmitNodeStatus();      
      break;
    }
    case 2:
    {
      Serial.println(F("Requesting firmware update packet - In state = 2"));     
      requestFirmwareUpdatePacket();      
      break;
    }
    case 3:
    {
      Serial.println(F("Checking for missing firmware update packets - In state = 3"));  
      break;    
    }
    case 4:
    {
      Serial.println(F("Updating firmware - In state = 4"));
      break;
    }
  }
}

////////////////////////////////////////////////////////
/////////////     NODE STATUS FUNCTIONS    /////////////
////////////////////////////////////////////////////////

void transmitNodeStatus()
{
  transmitMessage("3", "0105061e22");  // v1.5.6 is out-of-date
}

////////////////////////////////////////////////////////
/////////////     FW UPDATE FUNCTIONS      /////////////
////////////////////////////////////////////////////////

void requestFirmwareUpdatePacket(){  
  transmitMessage("4", "");  
}

char handleUpdatePacket(String packet) 
{
  Serial.print(F("Processing update packet->"));
  Serial.println(packet);

  return '0';
}

////////////////////////////////////////////////////////
/////////////     DOWNLINK HANDLER         /////////////
////////////////////////////////////////////////////////

void handleDownlink(String response)
{
  char opcode = response.charAt(0);
  Serial.print(F("Response Opcode: "));
  Serial.println(opcode);

  switch(opcode)
  {
    case '2':
    {
      state = 0; //Move back to idle state
      break;
    }
    case '3':
    {
      state = 0; //Move back to idle state
      break;
    }
    case '4':
    {
      state = 2; //Move to Request FW Update Packets State.
      break;
    }
    case '5':
    {
      Serial.println(F("Firmware update packet received ..."));
      char flag = handleUpdatePacket(response);
      switch (flag)
      {
        case '0':
        {
          state = 2;      
          break;
        }
        case '1':
        {
          state = 3;      
          break;  
        }
        case '2':
        {
          Serial.println("Pausing FW Update process");
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

void transmitMessage(String opcode, String data)
{
  String payload = opcode + getNextSeqNum() + "000" + data;
  char command[] = "mac tx uncnf 1 ";
  Serial.println("Transmitting: " + payload);
  
  switch(myLora.txCommand(command, payload, false)) //blocking function
    {
      case TX_FAIL:
      {
        Serial.println(F("Transmission UNSUCESSFUL or NOT acknowledged"));
        break;
      }
      case TX_SUCCESS:
      {
        Serial.println(F("Transmission SUCCESSFUL and acknowledged"));
        incrementSeqNum();
        break;
      }
      case TX_WITH_RX:
      {
        String received = myLora.getRx();
        Serial.println(F("Received downlink (hex): "));
        Serial.println(received);
        handleDownlink(received);
        incrementSeqNum();
        break;
      }
      case TX_MAC_ERR:
      {
        Serial.println(F("Problem receiving downlink. Uplink may have been successful."));
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
      }
    }
    logRN2483Response();
}

////////////////////////////////////////////////////////
/////////////     UTILITY FUNCTIONS        /////////////
////////////////////////////////////////////////////////

void countDown(unsigned long t, String m){
  Serial.println(m);
  while(t > 0)
  {
    Serial.print(String(t/1000));
    Serial.println(F("..."));
    t = t - 5000;
    delay(5000);
  }
}

void incrementSeqNum()
{
  seqNum++;
}

String getNextSeqNum()
{
  return padWithZeros(String(seqNum, HEX), 4);
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
  Serial.print(F("RN2483: "));
  Serial.println(myLora.getRN2483Response());
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
 
  join_result = myLora.initOTAA(network.getAppEUI(), network.getAppKey());

  while(!join_result)
  {
    logRN2483Response();
    i++;
    Serial.println("Unable to join. Are your keys correct, and do you have Network coverage?");
    countDown(60000, "Trying to join again in ...");
    Serial.println("Join Attempt: " + String(i));
    join_result = myLora.joinOTAA();
  }
  logRN2483Response();
  Serial.println("Successfully joined Network");
  delay(5000);

}

