#include "network.h"
#include "config.h"
#include <WiFi.h>
#include <PubSubClient.h>

WiFiClient espClient;
PubSubClient client(espClient);

// Construção dinâmica dos tópicos baseada no config.h
String feed_temp   = String(IO_USERNAME) + "/feeds/calisto-temp";
String feed_vib_x  = String(IO_USERNAME) + "/feeds/calisto-vib-x";
String feed_vib_y  = String(IO_USERNAME) + "/feeds/calisto-vib-y";
String feed_vib_z  = String(IO_USERNAME) + "/feeds/calisto-vib-z";
String feed_status = String(IO_USERNAME) + "/feeds/calisto-status";

void setupNetwork() {
  Serial.printf("[WIFI] Conectando a %s ", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[WIFI] Conectado! IP: " + WiFi.localIP().toString());
  
  client.setServer(MQTT_SERVER, MQTT_PORT);
}

void keepNetworkAlive() {
  while (!client.connected()) {
    Serial.print("[MQTT] Conectando com Adafruit IO...");
    String clientId = "Calisto-ESP32-" + String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str(), IO_USERNAME, IO_KEY)) {
      Serial.println(" Conectado!");
    } else {
      Serial.print(" Falha, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando em 5 seg...");
      delay(5000);
    }
  }
  client.loop(); // Processa pacotes de keep-alive
}

void publishData(float temp, float rmsX, float rmsY, float rmsZ) {
  client.publish(feed_temp.c_str(), String(temp, 2).c_str());
  delay(1000);
  client.publish(feed_vib_x.c_str(), String(rmsX, 4).c_str());
  delay(1000);
  client.publish(feed_vib_y.c_str(), String(rmsY, 4).c_str());
  delay(1000);
  client.publish(feed_vib_z.c_str(), String(rmsZ, 4).c_str());
  delay(1000);
  
  client.publish(feed_status.c_str(), "1"); 
  
  Serial.println("[OK] Dados publicados na nuvem.");
}