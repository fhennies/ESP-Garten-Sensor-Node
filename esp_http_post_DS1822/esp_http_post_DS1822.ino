    #include "config.h"
    #include <ESP8266WiFi.h>
    #include <OneWire.h>
    #include <DallasTemperature.h>

    // Data wire is plugged into pin 2
    #define ONE_WIRE_BUS 4
    #define HOST "data.sparkfun.com"
    #define HOSTIFTTT "maker.ifttt.com"
    #define HTTPPORT 80


//    #define DEBUG

    #ifdef DEBUG
    #define DEBUG_PRINT(x)  Serial.print (x)
    #define DEBUG_PRINTLN(x)  Serial.println (x)
    #define DEBUG_START(x) Serial.begin(x)
    #define DEBUG_DELAY(x) delay(x)
    #else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_START(x)
    #define DEBUG_DELAY(x)
    #endif

    // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
    OneWire oneWire(ONE_WIRE_BUS);

    // Pass our oneWire reference to Dallas Temperature. 
    DallasTemperature sensors(&oneWire);

    float temperature = 20;
    float temperature1 = 20;
    float voltage = 0;

    long startMillis;

    void updateSparkFun() {
      DEBUG_PRINTLN("connecting to ");
      DEBUG_PRINTLN(HOST);

      // Use WiFiClient class to create TCP connections
      WiFiClient client;
      
      if (!client.connect(HOST, HTTPPORT)) {
        DEBUG_PRINTLN("connection failed");
        if (millis()-startMillis > 20000) {
          DEBUG_PRINTLN("Connection timeout, go to sleep for 10 minutes");
          ESP.deepSleep(10 * 60 * 1000000);
        }
        return;
      }

           
      // We now create a URI for the request
      String url = "/input/" + String(SPARK_PUBKEY) + "?private_key=" + String(SPARK_PRIVKEY) + "&temp=" + temperature + "&temp1=" + temperature1 + "&vcc=" + voltage;
      DEBUG_PRINT("Requesting URL: ");
      DEBUG_PRINTLN(url);
      
      // This will send the request to the server
      client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                   "Host: " + HOST + "\r\n" + 
                   "Connection: close\r\n\r\n");
      delay(500);
    }

    void triggerIfttt() {
         DEBUG_PRINTLN("connecting to ");
         DEBUG_PRINTLN(HOSTIFTTT);

         WiFiClient client;

         
         if (!client.connect(HOSTIFTTT, HTTPPORT)) {
          DEBUG_PRINTLN("connection failed");
          if (millis()-startMillis > 30000) {
          DEBUG_PRINTLN("Connection timeout, go to sleep for 10 minutes");
          ESP.deepSleep(10 * 60 * 1000000);
         }
         return;
        }

        // construct the JSON
        String postJson = "{\"value1\":\"" + String(millis()) + "\",\"value2\":\"" + temperature + "\",\"value3\":\"" + voltage + "\"}";

        String postRqst = "POST /trigger/battery_low/with/key/" + String(IFTT_KEY) +" HTTP/1.1\r\n" + "Host: " + HOSTIFTTT + "\r\n"
                           "Content-Type: application/json\r\n" + "Content-Length: " + postJson.length() + "\r\n\r\n" + postJson;
        
        // finally we are ready to send the POST to the server!
        DEBUG_PRINTLN(postRqst);
        client.print(postRqst);
        client.stop();
        delay(500);
    }

    void setup() {

      pinMode(A0, INPUT);

      DEBUG_START(115200);
      DEBUG_DELAY(100);
     
      // We start by connecting to a WiFi network
      DEBUG_PRINTLN();
      DEBUG_PRINTLN();
      DEBUG_PRINT("Connecting to ");
      DEBUG_PRINTLN(WIFI_SSID);

      // stake time for timeout 
      startMillis = millis();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        DEBUG_PRINT(".");
        if (millis()-startMillis > 10000) {
          DEBUG_PRINTLN("WiFi timeout, go to sleep for 10 minutes");
          ESP.deepSleep(10 * 60 * 1000000);
        }
      }

      DEBUG_PRINTLN("");
      DEBUG_PRINTLN("WiFi connected");  
      DEBUG_PRINTLN("IP address: ");
      DEBUG_PRINTLN(WiFi.localIP());

      voltage = analogRead(A0) * 8.0913 / 1000;
      if (voltage < 6.6) {
        DEBUG_PRINTLN("Voltage low, go to sleep for 60 minutes");
        ESP.deepSleep(60 * 60 * 1000000);
      }

      // Start up the DS sensor library
      sensors.setResolution(12);
      sensors.begin(); 
    }
     
    
    void loop() {

      // call sensors.requestTemperatures() to issue a global temperature 
      // request to all devices on the bus
      sensors.requestTemperatures(); // Send the command to get temperatures
      temperature = sensors.getTempCByIndex(0); 
      temperature1 = sensors.getTempCByIndex(1);

      DEBUG_PRINT("Temperatures - Device 0: ");
      DEBUG_PRINT(temperature);
      DEBUG_PRINT(" Device 1: ");
      DEBUG_PRINT(temperature1);
      DEBUG_PRINT(" - Voltage: ");
      DEBUG_PRINTLN(voltage);

      updateSparkFun();
        
      if (voltage < 6.8) {
         DEBUG_PRINTLN("Voltage low");
         triggerIfttt();
      }
      
      DEBUG_PRINTLN("Go to sleep");
      ESP.deepSleep(10 * 60 * 1000000);
      delay(100);
    }
