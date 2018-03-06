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
int seqNum = 0;

//create an instance of the rn2xx3 library,
//giving the software serial as port to use
maiRN2xx3 myLora(mySerial);

// the setup routine runs once when you press reset:
void setup()
{
    // Open serial communications and wait for port to open:
  Serial.begin(57600); //serial port to computer
  mySerial.begin(9600); //serial port to radio

  Serial.println("This test only sends uplink of opcode=2, i.e uplinks which expect a response from the server");
  Serial.println("-------------------------------------------------");


  initialize_radio();
  delay(2000);
}

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
    Serial.println("Communication with RN2xx3 unsuccesful. Power cycle the board.");
    Serial.println(hweui);
    delay(10000);
    hweui = myLora.hweui();
  }

  //print out the HWEUI so that we can register it via ttnctl
  Serial.println("When using OTAA, register this DevEUI: ");
  Serial.println(myLora.hweui());
  Serial.println("RN2xx3 firmware version:");
  Serial.println(myLora.sysver());

  //configure your keys and join the network
  Serial.println("Trying to join Network");
  bool join_result = false;

  int i = 1;
  Serial.println("Join Attempt: " + String(i));
 
  join_result = myLora.initOTAA(network.getAppEUI(), network.getAppKey());

//  join_result = true;
  
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

}

// the loop routine runs over and over again forever:
void loop()
{
    transmitSensorReadings(true);
    
    countDown(30000, "Transmitting again in ...");
}

bool nodeStatusButton(unsigned long startTime, String message){
  Serial.print("Press button in 5 seconds: " + message);
  while(millis() - startTime < 5000)
  {
    if (digitalRead(buttonPin) == HIGH)
    {
      Serial.println(" - Button Pressed");
      return true;
    }    
  }
  Serial.println(" - Time Expired");
  return false;  
}

void countDown(unsigned long t, String m){
  Serial.println(m);
  while(t > 0)
  {
    Serial.println(String(t/1000) + "...");
    t = t - 5000;
    delay(5000);
  }
}


String getNextSeqNum()
{
  seqNum++;
  return padWithZeros(String(seqNum, HEX), 4);
}

void transmitMessage(String opcode, String data, bool ack)
{
  String payload = opcode + getNextSeqNum() + "000" + data;
//  String command = ack ? "mac tx cnf 1 " : "mac tx uncnf 1 ";
  char command[] = "mac tx uncnf 1 ";
  Serial.println("Transmitting: " + payload);

  switch(myLora.txCommand(command, payload, false)) //blocking function
    {
      case TX_FAIL:
      {
        Serial.println("Transmission UNSUCESSFUL or NOT acknowledged");
        break;
      }
      case TX_SUCCESS:
      {
        Serial.println("Transmission SUCCESSFUL and acknowledged");
        break;
      }
      case TX_WITH_RX:
      {
        String received = myLora.getRx();
        Serial.println("Received downlink (hex): ");
        Serial.println(received);
        handleDownlink(received);
        break;
      }
      case TX_NOT_JOINED:
      {
        Serial.println("Not Joined ...");
        break;
      }
      default:
      {
        Serial.println("Unknown response from TX function");
      }
    }
    logRN2483Response();
}

void transmitSensorReadings(bool ack){
  String opcode = ack ? "2" : "1";
  transmitMessage(opcode, "331E441E55", ack);
}

void transmitNodeStatus(bool ack){
  transmitMessage("3", "0105071e22", ack);  
}

void transmitNodeStatusOUTOFDATE(bool ack){
  transmitMessage("3", "0105061e22", ack);  
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
  Serial.println("RN2483: " + myLora.getRN2483Response());
}

void handleDownlink(String s)
{
  
}

