#include <SoftwareSerial.h>
#include <DHT.h>

#define DHTPIN 7      // DHT11 veri pini (Arduino Uno'nun dijital pinlerinden birini kullanabilirsiniz)
#define DHTTYPE DHT11  // DHT tipi
#define MQ135_PIN A0   // MQ135 sensör pini
#define MQ2_PIN A1 // A1 pinini kullanın

DHT dht(DHTPIN, DHTTYPE);

String FIREBASE_HOST = "air-quality-monitoring-8c470-default-rtdb.europe-west1.firebasedatabase.app";
String FIREBASE_PATH = "/SensorData.json";
String WIFI_SSID = "Greenwich Cafe Kutuphane";
String WIFI_PASSWORD = "salepistermisiniz";

// SoftwareSerial ile ESP8266 modülüne bağlanacağız
SoftwareSerial espSerial(2, 3); // RX, TX (Arduino Uno'nun pinleri)

void setup() {
  Serial.begin(115200);
  espSerial.begin(115200);
  dht.begin();

  // ESP8266'yı başlat
  sendCommand("AT+RST\r\n", 2000);  // ESP8266'yı resetle
  sendCommand("AT+CWMODE=1\r\n", 2000);  // Wi-Fi mode 1 (station mode)
  connectToWiFi();
}

void loop() {
  if (WiFiStatus() == 1) {
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
    connectToWiFi();
  }
}

void sendDataToFirebase(String data) {
  String url = FIREBASE_HOST + FIREBASE_PATH;

  String request = "POST " + url + " HTTP/1.1\r\n";
  request += "Host: " + FIREBASE_HOST + "\r\n";
  request += "Content-Type: application/json\r\n";
  request += "Content-Length: " + String(data.length()) + "\r\n\r\n";
  request += data;

  espSerial.println(request);
  delay(1000);  // Veri gönderildikten sonra bir süre bekleyin
}

void sendCommand(String command, int timeout) {
  espSerial.println(command);
  long int time = millis();
  while ((time + timeout) > millis()) {
    while (espSerial.available()) {
      char c = espSerial.read();
      Serial.write(c);
    }
  }
}

void connectToWiFi() {
  sendCommand("AT+CWJAP=\""+WIFI_SSID+"\",\""+WIFI_PASSWORD+"\"\r\n", 5000);
}

int WiFiStatus() {
  sendCommand("AT+CWJAP?\r\n", 5000);
  return (espSerial.find("OK")) ? 1 : 0;  // Eğer Wi-Fi bağlıysa 1 döner
}
