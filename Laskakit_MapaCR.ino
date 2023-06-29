/*
* Firmware pro LaskaKit Mapu ČR
* -----------------------------
* Autor: Ondřej Kotas, KRtkovo.eu
* Verze: 0.2
* https://github.com/KRtekTM/Laskakit_MapaCR
*
* Postaveno na základě kódu od Jakuba Čížka: https://github.com/jakubcizek/pojdmeprogramovatelektroniku/tree/master/SrazkovyRadar
*/

#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

// Customizable variables
#define DEBUG false
#include "WiFi_Config.h"
const char *hostname = "laskakitmapa";
uint32_t t, last24HourTaskTime = 0;  // Refreshovaci timestamp
uint32_t delay10 = 30000;            // Prodleva mezi aktualizaci dat, 30 vterin
uint32_t startupDelay = 10000;       // Zobraz vlajku na 10 vterin
uint8_t jas = 5;                     // Vychozi jas

// URL endpoints
const char *urlFlag = "https://raw.githubusercontent.com/KRtekTM/Laskakit_MapaCR/master/static/vlajkaCR.json";
const char *urlRain = "http://oracle-ams.kloboukuv.cloud/radarmapa/?chcu=posledni.json";
const char *urlTemp = "http://cdn.tmep.cz/app/export/okresy-cr-teplota.json";
const char *urlCitiesMajor = "https://raw.githubusercontent.com/KRtekTM/Laskakit_MapaCR/master/static/krajskaMesta.json";
const char *urlRegions = "https://raw.githubusercontent.com/KRtekTM/Laskakit_MapaCR/master/static/kraje.json";
const char *urlHumid = "http://cdn.tmep.cz/app/export/okresy-cr-vlhkost.json";
const char *urlPressure = "http://cdn.tmep.cz/app/export/okresy-cr-tlak.json";
const char *urlDust = "http://cdn.tmep.cz/app/export/okresy-cr-prasnost.json";

// Objekt pro ovladani adresovatelnych RGB LED
// Je jich 72 a jsou v serii pripojene na GPIO pin 25
Adafruit_NeoPixel pixely(72, 25, NEO_GRB + NEO_KHZ800);
// HTTP server bezici na standardnim TCP portu 80
WebServer server(80);
// Pamet pro JSON s povely
// Alokujeme pro nej 10 000 B, co je hodne,
// ale melo by to stacit i pro jSON,
// ktery bude obsahovat instrukce pro vsech 72 RGB LED
StaticJsonDocument<10000> doc;


// TMEP city ID mapping to correct LED id
int XX = -1;
int LEDsTMEP[77] = {
  24, 19, 16, 10, 15, 9, 6, 8, 3,
  0, 4, 1, 2, 5, 11, 7, 12, 18, 21,
  29, 27, 34, 38, 31, 17, 25, 45, 30, 36,
  35, 42, 44, 56, 48, 61, 57, 65, 68, 59,
  49, 55, 63, XX, 71, 69, 62, 47, 53, 46,
  51, 64, 52, 67, 70, 66, 60, 58, 54, 50,
  37, XX, 40, XX, 41, 23, 22, 14, 13, 20,
  28, 33, 39, 43, 32, XX, XX, 26
};


// Map selector
float maxThreshold, minThreshold;
enum SelectedMap {
  MapRain,
  MapTemp,
  MapFlag,
  MapCitiesMajor,
  MapRegions,
  MapHumid,
  MapPressure,
  MapDust
};
SelectedMap currentMap;
bool currentMapTMEP = false;
bool firstRun = true;

// Dekoder JSONu a rozsvecovac svetylek
int jsonDecoder(String s, bool log) {
  DeserializationError e = deserializeJson(doc, s);
  if (e) {
    if (e == DeserializationError::InvalidInput) {
      return -1;
    } else if (e == DeserializationError::NoMemory) {
      return -2;
    } else {
      return -3;
    }
  } else {
    if (!(firstRun && currentMapTMEP)) pixely.clear();

    // Temperature map is in different format which needs remaping locations, follow else path
    if (!currentMapTMEP && (!(firstRun && currentMapTMEP))) {
      JsonArray mesta = doc["seznam"].as<JsonArray>();
      for (JsonObject mesto : mesta) {
        int id = mesto["id"];
        int r = mesto["r"];
        int g = mesto["g"];
        int b = mesto["b"];
        if (log) Serial.printf("Rozsvecuji mesto %d barvou R=%d G=%d B=%d\r\n", id, r, g, b);
        pixely.setPixelColor(id, pixely.Color(r, g, b));
      }
    } else {
      if (log) Serial.printf("minThreshold %f and maxThreshold %f\r\n", minThreshold, maxThreshold);

      // Read all TMEP districts with their indexes
      for (JsonObject item : doc.as<JsonArray>()) {
        int TMEPdistrictIndex = item["id"];
        // Substract 1, so index will start from 0
        TMEPdistrictIndex -= 1;
        float tempCelsius = item["h"];

        // Adjust the range of temperatures
        if (tempCelsius < minThreshold) minThreshold = tempCelsius;
        if (tempCelsius > maxThreshold) maxThreshold = tempCelsius;
        if (log) Serial.printf("minThreshold %f and maxThreshold %f\r\n", minThreshold, maxThreshold);

        if (log) Serial.printf("Mesto %d ma teplotu %f\r\n", LEDsTMEP[TMEPdistrictIndex], tempCelsius);

        if(!(firstRun && currentMapTMEP)) {
          byte r, g, b = 0;
          float ratio = 2 * (tempCelsius - minThreshold) / (maxThreshold - minThreshold);
          b = int(maxFloat(0.0, 255 * (1 - ratio)));
          r = int(maxFloat(0.0, 255 * (ratio - 1)));
          g = 255 - b - r;

          if (LEDsTMEP[TMEPdistrictIndex] >= 0) {
            if (log) Serial.printf("Rozsvecuji mesto %d barvou R=%d G=%d B=%d\r\n", LEDsTMEP[TMEPdistrictIndex], r, g, b);
            pixely.setPixelColor(LEDsTMEP[TMEPdistrictIndex], pixely.Color(r, g, b));
          }
        }
      }
    }

    if (!(firstRun && currentMapTMEP)) pixely.show();
    return 0;
  }
}

// Helper function to compare two float values and return the maximum
float maxFloat(float a, float b) {
  return (a > b) ? a : b;
}

// Stazeni radarovych dat z webu
void stahniData() {
  HTTPClient http;

  // Handle all maps with correct json format
  if (DEBUG) Serial.print("Zvolený mapový režim: ");
  if (DEBUG) Serial.println(GetSelectedMapMode());
  String url = "";
  switch (currentMap) {
    case MapFlag:
      url = urlFlag;
      currentMapTMEP = false;
      break;
    case MapCitiesMajor:
      url = urlCitiesMajor;
      currentMapTMEP = false;
      break;
    case MapRain:
      url = urlRain;
      currentMapTMEP = false;
      break;
    case MapTemp:
      url = urlTemp;
      currentMapTMEP = true;
      break;
    case MapRegions:
      url = urlRegions;
      currentMapTMEP = false;
      break;
    case MapHumid:
      url = urlHumid;
      currentMapTMEP = true;
      break;
    case MapPressure:
      url = urlPressure;
      currentMapTMEP = true;
      break;
    case MapDust:
      url = urlDust;
      currentMapTMEP = true;
      break;
    default:
      return;
  }

  if (DEBUG) Serial.print("Stahuji data z ");
  if (DEBUG) Serial.println(url);
  http.begin(url);
  http.addHeader("Cache-Control", "no-cache");
  http.addHeader("Pragma", "no-cache");
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    int err = jsonDecoder(http.getString(), DEBUG);
    if (DEBUG) {
      switch (err) {
        case 0:
          Serial.println("Hotovo!");
          break;
        case -1:
          Serial.println("Spatny format JSONu");
          break;
        case -2:
          Serial.println("Malo pameti, navys velikost StaticJsonDocument");
          break;
        case -3:
          Serial.println("Chyba pri parsovani JSONu");
          break;
      }
    }
  }
  http.end();
}

String GetSelectedMapMode() {
  switch (currentMap) {
    case MapFlag:
      return "Vlajka";
    case MapCitiesMajor:
      return "Krajská města";
    case MapRain:
      return "Srážky";
    case MapTemp:
      return "Teplotní mapa";
    case MapRegions:
      return "Kraje";
    case MapHumid:
      return "Vlhkost";
    case MapPressure:
      return "Tlak";
    case MapDust:
      return "Prašnost";
    default:
      return "";
  }
}

String GetUnitForMapMode(SelectedMap mapToCheck) {
  switch (mapToCheck) {
    case MapTemp:
      return " °C";
    case MapHumid:
      return " %";
    case MapPressure:
      return " hPa";
    case MapDust:
      return " mg/m3";
    default:
      return "";
  }
}

void processMapRequest(const String &arg, SelectedMap mapType, bool selectedIsTMEP) {
  int err = jsonDecoder(arg, DEBUG);

  switch (err) {
    case 0:
      currentMap = mapType;
      currentMapTMEP = selectedIsTMEP;

      // Range adjusting
      if (currentMapTMEP) {
        firstRun = true;
        delay(100);
        stahniData();
        delay(100);
        firstRun = false;
      }

      // Redraw map
      stahniData();

      server.sendHeader("Location", "http://" + WiFi.localIP().toString() + "");
      server.send(302);  // Kód 302 označuje přesměrování (Found/Temporary Redirect)
      break;
    case -1:
      server.send(200, "text/plain", "CHYBA\nSpatny format JSON");
      break;
    case -2:
      server.send(200, "text/plain", "CHYBA\nMalo pameti RAM pro JSON. Navys ji!");
      break;
    case -3:
      server.send(200, "text/plain", "CHYBA\nNepodarilo se mi dekodovat jSON");
      break;
  }
}

float getMiddleNumber(float minVal, float maxVal) {
  return minVal + (maxVal - minVal) / 2;
}

// Tuto funkci HTTP server zavola v pripade HTTP GET/POST pzoadavku na korenovou cestu /
void httpDotaz(void) {
  // Pokud HTTP data obsahuji parametr mesta
  // predame jeho obsah JSON dekoderu
  if (server.hasArg("temp")) {
    maxThreshold = -25;
    minThreshold = 45;
    processMapRequest(server.arg("temp"), MapTemp, true);
  } else if (server.hasArg("rain")) {
    processMapRequest(server.arg("rain"), MapRain, false);
  } else if (server.hasArg("flag")) {
    processMapRequest(server.arg("flag"), MapFlag, false);
  } else if (server.hasArg("citiesMajor")) {
    processMapRequest(server.arg("citiesMajor"), MapCitiesMajor, false);
  } else if (server.hasArg("regions")) {
    processMapRequest(server.arg("regions"), MapRegions, false);
  } else if (server.hasArg("humidity")) {
    maxThreshold = 0;
    minThreshold = 100;
    processMapRequest(server.arg("humidity"), MapHumid, true);
  } else if (server.hasArg("pressure")) {
    maxThreshold = 850;
    minThreshold = 1200;
    processMapRequest(server.arg("pressure"), MapPressure, true);
  } else if (server.hasArg("dust")) {
    maxThreshold = 0;
    minThreshold = 300;
    processMapRequest(server.arg("dust"), MapDust, true);
  } else if (server.hasArg("css")) {
    String cssContent = String(
      "html, body {\n"
      "  text-align: center;\n"
      "  background: #FFFFFF;\n"
      "  color: #000000;\n"
      "}\n"
      "\n"
      "button {\n"
      "  width: 90%; height: 32px; margin: 6px;\n"
      "}\n"
      "\n"
      ".selected {\n"
      "  background: #00A2FF; font-weight: bold;\n"
      "}\n"
      ".minimum {\n"
      "  color: #00A2FF;\n"
      "}\n"
      ".maximum {\n"
      "  color: #FF0000;\n"
      "}\n"
      ".median {\n"
      "  color: #00FF00;\n"
      "}\n"
      "\n");
    server.send(200, "text/css", cssContent);
  }
  // Pokud jsme do mapy poslali jen HTTP GET/POST parametr smazat, mapa zhasne
  else if (server.hasArg("smazat")) {
    server.send(200, "text/plain", "OK");
    pixely.clear();
    pixely.show();
  } else if (server.hasArg("jas")) {
    server.send(200, "text/plain", "OK");
    jas = server.arg("jas").toInt();
    pixely.setBrightness(jas);
    pixely.show();
  }
  // Ve vsech ostatnich pripadech odpovime chybovym hlasenim
  else {
    if (firstRun) {
      server.send(200, "text/plain", "Startovani, cekej prosim.");
    } else {
      String htmlContent = String("<!DOCTYPE html>\n"
                                  "<html>\n"
                                  "<head>\n"
                                  "  <meta charset=\"UTF-8\">\n"
                                  "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=0\">\n"
                                  "  <title>Ovládání mapy</title>\n"
                                  "  <link href=\"http://"
                                  + WiFi.localIP().toString() + "?css={}\" rel=\"stylesheet\" crossorigin=\"anonymous\" />\n"
                                                                "</head>\n"
                                                                "<body>\n"
                                                                "  <h1>Ovládání mapy</h1>\n"
                                                                "  <h2>Zvolený režim: "
                                  + (GetSelectedMapMode()) + "</h2>\n"
                                  + (currentMapTMEP ? String("<p><span class=\"minimum\">Minimum: " + String(minThreshold) + GetUnitForMapMode(currentMap) + "</span><br />\n<span class=\"median\">" + String(getMiddleNumber(minThreshold, maxThreshold)) + GetUnitForMapMode(currentMap) + "</span><br />\n<span class=\"maximum\">Maximum: " + String(maxThreshold) + GetUnitForMapMode(currentMap) + "</span></p>\n") : String(""))
                                  + (currentMap == MapRain ? String("<p><span class=\"minimum\">Minimum: 0 mm/h</span><br />\n<span class=\"maximum\">Maximum: ∞ mm/h</span></p>\n") : String("")) + "\n"
                                  "  <button onclick=\"sendRainRequest()\" class=\""
                                  + (currentMap == MapRain ? String("selected") : String("")) + "\">Zobrazit srážky</button>\n"
                                                                                                "  <button onclick=\"sendTempRequest()\" class=\""
                                  + (currentMap == MapTemp ? String("selected") : String("")) + "\">Zobrazit teplotní mapu</button>\n"
                                                                                                "  <button onclick=\"sendHumidRequest()\" class=\""
                                  + (currentMap == MapHumid ? String("selected") : String("")) + "\">Zobrazit vlhkost</button>\n"
                                                                                                 "  <button onclick=\"sendPressureRequest()\" class=\""
                                  + (currentMap == MapPressure ? String("selected") : String("")) + "\">Zobrazit tlak</button>\n"
                                                                                                    "  <button onclick=\"sendDustRequest()\" class=\""
                                  + (currentMap == MapDust ? String("selected") : String("")) + "\">Zobrazit prašnost</button>\n"
                                                                                                "  <button onclick=\"sendFlagRequest()\" class=\""
                                  + (currentMap == MapFlag ? String("selected") : String("")) + "\">Zobrazit vlajku</button>\n"
                                                                                                "  <button onclick=\"sendCitiesMajorRequest()\" class=\""
                                  + (currentMap == MapCitiesMajor ? String("selected") : String("")) + "\">Zobrazit krajská města</button>\n"
                                                                                                       "  <button onclick=\"sendRegionsRequest()\" class=\""
                                  + (currentMap == MapRegions ? String("selected") : String("")) + "\">Zobrazit kraje</button>\n"
                                                                                                   "\n"
                                                                                                   "  <script>\n"
                                                                                                   "    function sendRainRequest() {\n"
                                                                                                   "      var url = \"http://"
                                  + WiFi.localIP().toString() + "?rain={}\";\n"
                                                                "      window.location.href = url;\n"
                                                                "    }\n"
                                                                "\n"
                                                                "    function sendTempRequest() {\n"
                                                                "      var url = \"http://"
                                  + WiFi.localIP().toString() + "?temp={}\";\n"
                                                                "      window.location.href = url;\n"
                                                                "    }\n"
                                                                "\n"
                                                                "    function sendHumidRequest() {\n"
                                                                "      var url = \"http://"
                                  + WiFi.localIP().toString() + "?humidity={}\";\n"
                                                                "      window.location.href = url;\n"
                                                                "    }\n"
                                                                "\n"
                                                                "    function sendPressureRequest() {\n"
                                                                "      var url = \"http://"
                                  + WiFi.localIP().toString() + "?pressure={}\";\n"
                                                                "      window.location.href = url;\n"
                                                                "    }\n"
                                                                "\n"
                                                                "    function sendDustRequest() {\n"
                                                                "      var url = \"http://"
                                  + WiFi.localIP().toString() + "?dust={}\";\n"
                                                                "      window.location.href = url;\n"
                                                                "    }\n"
                                                                "\n"
                                                                "    function sendFlagRequest() {\n"
                                                                "      var url = \"http://"
                                  + WiFi.localIP().toString() + "?flag={}\";\n"
                                                                "      window.location.href = url;\n"
                                                                "    }\n"
                                                                "\n"
                                                                "    function sendCitiesMajorRequest() {\n"
                                                                "      var url = \"http://"
                                  + WiFi.localIP().toString() + "?citiesMajor={}\";\n"
                                                                "      window.location.href = url;\n"
                                                                "    }\n"
                                                                "\n"
                                                                "    function sendRegionsRequest() {\n"
                                                                "      var url = \"http://"
                                  + WiFi.localIP().toString() + "?regions={}\";\n"
                                                                "      window.location.href = url;\n"
                                                                "    }\n"
                                                                "  </script>\n"
                                                                "</body>\n"
                                                                "</html>\n");

      server.send(200, "text/html", htmlContent);
    }
  }
}

// Hlavni funkce setup se zpracuje hned po startu cipu ESP32
void setup() {
  // Nastartujeme serivou linku rychlosti 115200 b/s
  Serial.begin(115200);
  // Pripojime se k Wi-Fi a pote vypiseme do seriove linky IP adresu
  WiFi.disconnect();  // Vynucene odpojeni; obcas pomuze, kdyz se cip po startu nechce prihlasit
  WiFi.mode(WIFI_STA);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.setHostname(hostname);  //define hostname
  WiFi.begin(ssid, heslo);
  Serial.printf("Pripojuji se k %s ", ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Automaticke pripojeni pri ztrate Wi-Fi
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  // Vypiseme do seriove linky pro kontrolu LAN IP adresu mapy
  Serial.print("OK\nIP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Hostname: ");
  Serial.println(hostname);
  // Pro HTTP pozadavku / zavolame funkci httpDotaz
  server.on("/", httpDotaz);
  // Aktivujeme server
  server.begin();
  // Nakonfigurujeme adresovatelene LED do vychozi zhasnute pozice
  // Nastavime 8bit jas na hodnotu 5
  // Nebude svitit zbytecne moc a vyniknou mene kontrastni barvy
  pixely.begin();
  pixely.setBrightness(jas);
  pixely.clear();
  pixely.show();

  // Pri startovani zobraz vlajku
  currentMap = MapFlag;
  firstRun = true;
  currentMapTMEP = false;
  stahniData();
}

// Smycka loop se opakuje stale dokola
// a nastartuje se po zpracovani funkce setup
void loop() {
  // Vyridime pripadne TCP spojeni klientu se serverem
  server.handleClient();
  // Jednou za zvoleny interval stahnu nova data
  if (millis() - t > ((firstRun && currentMap == MapFlag) ? startupDelay : delay10)) {
    if (firstRun) {
      currentMap = MapRain;
      currentMapTMEP = false;
      if(currentMapTMEP) stahniData();
      firstRun = false;
    }
    stahniData();
    t = millis();
  }

  // Přidáme část, která se vykoná jednou za 24 hodin
  // 86400000 = 24 * 60 * 60 * 1000
  if (millis() - last24HourTaskTime > 86400000) {
    // Vykonávání úlohy jednou za 24 hodin
    switch (currentMap) {
      case MapTemp:
        maxThreshold = -25;
        minThreshold = 45;
        break;
      case MapHumid:
        maxThreshold = 0;
        minThreshold = 100;
        break;
      case MapPressure:
        maxThreshold = 850;
        minThreshold = 1200;
        break;
      case MapDust:
        maxThreshold = 0;
        minThreshold = 300;
        break;
    }

    // For correct range adjustment after thresholds reset,
    // this will download data silently
    // (without glowing LEDs to not confuse viewer by different range)
    if (currentMapTMEP) {
      firstRun = true;
      stahniData();
      firstRun = false;
    }

    // Aktualizace času posledního vykonání úlohy
    last24HourTaskTime = millis();
  }

  // Pockame 2 ms (prenechame CPU pro ostatni ulohy na pozadi) a opakujeme
  delay(2);
}