

// SDL_ESP32_BC24_AIRQUALITY
// SwitchDoc Labs BC24 Pixel ESP32 Board plus Air Quality Sensor
// June 2018
//
//

#define BC24AIRQUALITYSOFTWAREVERSION "006"
#undef BC24DEBUG



#define BUTTONPIN 17

#define BLINKPIN 13

#define BC24


#include <Wire.h>


#include "utility.h"

#include "BigCircleFunctions.h"

#if defined(ARDUINO) && ARDUINO >= 100
// No extras
#elif defined(ARDUINO) // pre-1.0
// No extras
#elif defined(ESP_PLATFORM)
#include "arduinoish.hpp"
#endif


bool WiFiPresent = false;

#include <time.h>


#include <WiFi.h>



#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#include <TimeLib.h>

#include "Clock.h"

#include <Preferences.h>


#include "NTPClient.h"

WiFiUDP ntpUDP;


NTPClient timeClient(ntpUDP);

/* create an instance of Preferences library */
Preferences preferences;


#include "WiFiManager.h"          //https://github.com/tzapu/WiFiManager

//gets called when WiFiManager enters configuration mode


void configModeCallback (WiFiManager *myWiFiManager)
//void configModeCallback ()
{

  Serial.println("Entered config mode");

  Serial.println(WiFi.softAPIP());

}

#define WEB_SERVER_PORT 80
String APssid;

String Wssid;
String WPassword;



#include "BC24Preferences.h"

#include <esp_wps.h>
#include <esp_smartconfig.h>


#define ESP_WPS_MODE WPS_TYPE_PBC

// Kludge for latest ESP32 SDK - July 1, 2018

#define WPS_CONFIG_INIT_DEFAULT(type) { \
    .wps_type = type, \
    .crypto_funcs = &g_wifi_default_wps_crypto_funcs, \
}


esp_wps_config_t config = WPS_CONFIG_INIT_DEFAULT(ESP_WPS_MODE);

#include "SDL_ESP32_BC24_GETIP.h"

// aREST

#include "MaREST.h"
// Create aREST instance
aREST rest = aREST();

// Create an instance of the server
WiFiServer server(WEB_SERVER_PORT);

// Variables to be exposed to the API
long RESTrawAirQuality;
float RESTnumberAirQuality;

String RESTtextAirQuality;

#include "AirQualityLibrary.h"


// Declare functions to be exposed to the API
int ledControl(String command);

// Air Quality Sensor

int currentAirQuality;
int  AnalogPin = A7;

/* Global variables */
void setup()
{



  char rtn = 0;
  Serial.begin(115200);  // Serial is used for debugging
  delay(1000);

  // set up button Pin
  pinMode (BUTTONPIN, INPUT);
  pinMode(BUTTONPIN, INPUT_PULLUP);  // Pull up to 3.3V on input - some buttons already have this done

  Serial.print("Button=");
  Serial.println(digitalRead(BUTTONPIN));

  // Now check for clearing Preferences on hold down of Mode pin on reboot
  if (digitalRead(BUTTONPIN) == 0)
  {

    resetPreferences();
  }

  // setup preferences
  readPreferences();

  ClockTimeOffsetToUTC = -25200;

  pinMode(BLINKPIN, OUTPUT);

  Serial.println();
  Serial.println();
  Serial.println("--------------------");
  Serial.println("BC24 Air Quality Software");
  Serial.println("--------------------");
  Serial.print("Version: ");
  Serial.println(BC24AIRQUALITYSOFTWAREVERSION);

  Serial.print("Compiled at:");
  Serial.print (__TIME__);
  Serial.print(" ");
  Serial.println(__DATE__);

  Serial.println("\r\npower on");

  Wire.begin();


  Serial.print("AnalogPin=");
  Serial.println(AnalogPin);

  pinMode(AnalogPin, INPUT);
  adcAttachPin(AnalogPin);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);



  // Free heap on ESP32
  Serial.print("Free Heap Space on BC24:");
  Serial.println(ESP.getFreeHeap());

#ifdef BC24DEBUG
  Serial.print("CPU0 reset reason: ");
  print_reset_reason(rtc_get_reset_reason(0));

  Serial.print("CPU1 reset reason: ");
  print_reset_reason(rtc_get_reset_reason(1));
#endif

  // initalize our friend the BC24!
  //BC24inititialzeCircle();

  // setup BC24

  dumpSysInfo();
  getMaxMalloc(1 * 1024, 16 * 1024 * 1024);

  if (digitalLeds_initStrands(STRANDS, STRANDCNT)) {
    Serial.println("Init FAILURE: halting");
    while (true) {};
  }
  for (int i = 0; i < STRANDCNT; i++) {
    strand_t * pStrand = &STRANDS[i];
    Serial.print("Strand ");
    Serial.print(i);
    Serial.print(" = ");
    Serial.print((uint32_t)(pStrand->pixels), HEX);
    Serial.println();
#if DEBUG_ESP32_DIGITAL_LED_LIB
    dumpDebugBuffer(-2, digitalLeds_debugBuffer);
#endif
    digitalLeds_resetPixels(pStrand);
#if DEBUG_ESP32_DIGITAL_LED_LIB
    dumpDebugBuffer(-1, digitalLeds_debugBuffer);
#endif
  }
  Serial.println("BC24 Init complete");

  // Init variables and expose them to REST API

  rest.variable("rawAirQuality", &RESTrawAirQuality);
  rest.variable("numberAirQuality", &RESTnumberAirQuality);
  rest.variable("textAirQuality", &RESTtextAirQuality);


  // Function to be exposed
  rest.function("led", ledControl);

  // Give name & ID to the device (ID should be 6 characters long)
  rest.set_id("000001");
  rest.set_name("SDL_ESP32_BC24_AIRQUALITY");

  //---------------------
  // Setup WiFi Interface
  //---------------------
  WiFiPresent = false;

  WiFi.begin();
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  // check for SSID



  if (WiFi_SSID.length() != 0)
  {
    // use existing SSID
    Serial.println("Using existing SSID/psk");

    Serial.printf("SSID="); Serial.println(WiFi_SSID);
    Serial.printf("psk="); Serial.println(WiFi_psk);
    WiFi.begin(WiFi_SSID.c_str(), WiFi_psk.c_str());
    //Wait for WiFi to connect to AP
    Serial.println("Waiting for Saved WiFi");
#define WAITFORCONNECT 10
    for (int i = 0; i < WAITFORCONNECT * 2; i++)
    {
      if (WiFi.status() == WL_CONNECTED)
      {

        Serial.println("");
        Serial.println("WiFI Connected.");
        WiFiPresent = true;
#ifdef BC24
        BC24ThreeBlink(Green, 1000);
#endif

        break;
      }

      Serial.print(".");
      WiFiPresent = false;
      BC24ThreeBlink(White, 1000);
    }
    Serial.println();

  }


  if (WiFiPresent == false)
  {
    // do SmartConfig
#define WAITFORSTART 15
#define WAITFORCONNECT 20

    WiFiPresent = SmartConfigGetIP(WAITFORSTART, WAITFORCONNECT);

  } // if not already programmed before





  if (WiFiPresent != true)
  {
#define WPSTIMEOUTSECONDS 60
    // now try WPS Button

    WiFiPresent = WPSGetIP(WPSTIMEOUTSECONDS);

  }

  if (WiFiPresent != true)
  {
#define APTIMEOUTSECONDS 60
    WiFiPresent = localAPGetIP(APTIMEOUTSECONDS);
  }


  if (WiFiPresent == true)
  {


    Serial.println("-------------");
    Serial.println("WiFi Connected");
    Serial.println("-------------");
    WiFi_SSID = WiFi.SSID();
    WiFi_psk = WiFi.psk();
    Serial.print("SSID=");
    Serial.println(WiFi_SSID);

    Serial.print("psk=");
    Serial.println(WiFi_psk);
  }
  else
  {
    Serial.println("-------------");
    Serial.println("WiFi NOT Connected");
    Serial.println("-------------");
  }






  if (WiFiPresent == true)
  {

    WiFi_SSID = WiFi.SSID();
    WiFi_psk = WiFi.psk();
  }
  else
  {
    Serial.println("No Wifi Present");
    BC24ThreeBlink(Red, 1000);
  }

  // write out preferences

  writePreferences();
  if (WiFiPresent == true)
  {
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    Serial.print("WiFi Channel= ");
    Serial.println(WiFi.channel());
  }

  //---------------------
  // End Setup WiFi Interface
  //---------------------


  if (WiFiPresent == true)
  {
    // REST server

    // Start the server
    server.begin();
    Serial.println("Server started");

    // Print the IP address
    Serial.println(WiFi.localIP());

    timeClient.setTimeOffset(ClockTimeOffsetToUTC);
    timeClient.setUpdateInterval(3600000);
    timeClient.update();


    time_t t = timeClient.getEpochTime();

    setTime(t);


    digitalClockDisplay();
  }




  Serial.print("RAM left: ");
  Serial.println(system_get_free_heap_size());

  blinkLED(2, 300);  // blink twice - OK!


}

long loopCount = 0;


void handleRESTCalls()
{

  if (WiFiPresent == true)
  {

    // Handle REST calls
    WiFiClient client = server.available();

    /*
      while (!client.available()) {
      delay(1);
      }
    */
    if (client.available())
    {
      // update air quality numbers
      RESTrawAirQuality = getRawAirQuality(AnalogPin);
      RESTnumberAirQuality = getAirQuality(AnalogPin);

      RESTtextAirQuality = AirQualityTextFromRaw(RESTrawAirQuality);


      rest.handle(client);
    }
  }

}


void loop()
{

  handleRESTCalls();


  //if ((loopCount % 1000) == 0)
  if ((loopCount % 10) == 0)
    printValues();

  if ((loopCount % 50) == 0)
    blinkLED(2, 300);  // blink twice - I'm still here!

    // display air quality
    
     strand_t * pStrand = &STRANDS[0];

      switch (getAirQuality(AnalogPin))
      {

        case 4:  // Very bad

          BC24setStrip(pStrand, DarkRed );

          break;

        case 3:  // High polution
          BC24setStrip(pStrand, Red );
          break;
        case 2:  // Medium Pollution

          BC24setStrip(pStrand, DarkYellow );
          break;

        case 1:
          BC24setStrip(pStrand, DarkGreen );
          break;

        case 0:
          BC24setStrip(pStrand, Green );
          break;

        default:
          BC24setStrip(pStrand, White );
          break;

      }

      //vTaskDelay(10000 / portTICK_PERIOD_MS);

      //BC24clearStrip(pStrand);
      vTaskDelay(10 / portTICK_PERIOD_MS);



  loopCount++;




  delay(100);  // delay for serial readability
}

void printValues()
{
  char report[80];
  
  Serial.println( "------------------------------");
  Serial.println( "AirQuality Sensor");
  Serial.println( "------------------------------");

  long currentAirQuality;
  String currentAirQualityText;
  long currentRawAirQuality;

  currentRawAirQuality = getRawAirQuality(AnalogPin);
  currentAirQuality = getAirQuality(AnalogPin);
  currentAirQualityText = AirQualityTextFromRaw(currentRawAirQuality);


  Serial.print("currentAirQuality=");
  Serial.println(currentAirQuality);
  Serial.print("rawAirQuality=");
  Serial.println(getRawAirQuality(AnalogPin));
  Serial.print("textAirQuality=");
  Serial.println(currentAirQualityText);
  Serial.println("------------------------------");
}

// Custom function accessible by the API
int ledControl(String command) {

  // Get state from command
  int state = command.toInt();

  digitalWrite(BLINKPIN, state);
  return 1;
}
