#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <time.h>

const char* ssid = "wifi_id";
const char* password = "wifi_password";
const String apiKey = "Open_weather_api_key";

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3 * 3600;
const int daylightOffset_sec = 0;

#define TFT_CS   17
#define TFT_DC   16
#define TFT_RST  5
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

#define TOUCH_CS 21
#define TOUCH_IRQ 4
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

String iller[81] = {
  "Adana","Adiyaman","Afyon","Agri","Amasya","Ankara","Antalya","Artvin","Aydin","Balikesir",
  "Bilecik","Bingol","Bitlis","Bolu","Burdur","Bursa","Canakkale","Cankiri","Corum","Denizli",
  "Diyarbakir","Edirne","Elazig","Erzincan","Erzurum","Eskisehir","Gaziantep","Giresun","Gumushane","Hakkari",
  "Hatay","Isparta","Mersin","Istanbul","Izmir","Kars","Kastamonu","Kayseri","Kirklareli","Kirsehir",
  "Kocaeli","Konya","Kutahya","Malatya","Manisa","Kahramanmaras","Mardin","Mugla","Mus","Nevsehir",
  "Nigde","Ordu","Rize","Sakarya","Samsun","Siirt","Sinop","Sivas","Tekirdag","Tokat",
  "Trabzon","Tunceli","Sanliurfa","Usak","Van","Yozgat","Zonguldak","Aksaray","Bayburt","Karaman",
  "Kirikale","Batman","Sirnak","Bartin","Ardahan","Igdir","Yalova","Karabuk","Kilis","Osmaniye",
  "Duzce"
};

int sayfa = 0;
int secilenPlaka = -1;
float sicaklik = 0;
int nem = 0;
float ruzgar = 0;

void setup() {
  Serial.begin(115200);
  tft.begin();
  tft.setRotation(0);
  ts.begin();
  ts.setRotation(0);

  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(10, 140);
  tft.setTextSize(2);
  tft.println("Wi-Fi baglaniyor...");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  plakaEkrani();
}

void loop() {
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    int x = map(p.x, 3700, 200, 0, 240);
    int y = map(p.y, 3700, 200, 0, 320);

    if (y > 270 && y < 310 && x > 10 && x < 110) {
      sayfa = (sayfa + 2) % 3;
      plakaEkrani();
      delay(300);
    }
    if (y > 270 && y < 310 && x > 130 && x < 230) {
      sayfa = (sayfa + 1) % 3;
      plakaEkrani();
      delay(300);
    }

    for (int row = 0; row < 5; row++) {
      for (int col = 0; col < 2; col++) {
        int btnX = 10 + col * 120;
        int btnY = 30 + row * 45;
        if (x > btnX && x < btnX + 100 && y > btnY && y < btnY + 40) {
          int index = row * 2 + col;
          secilenPlaka = sayfa * 30 + index + 1;
          if (secilenPlaka <= 81) {
            havaDurumuEkrani();
            delay(300);
          }
        }
      }
    }
  }
}

void plakaEkrani() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 5);
  tft.printf("Plaka Sec (%d-%d)\n", (sayfa * 30) + 1, min((sayfa + 1) * 30, 81));

  for (int i = 0; i < 10; i++) {
    int plaka = sayfa * 30 + i + 1;
    if (plaka > 81) break;
    int x = 10 + (i % 2) * 120;
    int y = 30 + (i / 2) * 45;
    tft.fillRoundRect(x, y, 100, 40, 6, ILI9341_BLUE);
    tft.setCursor(x + 5, y + 12);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(1);
    tft.printf("%2d %s", plaka, iller[plaka - 1].c_str());
  }

  tft.fillRoundRect(10, 270, 100, 40, 6, ILI9341_BLUE);
  tft.setCursor(30, 280);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.print("GERI");

  tft.fillRoundRect(130, 270, 100, 40, 6, ILI9341_BLUE);
  tft.setCursor(150, 280);
  tft.print("ILERI");
}

void havaDurumuEkrani() {
  String sehir = iller[secilenPlaka - 1];
  String url = "http://api.openweathermap.org/data/2.5/weather?q=" + sehir + "&appid=" + apiKey + "&units=metric&lang=tr";

  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();
  if (httpCode > 0) {
    String payload = http.getString();
    StaticJsonDocument<1024> doc;
    deserializeJson(doc, payload);
    sicaklik = doc["main"]["temp"];
    nem = doc["main"]["humidity"];
    ruzgar = doc["wind"]["speed"];
  }
  http.end();

  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.printf("%2d - %s\n", secilenPlaka, sehir.c_str());
  tft.setCursor(10, 50);
  tft.printf("Sicaklik: %.1f C\n", sicaklik);
  tft.setCursor(10, 80);
  tft.printf("Nem: %d %%\n", nem);
  tft.setCursor(10, 110);
  tft.printf("Ruzgar: %.1f m/s\n", ruzgar);

  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    char timeStr[16];
    strftime(timeStr, sizeof(timeStr), "Saat: %H:%M", &timeinfo);
    tft.setCursor(10, 150);
    tft.println(timeStr);
  }

  tft.fillRoundRect(60, 260, 120, 40, 6, ILI9341_RED);
  tft.setCursor(90, 270);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.print("GERI");

  while (true) {
    if (ts.touched()) {
      TS_Point p = ts.getPoint();
      int x = map(p.x, 3700, 200, 0, 240);
      int y = map(p.y, 3700, 200, 0, 320);
      if (x > 60 && x < 180 && y > 260 && y < 300) {
        plakaEkrani();
        break;
      }
    }
  }
}
