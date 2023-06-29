# Laskakit_MapaCR
Prototyp vlastního FW pro LaskaKit mapu ČR. Jde o upravenou verzi FW od Jakuba Čížka ze Živě.cz: https://www.zive.cz/clanky/naprogramovali-jsme-radarovou-mapu-ceska-ukaze-kde-prave-prsi-a-muzete-si-ji-dat-i-na-zed/sc-3-a-222111/default.aspx                    
                  
Volitelné režimy zobrazitelné na mapě:
- aktuální srážky
- teplotní mapa, vlhkost, tlak, prašnost (z čidel TMEP)
- vlajka ČR (při startu nebo při zvoleném režimu)
- krajská města
- kraje

Nastavení zobrazení pomocí GUI po připojení prohlížečem na IP mapy.

Náhled funkčnosti na YouTube:
https://www.youtube.com/watch?v=hC3fB_leQMU
[![youtube](https://img.youtube.com/vi/hC3fB_leQMU/maxresdefault.jpg)](https://www.youtube.com/watch?v=hC3fB_leQMU)


## Instalace
Pro instalaci ESP32 desek do ArduinoIDE následujte tento návod: https://navody.dratek.cz/navody-k-produktum/jednoducha-instalace-esp32-do-arduino-ide.html         
                  
Do ArduinoIDE si doinstalujte následující knihovny:
- Adafruit_NeoPixel
- ArduinoJson

V souboru WiFi_Config.h si upravte SSID a heslo k vaší WiFi síti.

## Použití
Po nahrání sketche do ESP32 v LaskaKit mapě si otevřete SerialMonitor v ArduinoIDE, nastavte rychlost 115200 baudů a sledujte výstup. Mapa vám po připojení k WiFi vypíše svou IP adresu, přes kterou můžete přistupovat k ovládacímu rozhraní.


## Poznámky
JSON TMEP: https://wiki.tmep.cz/doku.php?id=ruzne:led_mapa_okresu_cr

LEDs sequence and districts
| Real LaskaKit ID | TMEP ID | Okres |
|:--:|:-------:|:------|
| 24 | 1 | Cheb |
| 19 | 2 | Sokolov |
| 16 | 3 | **Karlovy Vary** |
| 10 | 4 | Chomutov |
| 15 | 5 | Louny |
| 9 | 6 | Most |
| 6 | 7 | Teplice |
| 8 | 8 | Litoměřice |
| 3 | 9 | **Ústí nad Labem** |
| 0 | 10 | Děčín |
| 4 | 11 | Česká Lípa |
| 1 | 12 | **Liberec** |
| 2 | 13 | Jablonec nad Nisou |
| 5 | 14 | Semily |
| 11 | 15 | Jičín |
| 7 | 16 | Trutnov |
| 12 | 17 | Náchod |
| 18 | 18 | **Hradec Králové** |
| 21 | 19 | Rychnov nad Kněžnou |
| 29 | 20 | Ústí nad Orlicí |
| 27 | 21 | **Pardubice** |
| 34 | 22 | Chrudim |
| 38 | 23 | Svitavy |
| 31 | 24 | Šumperk |
| 17 | 25 | Jeseník |
| 25 | 26 | Bruntál |
| 45 | 27 | **Olomouc** |
| 30 | 28 | Opava |
| 36 | 29 | **Ostrava-město** |
| 35 | 30 | Karviná |
| 42 | 31 | Frýdek-Místek |
| 44 | 32 | Nový Jičín |
| 56 | 33 | Vsetín |
| 48 | 34 | Přerov |
| 61 | 35 | **Zlín** |
| 57 | 36 | Kroměříž |
| 65 | 37 | Uherské Hradiště |
| 68 | 38 | Hodonín |
| 59 | 39 | Vyškov |
| 49 | 40 | Prostějov |
| 55 | 41 | Blansko |
| 63 | 42 | **Brno-město** |
| 71 | 44 | Břeclav |
| 69 | 45 | Znojmo |
| 62 | 46 | Třebíč |
| 47 | 47 | Žďár nad Sázavou |
| 53 | 48 | **Jihlava** |
| 46 | 49 | Havlíčkův Brod |
| 51 | 50 | Pelhřimov |
| 64 | 51 | Jindřichův Hradec |
| 52 | 52 | Tábor |
| 67 | 53 | **České Budějovice** |
| 70 | 54 | Český Krumlov |
| 66 | 55 | Prachatice |
| 60 | 56 | Strakonice |
| 58 | 57 | Písek |
| 54 | 58 | Klatovy |
| 50 | 59 | Domažlice |
| 37 | 60 | Tachov |
| 40 | 62 | **Plzeň-město** |
| 41 | 64 | Rokycany |
| 23 | 65 | Rakovník |
| 22 | 66 | Kladno |
| 14 | 67 | Mělník |
| 13 | 68 | Mladá Boleslav |
| 20 | 69 | Nymburk |
| 28 | 70 | Kolín |
| 33 | 71 | Kutná Hora |
| 39 | 72 | Benešov |
| 43 | 73 | Příbram |
| 32 | 74 | Beroun |
| 26 | 77 | **Praha** |
