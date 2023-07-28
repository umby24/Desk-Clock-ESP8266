#include <EEPROM.h> // -- Load and save wifi info to eeprom
#include <NTP.h> // -- get time from NTP services
#include <ESP8266WiFi.h> // -- connect to wifi
#include <ESP8266WebServer.h> // -- run a web server to reconfigure
#include <WiFiUdp.h> // -- Dependency of ntp 
#include <ArduinoOTA.h> // -- Over the air code update
#include <Adafruit_GFX.h>  // -- Display libraries
#include <Adafruit_ST7735.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

#include <JSON_Decoder.h> // -- Weather information
#include <OpenWeather.h>

#define TFT_CS         4  // 4
#define TFT_RST        16  // 16                                            
#define TFT_DC         5  // 5
#define ESP8266
#define TIME_OFFSET 1UL * 3600UL // UTC + 0 hour

#ifndef STASSID
#define FBSSID "ClockFallback"
#define FBPSK "fallbackclock"
#endif

// -- A sprite of the wifi icon. 48x48 pixels.
const unsigned char pix [] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xfe, 0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 0xc0, 0x00, 
	0x00, 0x1f, 0xff, 0xff, 0xf8, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xfe, 0x00, 0x01, 0xff, 0xff, 0xff, 
	0xff, 0x80, 0x03, 0xff, 0xfc, 0x3f, 0xff, 0xc0, 0x0f, 0xff, 0x00, 0x00, 0xff, 0xf0, 0x1f, 0xfc, 
	0x00, 0x00, 0x3f, 0xf8, 0x3f, 0xe0, 0x00, 0x00, 0x07, 0xfc, 0x7f, 0xc0, 0x00, 0x00, 0x03, 0xfe, 
	0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xfe, 0x00, 0x3f, 0xfc, 0x00, 0x7f, 0xfc, 0x01, 0xff, 0xff, 
	0x80, 0x3f, 0x78, 0x07, 0xff, 0xff, 0xe0, 0x1e, 0x00, 0x0f, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x3f, 
	0xff, 0xff, 0xfc, 0x00, 0x00, 0x7f, 0xf8, 0x1f, 0xfe, 0x00, 0x00, 0xff, 0xc0, 0x03, 0xff, 0x00, 
	0x00, 0xff, 0x00, 0x00, 0xff, 0x00, 0x00, 0xfc, 0x00, 0x00, 0x3f, 0x00, 0x00, 0xf8, 0x03, 0xc0, 
	0x1f, 0x00, 0x00, 0x70, 0x1f, 0xf8, 0x0e, 0x00, 0x00, 0x00, 0x7f, 0xfe, 0x00, 0x00, 0x00, 0x00, 
	0xff, 0xff, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0x80, 0x00, 0x00, 0x01, 0xff, 0xff, 0x80, 0x00, 
	0x00, 0x01, 0xf8, 0x1f, 0x80, 0x00, 0x00, 0x01, 0xf0, 0x0f, 0x80, 0x00, 0x00, 0x00, 0xe0, 0x07, 
	0x00, 0x00, 0x00, 0x00, 0x07, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x07, 0xe0, 0x00, 0x00, 0x00, 0x00, 
	0x0f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xf0, 0x00, 0x00, 
	0x00, 0x00, 0x07, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x03, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// -- Used for the access point config webpage
const char HTTP_CONFIG_PAGE[] = {"<!DOCTYPE html>\n<html>\n<head>\n<title>Config</title>\n</head>\n<body>\n<h1>Wi-Fi Config</h1>\n<form action=\"/\" method='POST'>\n<label>SSID:<input type='text' name='ssid'/></label><br/>\n<label>Password:<input type='password' name='password'/></label><br>\n<input type='submit' value='Save'/></form>\n"};
const char HTTP_SAVED_LABEL[] = {"<label style='color:red;font-style:bold;'>Saved! Rebooting.</label><br/>"};
const char HTTP_CONFIG_END[] = {"</body></html>"};
// -- Open weather config
const String api_key = "xxxxxxxxxxxxxxxxxxxx"; // -- Fill in with an open-weather api key. They're free.
const String latitude = "30.708011";
const String longitude = "-97.851886";
const String units = "imperial";
const String language = "en";

// -- Backup access point configuration.
IPAddress AP_IP = IPAddress(10,1,1,1); // -- IP of this device on the backup AP
IPAddress AP_subnet = IPAddress(255,255,255,0); // -- Subnet mask of the backup

WiFiUDP ntpUDP;
NTP timeClient(ntpUDP);
OW_Weather ow;
OW_forecast *forecast = new OW_forecast;

// -- EEPROM saved wifi information struct
struct WifiConf {
  char wifi_ssid[50];
  char wifi_password[50];
  char cstr_terminator = 0;
};

WifiConf wifiConf;

const char* fbssid = FBSSID;
const char* fbpw = FBPSK;
bool wifiConnected = false;

String CURRENT_TIME = "";
String CURRENT_TEMP = "";

ESP8266WebServer server(80);
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");
  // -- Setup our screen
  tft.init(240, 240);
  tft.setRotation(3); // -- Rotate 90 degrees
  tft.fillScreen(ST7735_BLACK);

  writeStatusText("Connecting...", ST7735_WHITE);

  EEPROM.begin(512); // -- Load wifi config from EEPROM
  readWifiConf();

  if (!connectToWifi()) {
    Serial.println("Connection to wifi failed, falling back.");
    setupBackupAP();
    wifiConnected = false;
    writeErrorText("No Wifi :(\n\nΘ");
  } else {
    Serial.println("Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    wifiConnected = true;
    timeClient.ruleDST("CDT", 2, 0, 2, 2, -300); // (-6 GMT, +1 Hour summer offset, -5*60)
    timeClient.ruleSTD("CST", First, Sun, Nov, 4, -360); // -- -6 GMT.
    timeClient.begin();
  }

  ArduinoOTA.setHostname("UnknownClock");
  setupWebServer();
  setupOnAirProgramming();
}

void writeTime(const char* text, const char* ampm) {
  tft.setCursor(10, 40);
  tft.setTextSize(6);
  tft.setTextColor(ST7735_WHITE);
  tft.print(text);

  tft.setCursor(200, 40);
  tft.setTextSize(3);
  tft.setTextColor(ST7735_GREEN);
  tft.print(ampm);
}

void writeDate(const char* inDay, const char* inMonth, const char* inYear) {
  tft.setCursor(10, 100);
  tft.setTextSize(3);
  tft.setTextColor(ST7735_WHITE);
  tft.print(inDay);

  tft.setCursor(10, 125);
  tft.setTextSize(3);
  tft.setTextColor(ST7735_WHITE);
  tft.print(inMonth);

  tft.setCursor(10, 155);
  tft.setTextSize(5);
  tft.setTextColor(ST7735_WHITE);
  tft.print(inYear);
}

void displayWeatherInfo() {
  if ((millis() % (5 * 60 * 1000)) == 0 || forecast->temp[0] == 0.00) {
    ow.getForecast(forecast, api_key, latitude, longitude, units, language, false);
  }
  if (forecast) {
      tft.setCursor(170, 140);
      tft.setTextSize(3);
      tft.setTextColor(ST7735_WHITE);
      tft.print((int)forecast->temp[0]);
      tft.setTextSize(2);
      tft.print("\x09");
      tft.setCursor(170, 170);
      tft.setTextSize(4);
      tft.setTextColor(ST7735_RED);
      tft.print("F");
  } else {
    tft.setCursor(170, 140);
    tft.setTextSize(2);
    tft.setTextColor(ST7735_BLUE);
    tft.print("Loading");
  }
}

void displayWifiIcon(uint16_t color) {
  tft.drawBitmap(170, 90, pix, 48, 48, color);
}

void writeStatusText(char *text, uint16_t color) {
  tft.setCursor(10, 100);
  tft.setTextSize(3);
  tft.setTextColor(color);
  tft.setTextWrap(false);
  tft.print(text);
}

void writeErrorText(char *text) {
  tft.setCursor(10, 100);
  tft.setTextSize(3);
  tft.setTextColor(ST7735_RED);
  tft.setTextWrap(true);
  tft.print(text);
}

void setupWebServer() {
  server.on("/", handleWebConnection);
  server.begin();
  Serial.println("Web server running.");
}

// -- Method to handle an incoming web request.
void handleWebConnection() {
  bool save = false;

  if (server.hasArg("ssid") && server.hasArg("password")) {
    server.arg("ssid").toCharArray(wifiConf.wifi_ssid, sizeof(wifiConf.wifi_ssid));
    server.arg("password").toCharArray(wifiConf.wifi_password, sizeof(wifiConf.wifi_password));
    save = true;
    writeWifiConf();
  }

  String message = String(HTTP_CONFIG_PAGE);
  if (save) {
    message += String(HTTP_SAVED_LABEL);
  }
  message += String(HTTP_CONFIG_END);

  server.send(200, "text/html", message);

  if (save) {
    Serial.println("Rebooting...");
    delay(1500);
    ESP.restart();
  }
}

void readWifiConf() {
  // -- Read EEPROM data one byte at a time into our struct
  for (int i = 0; i < sizeof(wifiConf); i++) {
    ((char*)(&wifiConf))[i] = char(EEPROM.read(i));
  }
  wifiConf.cstr_terminator = 0; // -- In case of garbage data
}

void writeWifiConf() {
  // -- Write out data one byte at a time to the EEPROM
  for (int i = 0; i < sizeof(wifiConf); i++) {
    EEPROM.write(i, ((char*)(&wifiConf))[i]);
  }

  EEPROM.commit();
}

// -- Connect to the wifi AP defined in the struct above.
// -- Returns true if connected, false otherwise.
bool connectToWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiConf.wifi_ssid, wifiConf.wifi_password);
  return WiFi.waitForConnectResult() == WL_CONNECTED;
}

void setupBackupAP() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(AP_IP, AP_IP, AP_subnet);
  if (WiFi.softAP(fbssid, fbpw)) {
    Serial.println("Backup access point live!");
  } else {
    Serial.println("Backup access point failed to go live!");
  }
}

void setupOnAirProgramming() {
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
}

void updateDisplay() {
  char day[4];
  char month[10];
  char year[5];
  char ampm[3];
  char time[6];

  
  timeClient.update(); // -- Refresh our NTP client

  strcpy(day, timeClient.formattedTime("%a")); // -- Copy out a few items (NTP library uses the same pointer for each fmt string)
  strcpy(month, timeClient.formattedTime("%d %B"));
  strcpy(year, timeClient.formattedTime("%Y"));
  strcpy(ampm, timeClient.formattedTime("%p"));
  strcpy(time, timeClient.formattedTime("%I:%M"));

  if (CURRENT_TIME != String(time)) {
    tft.fillScreen(ST7735_BLACK); // -- Clear the screen
    // -- Write our time and date to the display
    writeTime((const char*)&time, (const char*)&ampm);
    writeDate((const char*)&day, (const char*)&month, (const char*)&year);

    if (WiFi.status() != WL_CONNECTED)
      displayWifiIcon(ST7735_RED);
    else {
      displayWifiIcon(ST7735_GREEN);
    }

    displayWeatherInfo();
    CURRENT_TIME = String(time);
  }
  
  //tft.drawRGBBitmap(0, 0, canvas.getBuffer(), canvas.width(), canvas.height());
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  updateDisplay();
}
