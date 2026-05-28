/**
 * PROJETO CALISTO - MVP (Nó de Telemetria)
 * Sistema de Monitoramento Preditivo de Condensadores
 * Instituição: UFRN / IMD - Local: NPITI
 * 
 * Sensores:
 *   - Vibração: MPU9250_asukiaaa (acelerômetro 9-eixos I2C)
 *   - Temperatura: MAX6675 + Termopar Tipo K (SPI)
 * 
 * Conectividade: Wi-Fi → MQTT → Adafruit IO
 */

#include <Arduino.h>
#include "config.h"
#include "network.h"
#include "sensors.h"

unsigned long lastPublish = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("\n[CALISTO] Inicializando MVP Modular...");

  setupSensors();
  setupNetwork();
}

void loop() {
  // 1. Mantém as conexões vivas (Wi-Fi e MQTT)
  keepNetworkAlive();

  // 2. Temporizador não-blocante
  unsigned long now = millis();
  if (now - lastPublish >= PUBLISH_INTERVAL) {
    lastPublish = now;
    
    Serial.println("\n--- Iniciando coleta de dados ---");

    // 3. Coleta os dados delegando para o módulo de sensores
    float temp = getTemperature();
    float rmsX, rmsY, rmsZ;
    getVibrationRMS(rmsX, rmsY, rmsZ); // Passagem por referência

    // Imprime para depuração
    Serial.printf("Temperatura : %.2f °C\n", temp);
    Serial.printf("RMS Dinamico: X=%.4f g | Y=%.4f g | Z=%.4f g\n", rmsX, rmsY, rmsZ);

    // 4. Publica os dados delegando para o módulo de rede
    publishData(temp, rmsX, rmsY, rmsZ);
  }
}