    /*
     *  Simple HTTP get webclient test
     */

    #include "config.h"
    #include <ESP8266WiFi.h>
    #include <OneWire.h>
    #include <DallasTemperature.h>

    // Data wire is plugged into pin 2
    #define ONE_WIRE_BUS 4

    #define DEBUG

    #ifdef DEBUG
    #define DEBUG_PRINT(x)  Serial.print (x)
    #define DEBUG_PRINTLN(x)  Serial.println (x)
    #else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #endif

    // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
    OneWire oneWire(ONE_WIRE_BUS);

    // Pass our oneWire reference to Dallas Temperature. 
    DallasTemperature sensors(&oneWire);

    // helper functions for constructing the POST data
    // append a string or int to a buffer, return the resulting end of string

    char *append_str(char *here, char *s) {
      while (*here++ = *s++)
        ;
      return here-1;
    }

    char *append_ul(char *here, unsigned long u) {
      char buf[20];       // we "just know" this is big enough
      return append_str(here, ultoa(u, buf, 10));
    }


    const char* host = "data.sparkfun.com";
    const char* hostIfttt = "maker.ifttt.com";
    
    float temperature = 20;
    float temperature1 = 20;
    float voltage = 0;

    long startMillis;

    void setup() {

      pinMode(A0, INPUT);

#ifdef DEBUG
      Serial.begin(115200);
      delay(100);
#endif
     
      // We start by connecting to a WiFi network
     
      DEBUG_PRINTLN();
      DEBUG_PRINTLN();
      DEBUG_PRINT("Connecting to ");
      DEBUG_PRINTLN(WIFI_SSID);

      startMillis = millis();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        DEBUG_PRINT(".");
        if (millis()-startMillis > 10000) {
          DEBUG_PRINTLN("WiFi timeout");
          ESP.deepSleep(10 * 60 * 1000000);
        }
      }

      DEBUG_PRINTLN("");
      DEBUG_PRINTLN("WiFi connected");  
      DEBUG_PRINTLN("IP address: ");
      DEBUG_PRINTLN(WiFi.localIP());

      voltage = analogRead(A0) * 8.0913 / 1000;
      if (voltage < 6.6) {
        DEBUG_PRINTLN("Voltage low, go to sleep");
//        ESP.deepSleep(60 * 60 * 1000000);
      }

      // Start up the DS sensor library
      sensors.setResolution(12);
      sensors.begin(); // IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature measurement
    }
     
    
    void loop() {

      // call sensors.requestTemperatures() to issue a global temperature 
      // request to all devices on the bus
      sensors.requestTemperatures(); // Send the command to get temperatures
      temperature = sensors.getTempCByIndex(0); // Why "byIndex"? You can have more than one IC on the same bus. 0 refers to the first IC on the wire
      temperature1 = sensors.getTempCByIndex(1); // Why "byIndex"? You can have more than one IC on the same bus. 0 refers to the first IC on the wire

      DEBUG_PRINT("Temperatures - Device 0: ");
      DEBUG_PRINT(temperature);
      DEBUG_PRINT(" Device 1: ");
      DEBUG_PRINT(temperature1);
      DEBUG_PRINT(" - Voltage: ");
      DEBUG_PRINTLN(voltage);
  
      DEBUG_PRINTLN("connecting to ");
      DEBUG_PRINTLN(host);

      // Use WiFiClient class to create TCP connections
      WiFiClient client;
      const int httpPort = 80;
      
      if (!client.connect(host, httpPort)) {
        DEBUG_PRINTLN("connection failed");
        if (millis()-startMillis > 20000) {
          DEBUG_PRINTLN("Connection timeout");
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
                   "Host: " + host + "\r\n" + 
                   "Connection: close\r\n\r\n");
      delay(500);

      if (voltage < 6.8) {
         DEBUG_PRINTLN("Voltage low, connecting to ");
         DEBUG_PRINTLN(hostIfttt);
         
         if (!client.connect(hostIfttt, httpPort)) {
          DEBUG_PRINTLN("connection failed");
          if (millis()-startMillis > 30000) {
          DEBUG_PRINTLN("Connection timeout");
          ESP.deepSleep(10 * 60 * 1000000);
         }
         return;
        }

        // construct the POST request
        char post_rqst[256];    // hand-calculated to be big enough

        char *p = post_rqst;
        p = append_str(p, "POST /trigger/");
        p = append_str(p, "battery_low");
        p = append_str(p, "/with/key/");
        p = append_str(p, IFTT_KEY);
        p = append_str(p, " HTTP/1.1\r\n");
        p = append_str(p, "Host: maker.ifttt.com\r\n");
        p = append_str(p, "Content-Type: application/json\r\n");
        p = append_str(p, "Content-Length: ");

        // we need to remember where the content length will go, which is:
        char *content_length_here = p;

        // it's always two digits, so reserve space for them (the NN)
        p = append_str(p, "NN\r\n");

        // end of headers
        p = append_str(p, "\r\n");

        // construct the JSON; remember where we started so we will know len
        char *json_start = p;

        // As described - this example reports a pin, uptime, and "hello world"
        p = append_str(p, "{\"value1\":\"");
        p = append_ul(p, millis());
        p = append_str(p, "\",\"value2\":\"");
        p = append_ul(p, temperature);
        p = append_str(p, "\",\"value3\":\"");
        p = append_ul(p, voltage);
        p = append_str(p, "\"}");

        // go back and fill in the JSON length
        // we just know this is at most 2 digits (and need to fill in both)
        int i = strlen(json_start);
        content_length_here[0] = '0' + (i/10);
        content_length_here[1] = '0' + (i%10);

        // finally we are ready to send the POST to the server!
        DEBUG_PRINTLN(post_rqst);
        client.print(post_rqst);
        client.stop();
        delay(500);
      }
      
      DEBUG_PRINTLN("Go to sleep");
      ESP.deepSleep(10 * 60 * 1000000);
      delay(100);
      delay(2000);
    }
