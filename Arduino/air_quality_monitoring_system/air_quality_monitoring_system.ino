#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <DHT.h>

#define DHTPIN D2      // DHT11 veri pini
#define DHTTYPE DHT11  // DHT tipi
#define MQ135_PIN A0   // MQ135 sensör pini
#define MQ2_PIN A0     // MQ2 sensör pini (aynı A0 pinini kullanabilirsiniz)

DHT dht(DHTPIN, DHTTYPE);

String FIREBASE_HOST = "air-quality-monitoring-8c470-default-rtdb.europe-west1.firebasedatabase.app";
String FIREBASE_PATH = "/SensorData.json";
String WIFI_SSID = "Greenwich Cafe Kutuphane";
String WIFI_PASSWORD = "salepistermisiniz";

void setup() {
  Serial.begin(115200);
  dht.begin();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    // Sensör verilerini oku
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    int mq135Value = analogRead(MQ135_PIN);
    int mq2Value = analogRead(MQ2_PIN);

    float lpg = map(mq2Value, 0, 1023, 0, 100);
    float co = map(mq2Value, 0, 1023, 0, 50);
    float smoke = map(mq2Value, 0, 1023, 0, 70);

    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("Failed to read DHT11 data!");
      return;
    }

    // JSON verisi oluştur
    String jsonData = "{";
    jsonData += "\"temperature\":" + String(temperature) + ","; 
    jsonData += "\"humidity\":" + String(humidity) + ",";
    jsonData += "\"mq135\":" + String(mq135Value) + ",";
    jsonData += "\"mq2\":{\"lpg\":" + String(lpg) + ",\"co\":" + String(co) + ",\"smoke\":" + String(smoke) + "}"; 
    jsonData += "}";

    Serial.println("Sending data: " + jsonData);

    sendDataToFirebase(jsonData);
    delay(60000);  // 1 dakika bekle
  } else {
    Serial.println("WiFi disconnected! Reconnecting...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }
}

void sendDataToFirebase(String data) {
  WiFiClient client;
  HTTPClient http;

  String url = FIREBASE_HOST + FIREBASE_PATH;

  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(data);

  if (httpResponseCode > 0) {
    Serial.println("Data sent successfully!");
    Serial.println("HTTP Response code: " + String(httpResponseCode));
  } else {
    Serial.println("Error sending data. HTTP Response code: " + String(httpResponseCode));
  }

  http.end();
}
