#include <ArduinoJson.h>
#include <SPI.h>
#include <WiFi101.h>
#include <stdlib.h>
#include <stdint.h>
#include <WiFiUdp.h>
#include <RTCZero.h>

RTCZero rtc;

char ssid[] = "kunal"; //  your network SSID (name)
char pass[] = "kunal1109";    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;            // your network key Index number (needed only for WEP)

int status = WL_IDLE_STATUS;
// if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the server:

// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
WiFiClient client;

const char* server = "192.168.43.150";  // server's address http://192.168.209.1:3000/
const char* resource = "/data";                    // http resource
const unsigned long HTTP_TIMEOUT = 10000;  // max respone time from server
const size_t MAX_CONTENT_SIZE = 512;       // max size of the HTTP response

#define PWMCHARLENGTH 3
#define TIMECHARLENGTH 6
#define INTBASE 10

#define LED1PIN A3
#define LED2PIN A4
#define LED3PIN 0

const int TIMEZONE = -14400; //change this to adapt it to your time zone (in seconds)
const unsigned long HOUR = 60 * 60;
const unsigned long MINUTE = 60;
const int TARGET_BRIGHTNESS = (255 * 3) / 4;  // Maximum brightness at 75%


// The type of data that we want to extract from the page
struct UserData {
  char led1[PWMCHARLENGTH];
  char led2[PWMCHARLENGTH];
  char led3[PWMCHARLENGTH];
  char fadeUpTime[TIMECHARLENGTH];
  char fadeUpDuration[TIMECHARLENGTH];
  char fadeDownTime[TIMECHARLENGTH];
  char fadeDownDuration[TIMECHARLENGTH];

};

//convert the data to useful types
struct ConvertedData {
  uint8_t led1PWM;
  uint8_t led2PWM;
  uint8_t led3PWM;
  unsigned long fadeUpTime;
  unsigned long fadeUpDuration;
  unsigned long fadeDownTime;
  unsigned long fadeDownDuration;

};


void setup() {
  pinMode(LED1PIN, OUTPUT);
  pinMode(LED2PIN, OUTPUT);
  pinMode(LED3PIN, OUTPUT);
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  connectToWiFi();
  
  rtc.begin();
  unsigned long epoch;
  int numberOfTries = 0, maxTries = 6;
  do {
    epoch = WiFi.getTime();
    numberOfTries++;
  }
  while ((epoch == 0) || (numberOfTries > maxTries));
   Serial.println("here");
  if (numberOfTries > maxTries) {
    Serial.print("NTP unreachable!!");
    while (1);
  }
  else {
    Serial.print("Epoch received: ");
    Serial.println(epoch);
    rtc.setEpoch(epoch);

    Serial.println();
  }
}
boolean connectedToServer = false;
// ARDUINO entry point #2: runs over and over again forever
void loop() {
  static UserData userData;
  static ConvertedData convertedData;
  if (~connectedToServer)
  {
    Serial.println("connecting Server");
    if (connect(server)) {
      if (sendRequest(server, resource) && skipResponseHeaders()) {
        //UserData userData;
        //ConvertedData convertedData;
        if (readReponseContent(&userData)) {
          modifyResponseContent(&userData, &convertedData);
          printUserData(&convertedData);
          startLEDS(&convertedData);
        }
      }
    }
    disconnect();
    connectedToServer = true;

    //wait();
  }
  unsigned long TIME = (((rtc.getHours() * HOUR) + (rtc.getMinutes() * MINUTE) + rtc.getSeconds()) + TIMEZONE);
  Serial.println(TIME);
  Serial.println("");
  Serial.print("LED: ");
  Serial.println(brightness(TIME,&convertedData));
  analogWrite(LED1PIN, brightness(TIME , &convertedData));
  delay(1000);
}


// Open connection to the HTTP server
bool connect(const char* hostName) {
  Serial.print("Connect to ");
  Serial.println(hostName);

  bool ok = client.connect(hostName, 3000);

  Serial.println(ok ? "Connected" : "Connection Failed!");
  return ok;
}

// Send the HTTP GET request to the server
bool sendRequest(const char* host, const char* resource) {
  Serial.print("GET ");
  Serial.println(resource);

  client.print("GET ");
  client.print(resource);
  client.println(" HTTP/1.0");
  client.print("Host: ");
  client.println(host);
  client.println("Connection: close");
  client.println();

  return true;
}

// Skip HTTP headers so that we are at the beginning of the response's body
bool skipResponseHeaders() {
  // HTTP headers end with an empty line
  char endOfHeaders[] = "\r\n\r\n";

  client.setTimeout(HTTP_TIMEOUT);
  bool ok = client.find(endOfHeaders);

  if (!ok) {
    Serial.println("No response or invalid response!");
  }

  return ok;
}

// Parse the JSON from the input string and extract the interesting values
// Here is the JSON we need to parse
// {
//   "led1": "100",
//   "led2": "200",
//   "led3": "255",
//   "fadeUpTime"      : "86400"
//   "fadeUpDuration"  : "86400"
//   "fadeDownTime"    : "86400"
//   "fadeDownDuration": "86400"
//   }
// }
bool readReponseContent(struct UserData* userData) {
  // Compute optimal size of the JSON buffer according to what we need to parse.
  // This is only required if you use StaticJsonBuffer.
  const size_t BUFFER_SIZE =
    JSON_OBJECT_SIZE(7)    // the root object has 7 elements
    + MAX_CONTENT_SIZE;    // additional space for strings

  // Allocate a temporary memory pool
  DynamicJsonBuffer jsonBuffer(BUFFER_SIZE);

  JsonObject& root = jsonBuffer.parseObject(client);

  if (!root.success()) {
    Serial.println("JSON parsing failed!");
    return false;
  }

  // Here were copy the strings we're interested in
  strcpy(userData->led1, root["led1"]);
  strcpy(userData->led2, root["led2"]);
  strcpy(userData->led3, root["led3"]);
  strcpy(userData->fadeUpTime, root["fadeUpTime"]);
  strcpy(userData->fadeUpDuration, root["fadeUpDuration"]);
  strcpy(userData->fadeDownTime, root["fadeDownTime"]);
  strcpy(userData->fadeDownDuration, root["fadeDownDuration"]);



  // It's not mandatory to make a copy, you could just use the pointers
  // Since, they are pointing inside the "content" buffer, so you need to make
  // sure it's still in memory when you read the string

  return true;
}


// Print the data extracted from the JSON
void printUserData(const struct ConvertedData* ConvertedData) {
  Serial.print("led1 = ");
  Serial.println(ConvertedData->led1PWM);
  Serial.print("led2 = ");
  Serial.println(ConvertedData->led2PWM);
  Serial.print("led3 = ");
  Serial.println(ConvertedData->led3PWM);
  Serial.print("fadeUpTime = ");
  Serial.println(ConvertedData->fadeUpTime);
  Serial.print("fadeUpDuration = ");
  Serial.println(ConvertedData->fadeUpDuration);
  Serial.print("fadeDownTime = ");
  Serial.println(ConvertedData->fadeDownTime);
  Serial.print("fadeDownDuration = ");
  Serial.println(ConvertedData->fadeDownDuration);
}

void modifyResponseContent(struct UserData* userData, struct ConvertedData* convertedData ) {
  convertedData->led1PWM = atoi(userData->led1);
  convertedData->led2PWM = atoi(userData->led2);
  convertedData->led3PWM = atoi(userData->led3);
  convertedData->fadeUpTime = strtoul(userData->fadeUpTime, NULL, INTBASE);
  convertedData->fadeUpDuration = strtoul(userData->fadeUpDuration, NULL, INTBASE);
  convertedData->fadeDownTime = strtoul(userData->fadeDownTime, NULL, INTBASE);
  convertedData->fadeDownDuration = strtoul(userData->fadeDownDuration, NULL, INTBASE);

}
void startLEDS(struct ConvertedData* ConvertedData) {
  analogWrite(LED1PIN, ConvertedData->led1PWM);
  analogWrite(LED2PIN, ConvertedData->led2PWM);
  analogWrite(LED3PIN, ConvertedData->led3PWM);

}
// Close the connection with the HTTP server
void disconnect() {
  Serial.println("Disconnect");
  client.stop();
}

// Pause for a 1 minute
void wait() {
  Serial.println("Wait 60 seconds");
  delay(60000);
}

void connectToWiFi() {
  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }
  Serial.println("Connected to wifi");
  printWiFiStatus();
  delay(5000);
}

void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

//Time in Seconds
// 7  = 25200
// 12 = 43200
// 16 = 57600
//Dawn   : LED will start to fade up from FadeupStartTime till(FadeUpStartTime + 2 hours ).
//Mid Day: LED will be at 75% after (FadeUStartTime + 2 hours ).
//Evening: LED will start to fade down from FadeDownStartTime till (FadeDownStartTime + 2 hours);
//Night  : LED will be OFF
byte brightness(unsigned long TIME , struct ConvertedData* convertedData)
{

  //Mid day: light is at maximum brightness
  if (TIME >= ( convertedData -> fadeUpTime + convertedData -> fadeUpDuration ) && ( TIME <= convertedData -> fadeDownTime ))
{
  return TARGET_BRIGHTNESS;
}

//Dawn: Fade up the light
if (TIME >= ( convertedData -> fadeUpTime) && TIME  <= ( convertedData -> fadeUpTime + convertedData -> fadeUpDuration ))
{
  unsigned long seconds = TIME - ( convertedData -> fadeUpTime);  //No of seconds into the fade time
    return TARGET_BRIGHTNESS * seconds / (convertedData -> fadeUpDuration ) ; //fade up based on portion of  interval completed
  }

  //Evening: Fade down the light
  if (TIME >= ( convertedData -> fadeDownTime) && TIME <= ( convertedData -> fadeDownTime) + ( convertedData -> fadeDownDuration )) // Fading down
{
  unsigned long seconds = (( convertedData -> fadeDownTime) + ( convertedData -> fadeDownDuration )) - TIME; // Number of seconds remaining in the fade time
    return TARGET_BRIGHTNESS * seconds / ( convertedData -> fadeDownDuration ); // Fade down based on portion of interval left.
  }

  //Night: lights are OFF
  return 0;
}

void printTime()
{
  print2digits(rtc.getHours() + (TIMEZONE * HOUR * HOUR));
  Serial.print(":");
  print2digits(rtc.getMinutes());
  Serial.print(":");
  print2digits(rtc.getSeconds());
  Serial.println();
}

void printDate()
{
  Serial.print(rtc.getDay());
  Serial.print("/");
  Serial.print(rtc.getMonth());
  Serial.print("/");
  Serial.print(rtc.getYear());
  Serial.print(" ");
}

void print2digits(int number) {
  if (number < 10) {
    Serial.print("0");
  }
  Serial.print(number);
}


