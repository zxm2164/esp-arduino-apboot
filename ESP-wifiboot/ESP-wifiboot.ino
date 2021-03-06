#include "ESP8266WiFi.h"
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <EEPROM.h>

MDNSResponder mdns;
WiFiServer server(80);

const char* ssid = "Cloud Setup";
String st;
bool esidStored;
int webtype;

int WifiStatus = WL_IDLE_STATUS;
IPAddress ip; 

//This function simply checks that a Wifi network has been established
int testWifi(void) {
  int c = 0;
  Serial.println("Waiting for Wifi to connect");  
  Serial.print("Wifi Status: ");
  while ( c < 20 ) {
    //if connection is successful, return "20" for success and exit function
    if (WiFi.status() == WL_CONNECTED) {
      digitalWrite(0, HIGH);
      return(20);
    } 
    delay(500);
    Serial.print(WiFi.status());    
    c++;
  }
  Serial.println("Connect timed out, opening AP");
  digitalWrite(0, LOW);
  return(10);
} 

//Manage HTTP requests
int mdns1()
{
  // Check for any mDNS queries and send responses
  //mdns.update();
  
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    //Serial.println("No client connected, running the mdns function");
    return(20);
  }
  Serial.println("");
  Serial.println("New client");

  // Wait for data from client to become available
  while(client.connected() && !client.available()){
    delay(1);
   }
  
  // Read the first line of HTTP request
  String req = client.readStringUntil('\r');
  
  // First line of HTTP request looks like "GET /path HTTP/1.1"
  // Retrieve the "/path" part by finding the spaces
  int addr_start = req.indexOf(' ');
  int addr_end = req.indexOf(' ', addr_start + 1);
  if (addr_start == -1 || addr_end == -1) {
    Serial.print("Invalid request: ");
    Serial.println(req);
    return(20);
   }
  req = req.substring(addr_start + 1, addr_end);
  Serial.print("Request: ");
  Serial.println(req);
  client.flush(); 
  String s;
  if ( webtype == 0 ) {
      if (req == "/")
      {
        IPAddress ip = WiFi.softAPIP();
        String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
        s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 at ";
        s += ipStr;
        s += "<p>";
        s += st;
        s += "<form method='get' action='a'><label>SSID: </label><input name='ssid' length=32><input name='pass' length=64><input type='submit'></form>";
        s += "</html>\r\n\r\n";
        Serial.println("Sending 200");
      }
      else if ( req.startsWith("/ssid=") ) {
        // /a?ssid=blahhhh&pass=poooo
        Serial.println("clearing eeprom");
        for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
        String qsid; 
        qsid = req.substring(6,req.indexOf('&'));
        Serial.println(qsid);
        Serial.println("");
        String qpass;
        qpass = req.substring(req.lastIndexOf('=')+1);
        Serial.println(qpass);
        Serial.println("");
        
        Serial.println("writing eeprom ssid:");
        for (int i = 0; i < qsid.length(); ++i)
          {
            EEPROM.write(i, qsid[i]);
            Serial.print("Wrote: ");
            Serial.println(qsid[i]); 
          }
        Serial.println("writing eeprom pass:"); 
        for (int i = 0; i < qpass.length(); ++i)
          {
            EEPROM.write(32+i, qpass[i]);
            Serial.print("Wrote: ");
            Serial.println(qpass[i]); 
          }    
        EEPROM.commit();
        esidStored = true;
        
        s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 ";
        s += "Found ";
        s += req;
        s += "<p> saved to eeprom... reset to boot into new wifi</html>\r\n\r\n";
      }
      else
      {
        s = "HTTP/1.1 404 Not Found\r\n\r\n";
        Serial.println("Sending 404");
      }
  } 
  else
  {
      if (req == "/")
      {
        s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Hello from ESP8266";
        s += "<p>";
        s += "</html>\r\n\r\n";
        Serial.println("Sending 200");
      }
      else if ( req.startsWith("/cleareeprom") ) {
        s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Hello from ESP8266";
        s += "<p>Clearing the EEPROM<p>";
        s += "</html>\r\n\r\n";
        Serial.println("Sending 200");  
        Serial.println("clearing eeprom");
        for (int i = 0; i < 96; i++) { EEPROM.write(i, 0); }
        EEPROM.commit();
        esidStored = false;
        WiFi.disconnect();      //after EEPROM is cleared, disconnect from the current wifi access
        setup();
      }
      else
      {
        s = "HTTP/1.1 404 Not Found\r\n\r\n";
        Serial.println("Sending 404");
      }       
  }
  client.print(s);
  Serial.println("Done with client");
  return(20);
}

//webtype 0 - AP mode, webtype 1 - client mode
//MDNS only works for Client mode at this time!
void launchWebServer() {
          Serial.println("");
          Serial.println("Starting Web Server...");
          // Start the server
          //AP Mode
          if (webtype == 0){
              Serial.println("AP IP Address: ");
              Serial.println(WiFi.softAPIP());
            
          }
          //Client mode
          else if (webtype == 1){
                Serial.println("Client IP Address ");
                Serial.println(WiFi.localIP()); 
                if (!mdns.begin("Cloud", WiFi.localIP())) {
                  Serial.println("Error setting up AP MDNS responder!");
                  while(1) { 
                    delay(1000);
                  }
              }
                Serial.println("mDNS responder started");

               
          }
          
          server.begin();
          Serial.println("Server started");
          digitalWrite(0, HIGH);

          //call mdns1 once, then exit to main loop and call when it is necessary
          mdns1();
//          int b = 20;       //local variable which detects if a client is connected
//          int c = 0;
//          while(b == 20) { 
//             b = mdns1(webtype);
//           }
}


//This function searches for networks and stores them in the WiFi struct.  Called when 
//stored SSID connection fails or if there is no stored SSID
void setupAP(void) {
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
     {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*");
      delay(10);
     }
  }
  Serial.println(""); 
  st = "<ul>";
  for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      st += "<li>";
      st +=i + 1;
      st += ": ";
      st += WiFi.SSID(i);
      st += " (";
      st += WiFi.RSSI(i);
      st += ")";
      st += (WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*";
      st += "</li>";
    }
  st += "</ul>";
  delay(100);
  //create an AP with SSID as declared in the global vars
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid);
  Serial.println("Soft AP Mode");
  Serial.print("SSID: ");
  Serial.println(ssid);
  webtype = 0;
  launchWebServer();   //Call launch web, but let it know that this is in AP mode
  Serial.println("over");
}







void setup() {

  //Setup LED Diagnostic
  pinMode(0, OUTPUT);

  
  Serial.begin(115200);
  EEPROM.begin(512);
  delay(10);
  Serial.println();
  Serial.println();
  Serial.println("Startup");
  // read eeprom for ssid and pass
  Serial.println("Reading EEPROM ssid");
  String esid;              //string which contains the SSID stored within EEPROM
  for (int i = 0; i < 32; ++i)
    {
      esid += char(EEPROM.read(i));
    }
  Serial.print("SSID: ");
  if (esidStored) {
    Serial.println(esid);
  }
  else{
    Serial.println("None Stored");
  }
  
  Serial.println("Reading EEPROM pass");
  String epass = "";
  for (int i = 32; i < 96; ++i)
    {
      epass += char(EEPROM.read(i));
    }
  Serial.print("PASS: ");
  if (esidStored) {
    Serial.println(epass);
  }
  else{
    Serial.println("None Stored");
  }

  //After reading the stored "esid", if it is valid,
  //try to connect to it.  Else, create an AP and search for networks
  if ( esid.length() > 1 ) {
      Serial.println("Stored SSID length greater than 1");
      // test stored esid
      WiFi.begin(esid.c_str(), epass.c_str());
      Serial.println("Starting connection with ");
      Serial.print("ESID ");
      Serial.println(esid);
      Serial.print("PASS ");
      Serial.println(epass);
      if ( testWifi() == 20 ) { 
          Serial.println("");
          Serial.println("Client Connection Successful");
          webtype = 1;
          launchWebServer();
          return;
      }
  }
  //No stored or failed wireless connection.  Create AP to set up wireless
  Serial.println("SSID did not exist, creating an AP");
  setupAP(); 
}



void loop() {

  mdns1();

}
