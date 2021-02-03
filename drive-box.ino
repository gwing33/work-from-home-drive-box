#include "WiFiEsp.h"
#include <ArduinoJson.h>
#include <Servo.h>
 
Servo servo;

#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"

// set up software sefrial to allow serial communication to our TX and RX pins
//SoftwareSerial Serial1(2, 3); // RX | TX
SoftwareSerial Serial1(2, 3); // RX | TX
#endif

// Set  baud rate of so we can monitor output from esp.
#define ESP8266_BAUD 9600

// CHANGE THIS TO MATCH YOUR SETTINGS
char ssid[] = "WIFI NAME HERe";
char pass[] = "PASSWORD HERE";
int status = WL_IDLE_STATUS;

char server[] = "IP ADDRESS HERE";

WiFiEspClient client;

/* PIR Settings */
int pirPin = 4; // PIR Out pin 
int pirStat = 0; // PIR status

/* Program Settings */
boolean completedToday = false;
boolean isFetching = false;

unsigned long fetchStartTime;
unsigned long fetchPeriod = 320000;
unsigned long monitorStartTime;
unsigned long monitorPeriod = 10000;
unsigned long currentTime;

void setup() {  
  // Open up communications for arduino serial and esp serial at same rate
  Serial.begin(9600);
  Serial1.begin(9600);

  pinMode(pirPin, INPUT);
  servo.attach(9);
  closeFlag();
  // Initialize the esp module
  WiFi.init(&Serial1);

  // Start connecting to wifi network and wait for connection to complete
  while (status != WL_CONNECTED) {
      Serial.print("Conecting to wifi network: ");
      Serial.println(ssid);

      status = WiFi.begin(ssid, pass);
  }
  Serial.println("You're connected to the network");
  
  printAvailableCommands();
  Serial.println();

  monitorStartTime = millis();
}

void loop() {
  fetchStartTime = millis();
  while (completedToday) {
    currentTime = millis();
    handleCommands();
    fetchResults();
    
    if (currentTime - fetchStartTime >= fetchPeriod) {
      fetchStartTime = fetchStartTime + fetchPeriod;
      fetchWfhStatus();
    }
  }

  currentTime = millis();
  
  fetchResults();

  if (currentTime - monitorStartTime >= monitorPeriod) {
    monitorStartTime = monitorStartTime + monitorPeriod;
    monitorForMotion();
  }

  handleCommands();
}

void monitorForMotion() {
  if (!isFetching) {
    pirStat = digitalRead(pirPin);
    if (pirStat == HIGH) {
      fetchWfhStatus();
    }
  }
}

void printAvailableCommands() {
  Serial.println();
  Serial.println("Available commands are: 'help', 'fetchWfhStatus', 'reset', 'openFlag', 'closeFlag', 'prefForFlag'");
}

void handleCommands() {
  if(Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    if(command.equals("help")) {
        printAvailableCommands();
    }
    if(command.equals("fetchWfhStatus")) {
        fetchWfhStatus();
    }
    if(command.equals("reset")) {
        completedToday = false;
        closeFlag();
        Serial.println("Reset completed");
    }
    if(command.equals("openFlag")) {
      openFlag();
      Serial.println("Flag Open");
    }
    if(command.equals("closeFlag")) {
      closeFlag();
      Serial.println("Flag Closed");
    }
    if(command.equals("prefForFlag")) {
      servo.write(5);
      Serial.println("Ready for flag insertion...");
    }
  }
}

void fetchWfhStatus() {
  if (isFetching) {
    return;
  }
  isFetching = true;
  if (client.connect(server, 3000)) {
//    Serial.println("Attempting to fetch wfh status");
    // Make a HTTP request
    client.println("GET /wfh/status HTTP/1.1");
    client.println("Host: 192.168.7.135:3000");
    client.println("Connection: close");
    client.println();
  }
}

void fetchResults() {
  // if there are incoming bytes available
  // from the server, read them and print them
  while (client.available()) {
    // ignore headers and read to first json bracket
    client.readStringUntil('{');

    // get json body (everything inside of the main brackets)
    String jsonStrWithoutBrackets = client.readStringUntil('}');

    // Append brackets to make the string parseable as json
    String jsonStr = "{" + jsonStrWithoutBrackets + "}";
    const size_t bufferSize = JSON_OBJECT_SIZE(1) + 20;
    DynamicJsonBuffer jsonBuffer(bufferSize);
    JsonObject &root = jsonBuffer.parseObject(jsonStr);
    
    boolean isCompleted = root["isCompleted"];
    
    if (completedToday) {
      if (!isCompleted) {
        Serial.println("Fetch says to reset!");
        closeFlag();
        completedToday = false;
      } else {
        Serial.println("Don't reset yet");
      }
      isFetching = false;
      return;
    }
    
    if (isCompleted) {
      completedToday = true;
      long hours = atol(root["hours"]);
      long minutes = atol(root["minutes"]);
      long seconds = atol(root["seconds"]);
      
      openFlag();
      
      Serial.println("Travel was completed in " + getFormattedDuration(hours, minutes, seconds));
    } else {
      Serial.println("Travel was not completed");
    }
    isFetching = false;
  }
}

String getFormattedDuration(long hours, long minutes, long seconds) {
  String hoursStr = hours + plural(hours, " hour");
  String minutesStr = minutes + plural(minutes, " minute");
  String secondsStr = seconds + plural(seconds, " second");

  boolean hasHour = hours > 0;
  boolean hasMin = minutes > 0;
  boolean hasSec = seconds > 0;

  if (hasHour && hasMin && hasSec) {
    return hoursStr + ", " + minutesStr + " and " + secondsStr;
  }
  if (hasHour && hasMin) {
    return hoursStr + " and " + minutesStr;
  }
  if (hasHour && hasSec) {
    return hoursStr + " and " + secondsStr;
  }
  if (hasMin && hasSec) {
    return minutesStr + " and " + secondsStr;
  }
  if (hasHour) {
    return hoursStr;
  }
  if (hasMin) {
    return minutesStr;
  }
  if (hasSec) {
    return secondsStr;
  }
  
  return "";
}

String join(String vals[], String sep, int items) {
  String out = "";
  
  for (int i=0; i<items; i++) {
    out = out + String(vals[i]);
    if ((i + 1) < items) {
      out = out + sep;
    }
  }

  return out;
}

String plural(long count, String w) {
  if (count == 1) {
    return w;
  }
  return w + "s";
}

void openFlag() {
  servo.write(120);
}

void closeFlag() {
  servo.write(0);
}

// This is for running AT commands to setup WIFI
// Initially set the baud to 115200
// Then once it's running set the new baud with: 'AT+UART_DEF=9600,8,1,0,0'
//#include <SoftwareSerial.h>
//
//SoftwareSerial ESPserial(2, 3); // RX | TX
//
//void setup() {
//  Serial.begin(115200); // communication with the host computer
//  
//  // Start the software serial for communication with the ESP8266
//  ESPserial.begin(115200);
//  Serial.println("Started");
//}
//
//void loop() {
//  // listen for communication from the ESP8266 and then write it to the serial monitor
//  if (ESPserial.available()) {
//    Serial.write( ESPserial.read() );
//  }
//  
//  // listen for user input and send it to the ESP8266
//  if (Serial.available()) {
//    ESPserial.write(Serial.read());
//  }
//}