#include <DHT.h>
#include <SoftwareSerial.h>

// Pin tanımlamaları
#define DHT_PIN 6
#define DHT_TYPE DHT11
#define MQ135_PIN A0
#define MQ2_PIN A1

DHT dht(DHT_PIN, DHT_TYPE);
SoftwareSerial nodeMCUSerial(2, 3);  // RX, TX pinleri

void setup() {
  Serial.begin(9600);
  nodeMCUSerial.begin(9600);
  dht.begin();
}

void loop() {
  // MQ135 sensöründen veri okuma
  int mq135 = analogRead(MQ135_PIN);

  // MQ2 sensöründen veri okuma
  int mq2 = analogRead(MQ2_PIN);
  float lpg = mq2 * 0.1;  // Kalibrasyon katsayısı
  float co = mq2 * 0.05;
  float smoke = mq2 * 0.07;

  // DHT11 sensöründen sıcaklık ve nem okuma
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Hatalı okuma kontrolü
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("DHT sensöründen okuma hatası!");
    return;
  }

  // Verileri NodeMCU'ya gönder
  nodeMCUSerial.print(mq135);
  nodeMCUSerial.print(",");
  nodeMCUSerial.print(lpg);
  nodeMCUSerial.print(",");
  nodeMCUSerial.print(co);
  nodeMCUSerial.print(",");
  nodeMCUSerial.print(smoke);
  nodeMCUSerial.print(",");
  nodeMCUSerial.print(temperature);
  nodeMCUSerial.print(",");
  nodeMCUSerial.print(humidity);
  nodeMCUSerial.println();

  // 1 dakika bekle
  delay(60000);
}