#include <Wire.h>
#include <DHT.h>
#include <ArduinoJson.h>

// Pin tanımlamaları
#define MQ135_PIN A0
#define MQ2_PIN A1
#define DHTPIN 2
#define DHTTYPE DHT22  // DHT22 sensörü kullanıyoruz

// JSON belge boyutu
#define JSON_DOC_SIZE 256

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(9600);
  dht.begin();
  delay(2000);  // DHT22'nin başlaması için bekleme
}

void loop() {
  // MQ135 sensöründen veri okuma
  int mq135 = analogRead(MQ135_PIN);

  // MQ2 sensöründen veri okuma
  int mq2 = analogRead(MQ2_PIN);
  float lpg = mq2 * 0.1;
  float co = mq2 * 0.05;
  float smoke = mq2 * 0.07;

  // DHT22 sensöründen sıcaklık ve nem okuma
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Hatalı okuma kontrolü
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("DHT sensöründen okuma hatası!");
    return;
  }

  // JSON verisi oluştur
  DynamicJsonDocument doc(JSON_DOC_SIZE);

  doc["mq135"] = mq135;

  JsonObject mq2_data = doc.createNestedObject("mq2");
  mq2_data["lpg"] = lpg;
  mq2_data["co"] = co;
  mq2_data["smoke"] = smoke;

  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["timestamp"] = millis();

  // JSON verisini seri porta gönder
  serializeJson(doc, Serial);
  Serial.println();

  delay(60000);  // 1 dakika bekle
}