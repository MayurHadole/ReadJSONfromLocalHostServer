// Sample Arduino Json Web Client
// Downloads and parse http://jsonplaceholder.typicode.com/users/1
//
// Copyright Benoit Blanchon 2014-2017
// MIT License
//
// Arduino JSON library
// https://bblanchon.github.io/ArduinoJson/
// If you like this project, please add a star!

#include <ArduinoJson.h>
#include <SPI.h>
#include <WiFi101.h>
#include <stdlib.h> 

char ssid[] = "kunal"; //  your network SSID (name)
char pass[] = "kunal1109";    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;            // your network key Index number (needed only for WEP)

int status = WL_IDLE_STATUS;
// if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the server:

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
WiFiClient client;

const char* server = "192.168.43.150";  // server's address http://192.168.209.1:3000/
const char* resource = "/data";                    // http resource
//const unsigned long BAUD_RATE = 9600;                 // serial connection speed
const unsigned long HTTP_TIMEOUT = 10000;  // max respone time from server
const size_t MAX_CONTENT_SIZE = 512;       // max size of the HTTP response

// The type of data that we want to extract from the page
struct UserData {
  char led1[5];
  char led2[5];
  char led3[5];
  int led1PWM;
  int led2PWM;
  int led3PWM;
 // char name[32];
  //char company[32];
};

void setup() {
  pinMode(A3, OUTPUT);
  pinMode(A4, OUTPUT);
  pinMode(0, OUTPUT);
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

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
}

// ARDUINO entry point #2: runs over and over again forever
void loop() {
  if (connect(server)) {
    if (sendRequest(server, resource) && skipResponseHeaders()) {
      UserData userData;
      if (readReponseContent(&userData)) {
        modifyResponseContent(&userData);
        printUserData(&userData);
        startLEDS(&userData);
      }
    }
  }
  disconnect();
  wait();
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
//   }
// }
bool readReponseContent(struct UserData* userData) {
  // Compute optimal size of the JSON buffer according to what we need to parse.
  // This is only required if you use StaticJsonBuffer.
  const size_t BUFFER_SIZE =
      JSON_OBJECT_SIZE(8)    // the root object has 8 elements
      + JSON_OBJECT_SIZE(5)  // the "address" object has 5 elements
      + JSON_OBJECT_SIZE(2)  // the "geo" object has 2 elements
      + JSON_OBJECT_SIZE(3)  // the "company" object has 3 elements
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
  
  // It's not mandatory to make a copy, you could just use the pointers
  // Since, they are pointing inside the "content" buffer, so you need to make
  // sure it's still in memory when you read the string

  return true;
}


// Print the data extracted from the JSON
void printUserData(const struct UserData* userData) {
  Serial.print("led1 = ");
  Serial.println(userData->led1);
  Serial.print("led2 = ");
  Serial.println(userData->led2);
  Serial.print("led3 = ");
  Serial.println(userData->led3);
  Serial.println("LED Values");
  Serial.println(userData->led1PWM);
  Serial.println(userData->led2PWM);
  Serial.println(userData->led3PWM);
}

void modifyResponseContent(struct UserData* userData) {
  userData->led1PWM = atoi(userData->led1);
  userData->led2PWM = atoi(userData->led2);
  userData->led3PWM = atoi(userData->led3);
}

void startLEDS(struct UserData* userData){
  analogWrite(A3,userData->led1PWM);
  analogWrite(A4,userData->led2PWM);
  analogWrite(0,userData->led3PWM);
  
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
