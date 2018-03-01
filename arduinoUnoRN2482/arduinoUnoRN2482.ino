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

SoftwareSerial mySerial(10, 11); // RX, TX

//create an instance of the rn2xx3 library,
//giving the software serial as port to use
maiRN2xx3 myLora(mySerial);

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

  String appEUI = "0011223344556677"; // Insert APP EUI; 
  String appKey = "6a4e6a045f41bfea6ff752cb176c737f"; // Insert APP KEY;
 
  join_result = myLora.initOTAA(appEUI, appKey);
  
  while(!join_result)
  {
    Serial.println("Unable to join. Are your keys correct, and do you have Network coverage?");
    delay(60000); //delay a minute before retry
    join_result = myLora.init();
  }
  Serial.println("Successfully joined Network");

}

void transmitMessage(String opcode,String seqNum, String len, String data)
{

  String payload = opcode + seqNum + len + data;
  
  Serial.println("Transmitting: " + payload);
  
  switch(myLora.txCommand("mac tx uncnf 1 ", payload, false)) //blocking function
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

// the loop routine runs over and over again forever:
void loop()
{
    led_on();

    String opcode = "2";
    String seqNum = "0001";
    String len = "000";
    String data = "331E441E55";

    transmitMessage(opcode, seqNum, len, data);    

    led_off();
    delay(20000);
}

void led_on()
{
  digitalWrite(13, 1);
}

void led_off()
{
  digitalWrite(13, 0);
}

