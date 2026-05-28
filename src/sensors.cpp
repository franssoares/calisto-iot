/**
 * MÓDULO DE SENSORES - Projeto Calisto
 * Responsável pela leitura de vibração (MPU9250) e temperatura (MAX6675)
 * 
 * Sensor de Vibração: MPU9250_asukiaaa
 * - Acelerômetro 9-eixos (X, Y, Z) com giroscópio integrado
 * - Comunicação: I2C (Wire)
 * - Resolução: 16-bit
 * - Retorna dados já em g's (gravitação)
 * - Biblioteca: https://github.com/asukiaaa/MPU9250_asukiaaa
 * 
 * Sensor de Temperatura: MAX6675 + Termopar Tipo K
 * - Comunicação: SPI
 * - Resolução: 0.25°C
 * - Faixa: 0-1024°C
 */

#include "sensors.h"
#include "config.h"
#include <Wire.h>
#include <MPU9250_asukiaaa.h>
#include <max6675.h>

// Instâncias locais (não poluem o escopo global)
MPU9250_asukiaaa mpu;  // Instância do MPU9250 via biblioteca asukiaaa
MAX6675 thermocouple(THERMO_SCK, THERMO_CS, THERMO_SO);

/**
 * Inicializa todos os sensores
 * - MPU9250: Inicia o acelerômetro via I2C
 * - MAX6675: Prepara o módulo SPI para leitura de termopar
 */
void setupSensors() {
  mpu.beginAccel();  // Ativa o acelerômetro do MPU9250
  Serial.println("[OK] MPU9250_asukiaaa Configurado com sucesso.");
}

/**
 * Lê a temperatura do termopar via MAX6675
 * @return Temperatura em °C (ou 0.0 se falha na leitura)
 */
float getTemperature() {
  float tempC = thermocouple.readCelsius();
  if (isnan(tempC)) {
    Serial.println("[ERRO] Falha ao ler MAX6675.");
    return 0.0;
  }
  return tempC;
}

/**
 * Calcula o RMS (Root Mean Square) de vibração em cada eixo
 * Algoritmo:
 *   1. Coleta NUM_AMOSTRAS leituras do MPU9250
 *   2. Calcula a média (remove nível DC - gravidade e inclinação)
 *   3. Retorna o RMS por referência para cada eixo (X, Y, Z)
 * 
 * Parâmetros:
 *   rmsX, rmsY, rmsZ: Referências para armazenar os resultados em g's
 * 
 * Nota: MPU9250_asukiaaa já retorna os dados em g's (gravitação)
 */
void getVibrationRMS(float &rmsX, float &rmsY, float &rmsZ) {
  float bufferX[NUM_AMOSTRAS], bufferY[NUM_AMOSTRAS], bufferZ[NUM_AMOSTRAS];
  float sumX = 0, sumY = 0, sumZ = 0;

  // Passo A: Coletar NUM_AMOSTRAS leituras e acumular
  for (int i = 0; i < NUM_AMOSTRAS; i++) {
    mpu.accelUpdate();  // Atualiza os dados do acelerômetro via I2C
    bufferX[i] = mpu.accelX();  // Retorna valor em g (já normalizado)
    bufferY[i] = mpu.accelY();
    bufferZ[i] = mpu.accelZ();

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