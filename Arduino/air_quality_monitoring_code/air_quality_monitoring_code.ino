#include <SoftwareSerial.h>
#include <DHT.h>
#include <ArduinoJson.h>

// Pin tanimlamalari
#define DHTPIN 2
#define DHTTYPE DHT11
#define MQ135_PIN A3
#define MQ2_PIN A0
#define D0_PIN 7

// ESP-01S icin SoftwareSerial
SoftwareSerial espSerial(10, 11);  // TX = 10, RX = 11

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(9600);  // Arduino Seri Haberlesme
  espSerial.begin(115200);

  dht.begin();
  Serial.println("Setup tamamlandi.");

  // ESP'yi Wi-Fi agina bagla
  connectToWiFi();
}

void loop() {
  // Sensor verilerini oku
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int mq135Value = analogRead(MQ135_PIN);
  int mq2Value = analogRead(MQ2_PIN);

  // MQ2 degerlerini hesapla
  float lpg = map(mq2Value, 0, 1023, 0, 100);
  float co = map(mq2Value, 0, 1023, 0, 50);
  float smoke = map(mq2Value, 0, 1023, 0, 70);

  // JSON verisi olustur
  String jsonData = createJson(temperature, humidity, mq135Value, lpg, co, smoke);

  // Firebase'e gonder
  sendToFirebase(jsonData);

  delay(60000);  // 1 dakika bekle
}

void connectToWiFi() {
  sendCommand("AT+RST", "OK", 5000); // ESP'yi resetle
  sendCommand("ATE0", "OK", 2000);  // Yankı modunu kapat
  delay(1000);

  sendCommand("AT+CWMODE=1", "OK", 2000); // Wi-Fi Station Mode
  delay(1000);

  sendCommand("AT+CWJAP=\"Parzival\",\"92357280\"", "OK", 15000); // Wi-Fi Bağlantısı
  delay(2000);

  // Bağlantı durumunu kontrol et
  sendCommand("AT+CIPSTATUS", "STATUS:2", 5000); // Bağlı durumunu kontrol et
}


void sendToFirebase(String jsonData) {
  String firebaseHost = "34.107.226.223";
  String firebaseProjectId = "air-quality-monitoring-8c470";
  String firebaseSecret = "J8iK6aSAQUNIEGRalzlq4QANFvOI4KIrlHf5YsZX";  // Buraya aldığınız secret'ı yazın

  String path = "/SensorData.json?auth=" + firebaseSecret;

  sendCommand("AT+CIPSTART=\"TCP\",\"" + firebaseHost + "\",80", "OK", 5000);

  String postRequest = "POST " + path + " HTTP/1.1\r\n";
  postRequest += "Host: " + firebaseProjectId + ".firebasedatabase.app\r\n";
  postRequest += "Content-Type: application/json\r\n";
  postRequest += "Content-Length: " + String(jsonData.length()) + "\r\n";
  postRequest += "\r\n" + jsonData;

  sendCommand("AT+CIPSEND=" + String(postRequest.length()), ">", 2000);
  espSerial.print(postRequest);

  delay(3000);
  sendCommand("AT+CIPCLOSE", "OK", 2000);
}

String createJson(float temperature, float humidity, int mq135, float lpg, float co, float smoke) {
  StaticJsonDocument<200> doc;
  unsigned long timestamp = millis();

  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["mq135"] = mq135;
  JsonObject mq2 = doc.createNestedObject("mq2");
  mq2["lpg"] = lpg;
  mq2["co"] = co;
  mq2["smoke"] = smoke;
  doc["timestamp"] = timestamp;

  String jsonData;
  serializeJson(doc, jsonData);
  return jsonData;
}

void sendCommand(String command, String expectedResponse, int timeout) {
  espSerial.println(command);
  Serial.println("Komut gonderildi: " + command);

  long int startTime = millis();
  String response = "";

  while ((millis() - startTime) < timeout) {
    while (espSerial.available()) {
      char c = espSerial.read();
      response += c;
      delay(2);  // Karakterlerin tam gelmesini bekle
    }

    if (response.indexOf(expectedResponse) != -1) {
      Serial.println("Basarili: " + command);
      return;
    }

    // "busy" mesajini kontrol edip tekrar deneyebilirsiniz
    if (response.indexOf("busy") != -1) {
      Serial.println("ESP mesgul, tekrar deneniyor...");
      delay(2000);                                      // 2 saniye bekle ve tekrar dene
      sendCommand(command, expectedResponse, timeout);  // Komutlari yeniden gonder
      return;
    }
  }

  Serial.println("Basarisiz: " + command);
  Serial.println("Gelen Yanit: " + response);
}
