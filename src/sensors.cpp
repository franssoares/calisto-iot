#include "sensors.h"
#include "config.h"
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <max6675.h>

// Instâncias locais (não poluem o escopo global)
Adafruit_MPU6050 mpu;
MAX6675 thermocouple(THERMO_SCK, THERMO_CS, THERMO_SO);

void setupSensors() {
  if (!mpu.begin()) {
    Serial.println("[ERRO] MPU6050 nao encontrado!");
    while (1) delay(10); // Trava o sistema se falhar
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.println("[OK] MPU6050 Configurado.");
}

float getTemperature() {
  float tempC = thermocouple.readCelsius();
  if (isnan(tempC)) {
    Serial.println("[ERRO] Falha ao ler MAX6675.");
    return 0.0;
  }
  return tempC;
}

void getVibrationRMS(float &rmsX, float &rmsY, float &rmsZ) {
  float bufferX[NUM_AMOSTRAS], bufferY[NUM_AMOSTRAS], bufferZ[NUM_AMOSTRAS];
  float sumX = 0, sumY = 0, sumZ = 0;
  sensors_event_t a, g, temp_mpu;

  // Passo A: Coletar e somar
  for (int i = 0; i < NUM_AMOSTRAS; i++) {
    mpu.getEvent(&a, &g, &temp_mpu);
    bufferX[i] = a.acceleration.x / 9.81;
    bufferY[i] = a.acceleration.y / 9.81;
    bufferZ[i] = a.acceleration.z / 9.81;

    sumX += bufferX[i];
    sumY += bufferY[i];
    sumZ += bufferZ[i];
    delay(DELAY_AMOSTRA);
  }

  // Passo B: Calcular Nível DC (Média)
  float meanX = sumX / NUM_AMOSTRAS;
  float meanY = sumY / NUM_AMOSTRAS;
  float meanZ = sumZ / NUM_AMOSTRAS;

  // Passo C: Remover Nível DC e somar quadrados
  float sumSqX = 0, sumSqY = 0, sumSqZ = 0;
  for (int i = 0; i < NUM_AMOSTRAS; i++) {
    sumSqX += pow(bufferX[i] - meanX, 2);
    sumSqY += pow(bufferY[i] - meanY, 2);
    sumSqZ += pow(bufferZ[i] - meanZ, 2);
  }

  // Passo D: Retornar RMS por referência
  rmsX = sqrt(sumSqX / NUM_AMOSTRAS);
  rmsY = sqrt(sumSqY / NUM_AMOSTRAS);
  rmsZ = sqrt(sumSqZ / NUM_AMOSTRAS);
}