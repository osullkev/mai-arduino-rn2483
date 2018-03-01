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

//create an instance of the rn2xx3 library,
//giving the software serial as port to use
maiRN2xx3 myLora(mySerial);
networkDetails network("Network Name");

// the setup routine runs once when you press reset:
void setup()
{
  //output LED pin
  pinMode(13, OUTPUT);
  led_on();

  // Open serial communications and wait for port to open:
  Serial.begin(57600); //serial port to computer
  mySerial.begin(9600); //serial port to radio
  Serial.println("Startup");

  initialize_radio();

  String nodeStatus = "300051110105071e22"; //FW: Up to Date; Batter: 22%
  //transmit a startup message
  myLora.txCommand("mac tx uncnf 1 ", nodeStatus, false);

  led_off();
  delay(2000);
}

void initialize_radio()
{
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

  String appEUI = network.getAppEUI(); // Insert APP EUI; 
  String appKey = network.getAppKey(); // Insert APP KEY;
 
  join_result = myLora.initOTAA(appEUI, appKey);
  
  while(!join_result)
  {
    Serial.println("Unable to join. Are your keys correct, and do you have Network coverage?");
    delay(60000); //delay a minute before retry
    join_result = myLora.init();
  }
  Serial.println("Successfully joined Network");

}

void transmitMessage(String opcode,String seqNum, String data, bool ack)
{

  String payload = opcode + seqNum + "000" + data;
  String command = ack ? "mac tx cnf 1 " : "mac tx uncnf 1 ";
  
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
        Serial.println("Received downlink (hex): " + received);
//        received = myLora.base16decode(received);
//        Serial.print("Received downlink: " + received);
        break;
      }
      default:
      {
        Serial.println("Unknown response from TX function");
      }
    }  
}

void transmitSensorReadings(String seqNum, bool ack){
  String opcode = ack ? "2" : "1";
  String data = "331E441E55";  
  transmitMessage(opcode, seqNum, data, ack);
}

void transmitNodeStatus(String seqNum, bool ack){
  String opcode = "3";
  String data = "0105071e22";  
  transmitMessage(opcode, seqNum, data, ack);  
}

String padWithZeros(String s, int l)
{  
  Serial.println("s:" + s + " " + s.length());
  while (s.length() < l){
     s = "0" + s;
  }
  return s;
}

int seqNum = 1;
// the loop routine runs over and over again forever:
void loop()
{
    led_on();

    transmitSensorReadings(padWithZeros(String(seqNum), 4), false);

    led_off();
    Serial.println("Transmitting again in ...");
    Serial.println("15...");
    delay(5000);
    Serial.println("10...");
    delay(5000);
    Serial.println("5...");
    delay(5000);
    seqNum++;    
}

void txCountDown(int t){
  Serial.println("Transmitting again in ...");
  while(t > 0)
  {
    Serial.println(t + "...");
    delay(5000);
    t = t - 5000;
  }
}

void led_on()
{
  digitalWrite(13, 1);
}

void led_off()
{
  digitalWrite(13, 0);
}

