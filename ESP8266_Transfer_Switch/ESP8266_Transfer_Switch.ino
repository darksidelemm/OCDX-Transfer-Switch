/*

  ESP8266 Transfer Relay Controller
  Shamelessly hacked together from various examples.
 
  Mark Jessop 2016 <vk5qi@rfhead.net>

  A good guide on getting Arduino configured to upload to the ESP8266 is here:
  https://learn.sparkfun.com/tutorials/esp8266-thing-hookup-guide/installing-the-esp8266-arduino-addon

  Hardware Information:
    - Built around a Sparkfun "ESP8266 Thing"
    - Using a Freetronics Relay Driver board to switch some larger relays, which in turn drive the 
      Sage Laboratories SFN490EL transfer switch.

  Transfer Switch Information:
    - Two positions:
      - Position A: Ports 2-3 connected, Ports 1-4 connected.
      - Position B: Ports 1-2 connected, Ports 3-4 connected.
    - Usable as a SPDT switch by using port 2 as common, and port 1 (B) & 3 (A) as the two outputs.
    - Pinout (6-pin MIL-STD connector):
      - A: Coil GND
      - B: Switch to position A
      - C: Switch to position B
      - D: Sense Common
      - E: Position A Sense
      - F: Position B Sense.
    - Currently have RELAY_0 (see below) connected to the position A input, and
      RELAY_1 connected to the position B input.
    - Apply voltage for about 1 second to switch reliably.
    - Applying voltage to both pins B and C is probably a bad idea. 
    - Should probably be using the sense indicators.

*/


#include <ESP8266WiFi.h>

//////////////////////
// WiFi Definitions //
//////////////////////
const char WiFiSSID[] = "yourSSID";
const char WiFiPSK[] = "yourpassword";

/////////////////////
// Pin Definitions //
/////////////////////
const int LED_PIN = 5; // Thing's onboard, green LED
const int ANALOG_PIN = A0; // The only analog pin on the Thing, currently not using this.

// Relay Driver IO.
const int RELAY_0 = 4; // Pulsing this relay on will switch to position A.
const int RELAY_1 = 13; // Pulsing this relay on will switch to position B.
const int RELAY_2 = 12; // Un-used with the transfer switch.

#define RELAY_SWITCH_TIME   1000 // How many milliseconds to hold the coil voltage on to switch.

#define POSITION_A 0
#define POSITION_B 1

int ANTENNA_STATE = POSITION_A;

int RELAY_0_STATE = 0;
int RELAY_1_STATE = 0;
int RELAY_2_STATE = 0;

void set_state(int new_state){
  if (new_state>1) return;

  digitalWrite(RELAY_0, LOW);
  digitalWrite(RELAY_1, LOW);

  if (new_state == POSITION_A){
    digitalWrite(RELAY_0, HIGH);
    delay(RELAY_SWITCH_TIME);
    digitalWrite(RELAY_0, LOW);
    ANTENNA_STATE = POSITION_A;
  }else{
    digitalWrite(RELAY_1, HIGH);
    delay(RELAY_SWITCH_TIME);
    digitalWrite(RELAY_1, LOW);
    ANTENNA_STATE = POSITION_B;  
  }
  return;
}


WiFiServer server(80);

void setup() 
{
  initHardware();
  set_state(POSITION_A);
  connectWiFi();
  server.begin();
}

void loop() 
{
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();

  // Match the request
  int val = -1; // We'll use 'val' to keep track of both the
                // request type (read/set) and value if set.
  if (req.indexOf("/led/0") != -1)
    val = 0; // Will write LED low
  else if (req.indexOf("/led/1") != -1)
    val = 1; // Will write LED high
  // Leaving in ability to individually switch relays.
  else if (req.indexOf("/relay0/0") != -1)
  	val = 2;
  else if (req.indexOf("/relay0/1") != -1)
  	val = 3;
  else if (req.indexOf("/relay1/0") != -1)
  	val = 4;
  else if (req.indexOf("/relay1/1") != -1)
  	val = 5;
  else if (req.indexOf("/relay2/0") != -1)
  	val = 6;
  else if (req.indexOf("/relay2/1") != -1)
  	val = 7;
  else if (req.indexOf("/antenna/0") != -1)
    val = 8;
  else if (req.indexOf("/antenna/1") != -1)
    val = 9;
  else if (req.indexOf("/read") != -1)
    val = -2; // Will print pin reads
  // Otherwise request will be invalid. We'll say as much in HTML

  client.flush();

  // Prepare the response. Start with the common header:
  String s = "HTTP/1.1 200 OK\r\n";
  s += "Content-Type: text/html\r\n\r\n";
  s += "<!DOCTYPE HTML>\r\n<html>\r\n";

  // LED PIN
  if (val == 0)
  {
    s += "STATE,LED,";
    s += "OFF\n";
    digitalWrite(LED_PIN, LOW);
  }
  else if (val == 1)
  {
    s += "STATE,LED,";
    s += "ON\n";
    digitalWrite(LED_PIN, HIGH);
  }
  // RELAY 0
  else if (val == 2)
  {
    s += "STATE,RELAY0,";
    s += "OFF\n";
    digitalWrite(RELAY_0, LOW);
    RELAY_0_STATE = 0;
  }
  else if (val == 3)
  {
    s += "STATE,RELAY0,";
    s += "ON\n";
    digitalWrite(RELAY_0, HIGH);
    RELAY_0_STATE = 1;
  }
  else if (val == 4)
  {
    s += "STATE,RELAY1,";
    s += "OFF\n";
    digitalWrite(RELAY_1, LOW);
    digitalWrite(LED_PIN, LOW);
    RELAY_1_STATE = 0;
  }
  else if (val == 5)
  {
    s += "STATE,RELAY1,";
    s += "ON\n";
    digitalWrite(RELAY_1, HIGH);
    digitalWrite(LED_PIN, HIGH);
    RELAY_1_STATE = 1;
  }
  else if (val == 6)
  {
    s += "STATE,RELAY2,";
    s += "OFF\n";
    digitalWrite(RELAY_2, LOW);
    RELAY_2_STATE = 0;
  }
  else if (val == 7)
  {
    s += "STATE,RELAY2,";
    s += "ON\n";
    digitalWrite(RELAY_2, HIGH);
    RELAY_2_STATE = 1;
  }
  else if (val == 8) // Switch to Position A
  {
    s += "ANTENNA,";
    s += "0\n";
    set_state(POSITION_A);
  }
  else if (val == 9) // Switch to Position B
  {
    s += "ANTENNA,";
    s += "1\n";
    set_state(POSITION_B);
  }
  else if (val == -2)
  { // If we're reading pins, print out those values:
	s += "RELAYS,";
	s += String(RELAY_0_STATE);
	s += ",";
	s += String(RELAY_1_STATE);
	s += ",";
	s += String(RELAY_2_STATE);
	s += "\n";
  s += "ANTENNA,";
  s += String(ANTENNA_STATE);
  s += "\n";
	s += "RSSI,";
	s += String(WiFi.RSSI());
	s += "\n";
  s += "ANALOG,";
  s += String(analogRead(ANALOG_PIN));
  s += "\n<br>";
  }
  else
  {
    s += "ERROR";
  }
  s += "</html>\n";

  // Send the response to the client
  client.print(s);
  delay(1);
  Serial.println("Client disconnected");

  // The client will actually be disconnected 
  // when the function returns and 'client' object is detroyed
}

void connectWiFi()
{
  byte ledStatus = LOW;

  // Set WiFi mode to station (as opposed to AP or AP_STA)
  WiFi.mode(WIFI_STA);

  // WiFI.begin([ssid], [passkey]) initiates a WiFI connection
  // to the stated [ssid], using the [passkey] as a WPA, WPA2,
  // or WEP passphrase.
  WiFi.begin(WiFiSSID, WiFiPSK);

  // Use the WiFi.status() function to check if the ESP8266
  // is connected to a WiFi network.
  while (WiFi.status() != WL_CONNECTED)
  {
    // Blink the LED
    digitalWrite(LED_PIN, ledStatus); // Write LED high/low
    ledStatus = (ledStatus == HIGH) ? LOW : HIGH;

    // Delays allow the ESP8266 to perform critical tasks
    // defined outside of the sketch. These tasks include
    // setting up, and maintaining, a WiFi connection.
    delay(100);
    // Potentially infinite loops are generally dangerous.
    // Add delays -- allowing the processor to perform other
    // tasks -- wherever possible.
  }
}

void initHardware()
{
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(RELAY_0, OUTPUT);
  pinMode(RELAY_1, OUTPUT);
  pinMode(RELAY_2, OUTPUT);
  digitalWrite(RELAY_0, LOW);
  digitalWrite(RELAY_1, LOW);
  digitalWrite(RELAY_2, LOW);
  digitalWrite(LED_PIN, LOW);
}
