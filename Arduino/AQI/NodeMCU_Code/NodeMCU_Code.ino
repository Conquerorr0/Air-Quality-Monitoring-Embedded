#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <ArduinoJson.h>
#include <time.h>
#include <SoftwareSerial.h>

// WiFi ve Firebase ayarları
#define WIFI_SSID "Parzival"
#define WIFI_PASSWORD "92357280"
#define FIREBASE_HOST "https://air-quality-monitoring-8c470-default-rtdb.europe-west1.firebasedatabase.app"
#define FIREBASE_AUTH "J8iK6aSAQUNIEGRalzlq4QANFvOI4KIrlHf5YsZX"

// JSON belge boyutu
#define JSON_DOC_SIZE 1024

// Serial haberleşme pinleri
#define RX_PIN 5  // D1 (GPIO5)
#define TX_PIN 4  // D2 (GPIO4)

SoftwareSerial arduinoSerial(RX_PIN, TX_PIN);  // Sadece RX ve TX pinlerini belirt

// NTP Sunucu ayarları
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3 * 3600;  // UTC+3 için
const int daylightOffset_sec = 0;

// Firebase nesneleri
FirebaseData firebaseData;
FirebaseAuth auth;
FirebaseConfig config;
FirebaseJson json;
FirebaseJson aggregatedJson;

// Günlük ortalama hesaplama için değişkenler
float dailyMq135Sum = 0;
float dailyLpgSum = 0;
float dailyCoSum = 0;
float dailySmokeSum = 0;
float dailyTempSum = 0;
float dailyHumSum = 0;
int dailyReadingCount = 0;
String currentDate = "";

// Zaman damgası alma fonksiyonu
String getTimestamp() {
  time_t now;
  time(&now);
  return String(now);
}

// Günün tarihini alma fonksiyonu
String getCurrentDate() {
  time_t now;
  time(&now);
  struct tm* timeinfo;
  timeinfo = localtime(&now);
  char dateStr[11];
  sprintf(dateStr, "%d-%02d-%02d", timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday);
  return String(dateStr);
}

// Günlük ortalamaları hesaplama ve Firebase'e gönderme
void calculateAndSendDailyAverages() {
  if (dailyReadingCount > 0) {
    String date = getCurrentDate();
    String aggregatedPath = "/AggregatedData/Daily/" + date;

    aggregatedJson.clear();
    aggregatedJson.set("averageMq135", dailyMq135Sum / dailyReadingCount);

    // MQ2 ortalamaları
    FirebaseJson mq2AveragesJson;
    mq2AveragesJson.set("lpg", dailyLpgSum / dailyReadingCount);
    mq2AveragesJson.set("co", dailyCoSum / dailyReadingCount);
    mq2AveragesJson.set("smoke", dailySmokeSum / dailyReadingCount);
    aggregatedJson.set("averageMq2", mq2AveragesJson);

    // Sıcaklık ve nem ortalamaları
    aggregatedJson.set("averageTemperature", dailyTempSum / dailyReadingCount);
    aggregatedJson.set("averageHumidity", dailyHumSum / dailyReadingCount);

    // Zaman damgası ve okuma sayısı
    aggregatedJson.set("timestamp", getTimestamp().toInt());
    aggregatedJson.set("readingCount", dailyReadingCount);

    if (Firebase.setJSON(firebaseData, aggregatedPath, aggregatedJson)) {
      Serial.println("Günlük ortalamalar Firebase'e gönderildi");
    } else {
      Serial.println("Günlük ortalama gönderimi başarısız: " + firebaseData.errorReason());
    }
  }
}

// Değişkenleri sıfırlama
void resetDailyVariables() {
  dailyMq135Sum = 0;
  dailyLpgSum = 0;
  dailyCoSum = 0;
  dailySmokeSum = 0;
  dailyTempSum = 0;
  dailyHumSum = 0;
  dailyReadingCount = 0;
}

void setup() {
  // Arduino ile haberleşme için software serial
  arduinoSerial.begin(9600);
  delay(100);

  // Debug için hardware serial
  Serial.begin(115200);
  Serial.println("\nNodeMCU başlatılıyor...");

  // WiFi bağlantısı
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("WiFi'ye bağlanılıyor");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi bağlantısı başarılı!");
  Serial.print("IP Adresi: ");
  Serial.println(WiFi.localIP());

  // NTP sunucusuna bağlan ve zamanı ayarla
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  while (!time(nullptr)) {
    delay(1000);
    Serial.println("Zaman ayarlanıyor...");
  }

  // Firebase yapılandırması
  config.api_key = FIREBASE_AUTH;
  config.database_url = FIREBASE_HOST;

  // Firebase'i başlat
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Firebase bağlantı ayarları
  firebaseData.setBSSLBufferSize(1024, 1024);
  firebaseData.setResponseSize(1024);

  // İlk tarih değerini ayarla
  currentDate = getCurrentDate();

  Serial.println("Kurulum tamamlandı, veri bekleniyor...");
}

void loop() {
  while (arduinoSerial.available() > 0) {
    String jsonString = arduinoSerial.readStringUntil('\n');
    jsonString.trim();  // Başındaki ve sonundaki boşlukları temizle

    if (jsonString.length() > 0) {
      Serial.println("Alınan veri: " + jsonString);  // Debug için

      // JSON verisini ayrıştır
      DynamicJsonDocument doc(JSON_DOC_SIZE);
      DeserializationError error = deserializeJson(doc, jsonString);

      if (!error) {
        // Yeni günün başlangıcını kontrol et
        String newDate = getCurrentDate();
        if (newDate != currentDate) {
          calculateAndSendDailyAverages();
          resetDailyVariables();
          currentDate = newDate;
        }

        // Sensör verilerini al
        int mq135 = doc["mq135"];
        float lpg = doc["mq2"]["lpg"];
        float co = doc["mq2"]["co"];
        float smoke = doc["mq2"]["smoke"];
        float temperature = doc["temperature"];
        float humidity = doc["humidity"];

        Serial.println("Okunan değerler:");  // Debug için
        Serial.println("MQ135: " + String(mq135));
        Serial.println("LPG: " + String(lpg));
        Serial.println("CO: " + String(co));
        Serial.println("Smoke: " + String(smoke));
        Serial.println("Sıcaklık: " + String(temperature));
        Serial.println("Nem: " + String(humidity));

        // Günlük toplamları güncelle
        dailyMq135Sum += mq135;
        dailyLpgSum += lpg;
        dailyCoSum += co;
        dailySmokeSum += smoke;
        dailyTempSum += temperature;
        dailyHumSum += humidity;
        dailyReadingCount++;

        // Firebase'e gönderilecek JSON'ı hazırla
        String timestamp = getTimestamp();
        String sensorPath = "/SensorData/" + timestamp;

        FirebaseJson sensorJson;
        // Ana sensör verileri
        sensorJson.set("mq135", mq135);

        // MQ2 verilerini nested object olarak gönder
        FirebaseJson mq2Json;
        mq2Json.set("lpg", lpg);
        mq2Json.set("co", co);
        mq2Json.set("smoke", smoke);
        sensorJson.set("mq2", mq2Json);

        // Diğer sensör verileri
        sensorJson.set("temperature", temperature);
        sensorJson.set("humidity", humidity);
        sensorJson.set("timestamp", timestamp.toInt());

        // Firebase'e gönder
        if (Firebase.setJSON(firebaseData, sensorPath, sensorJson)) {
          Serial.println("Veriler Firebase'e gönderildi");

          // Eşik değerlerini kontrol et
          if (Firebase.getJSON(firebaseData, "/Settings/thresholds")) {
            DynamicJsonDocument thresholds(256);
            DeserializationError error = deserializeJson(thresholds, firebaseData.jsonString());

            if (!error) {
              int mq135Threshold = thresholds["mq135"]["max"];
              if (mq135 > mq135Threshold) {
                Serial.println("MQ135 eşik değeri aşıldı!");
              }
            }
          }
        } else {
          Serial.println("Firebase veri gönderimi başarısız: " + firebaseData.errorReason());
        }
      } else {
        Serial.println("JSON ayrıştırma hatası: " + String(error.c_str()));
      }
    }
  }
}