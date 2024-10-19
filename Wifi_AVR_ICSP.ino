#include <SPI.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266AVRISP.h>       // OVERWRITE native lib on Arduino core with with included in project (added programming state in section)
#include <Adafruit_SSD1306.h>    // OVERWRITE native lib on Arduino core with with included in project (added oled brightness control support)
#include <Adafruit_GFX.h>


// DEFINE HERE THE KNOWN NETWORKS
const char* KNOWN_SSID[] = {"SSID-1", "SSID-2"};
const char* KNOWN_PASSWORD[] = {"Password-SSID-2", "Password-SSID-2"};

const int   KNOWN_SSID_COUNT = sizeof(KNOWN_SSID) / sizeof(KNOWN_SSID[0]); // number of known networks

const char* host = "AVR-ICSP";
const uint16_t port = 328;
const uint8_t reset_pin = 2;
uint16_t timer = 0;

#define OLED_RESET -1  // GPIO0
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define SCREEN_ADDRESS 0x3C // See datasheet for Address; 0x3D or 0x3C


ESP8266AVRISP avrprog(port, reset_pin);

Adafruit_SSD1306 OLED(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  OLED.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  OLED.clearDisplay();
 
  //Add stuff into the 'display buffer'
  OLED.setTextWrap(false);
  OLED.setTextSize(1);
  OLED.setTextColor(WHITE);
  OLED.setCursor(0,0);
  OLED.println("[BOOT] ok "); OLED.display();
  delay(500);

  //OLED.fillRect(0, 0, 128, 8, BLACK);
  OLED.println("[UART] init "); OLED.display();
  delay(500);
  Serial.begin(115200);
  Serial.println("");
  Serial.println("Arduino AVR-ISP over TCP");
  avrprog.setReset(false); // let the AVR run

  boolean wifiFound = false;
  int i, n;

  Serial.begin(115200);

  // ----------------------------------------------------------------
  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  // ----------------------------------------------------------------
  //OLED.fillRect(0, 0, 128, 8, BLACK);
  OLED.println("[WIFI] init "); OLED.display();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  Serial.println("Setup done");

  // ----------------------------------------------------------------
  // WiFi.scanNetworks will return the number of networks found
  // ----------------------------------------------------------------
  Serial.println(F("scan start"));
  //OLED.fillRect(0, 0, 128, 8, BLACK);
  OLED.println("[WIFI] scan "); OLED.display();
  int nbVisibleNetworks = WiFi.scanNetworks();
  Serial.println(F("scan done"));
  if (nbVisibleNetworks == 0) {
      OLED.fillRect(0, 0, 128, 8, BLACK);
      OLED.println("[WIFI] error "); OLED.display();
    Serial.println(F("no networks found. Reset to try again"));
    while (true); // no need to go further, hang in there, will auto launch the Soft WDT reset
  }

  // ----------------------------------------------------------------
  // if you arrive here at least some networks are visible
  // ----------------------------------------------------------------
  Serial.print(nbVisibleNetworks);
  Serial.println(" network(s) found");

  // ----------------------------------------------------------------
  // check if we recognize one by comparing the visible networks
  // one by one with our list of known networks
  // ----------------------------------------------------------------
  for (i = 0; i < nbVisibleNetworks; ++i) {
    Serial.println(WiFi.SSID(i)); // Print current SSID
    for (n = 0; n < KNOWN_SSID_COUNT; n++) { // walk through the list of known SSID and check for a match
      if (strcmp(KNOWN_SSID[n], WiFi.SSID(i).c_str())) {
        Serial.print(F("\tNot matching "));
        Serial.println(KNOWN_SSID[n]);
      } else { // we got a match
        wifiFound = true;
        break; // n is the network index we found
      }
    } // end for each known wifi SSID
    if (wifiFound) break; // break from the "for each visible network" loop
  } // end for each visible network

  if (!wifiFound) {
    Serial.println(F("no Known network identified. Reset to try again"));
    //OLED.fillRect(0, 0, 128, 8, BLACK);
    OLED.println("[WIFI] not found "); OLED.display();

    while (true); // no need to go further, hang in there, will auto launch the Soft WDT reset
  }

  // ----------------------------------------------------------------
  // if you arrive here you found 1 known SSID
  // ----------------------------------------------------------------
  Serial.print(F("\nConnecting to "));
  Serial.println(KNOWN_SSID[n]);
  //OLED.fillRect(0, 0, 128, 8, BLACK);
  OLED.println("[WIFI] connecting "); OLED.display();

  // ----------------------------------------------------------------
  // We try to connect to the WiFi network we found
  // ----------------------------------------------------------------
  WiFi.begin(KNOWN_SSID[n], KNOWN_PASSWORD[n]);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");

  // ----------------------------------------------------------------
  // SUCCESS, you are connected to the known WiFi network
  // ----------------------------------------------------------------
  OLED.println("[WIFI] connected "); OLED.display(); delay(500);
  OLED.clearDisplay(); OLED.display(); OLED.setCursor(0,32);
  MDNS.begin(host);
  MDNS.addService("avrisp", "tcp", port);

  IPAddress local_ip = WiFi.localIP();
  Serial.print("IP address: ");
  Serial.println(local_ip);
  OLED.print("[ IP ] ");
  OLED.println(local_ip);
  OLED.print("[PORT] ");
  OLED.println(port);
  OLED.print("[SSID] ");
  OLED.println(KNOWN_SSID[n]);
  OLED.display(); //output 'display buffer' to screen  
  Serial.println("Use your avrdude:");
  Serial.print("avrdude -b 230400 -c arduino -p <device> -P net:");
  Serial.print(local_ip);
  Serial.print(":");
  Serial.print(port);
  Serial.println(" -t # or -U ...");
  OLED.fillRect(0, 0, 128, 8, BLACK); //OLED.display();
  OLED.setCursor(0,0);  OLED.print("[ICSP] ready"); OLED.display();
  delay(1000);
  OLED.setBrightness(64); // THIS WORKS ONLY WITH MODDED LIB

  // listen for avrdudes
  avrprog.begin();
}

void loop() {
  static AVRISPState_t last_state = AVRISP_STATE_IDLE;
  AVRISPState_t new_state = avrprog.update();
  
  if ((millis()/1000 - timer) > 1){
  timer = millis()/1000;
  
  OLED.fillRect(0, 56, 128, 8, BLACK); //OLED.display();
  OLED.setCursor(0,56);  OLED.print("[RSSI] "); OLED.print(WiFi.RSSI());
  OLED.display();
            
  if (last_state != new_state) {
    switch (new_state) {
      case AVRISP_STATE_IDLE: {
          Serial.printf("[AVRISP] now idle\r\n");
            OLED.fillRect(0, 16, 128, 8, BLACK); //OLED.display();
            OLED.setCursor(0,16);  OLED.print("[LINK] idle"); 
            OLED.fillRect(0, 0, 128, 8, BLACK); //OLED.display();
            OLED.setCursor(0,0);  OLED.print("[ICSP] ready");
            OLED.display();
          // Use the SPI bus for other purposes
          break;
        }
      case AVRISP_STATE_PENDING: {
          Serial.printf("[AVRISP] connection pending\r\n");
            OLED.fillRect(0, 16, 128, 8, BLACK); //OLED.display();
            OLED.setCursor(0,16);  OLED.print("[LINK] connecting"); 
            OLED.fillRect(0, 0, 128, 8, BLACK); //OLED.display();
            OLED.setCursor(0,0);  OLED.print("[ICSP] ready");
            OLED.display();
          // Clean up your other purposes and prepare for programming mode
          break;
        }
      case AVRISP_STATE_ACTIVE: {
          Serial.printf("[AVRISP] programming mode\r\n");
            OLED.fillRect(0, 16, 128, 8, BLACK); //OLED.display();
            OLED.setCursor(0,16);  OLED.print("[LINK] connected"); 
            OLED.fillRect(0, 0, 128, 8, BLACK); //OLED.display();
            OLED.setCursor(0,0);  OLED.print("[ICSP] ready");
            OLED.display();
          // Stand by for completion
          break;
        }
   //------------------------------ THIS SECTION WORKS ONLY WITH MODDED LIB -----------------------//     
      case AVRISP_STATE_PROG: {
          Serial.printf("[AVRISP] programming now\r\n");
            OLED.fillRect(0, 0, 128, 8, BLACK); //OLED.display();
            OLED.setCursor(0,0);  OLED.print("[ICSP] programming"); 
            OLED.display();
          // Stand by for completion
          break;
        }
   //------------------------------ THIS SECTION WORKS ONLY WITH MODDED LIB -----------------------//
    }
    last_state = new_state;
  }
 }
  // Serve the client
  if (last_state != AVRISP_STATE_IDLE) {
    avrprog.serve();
  }
}
