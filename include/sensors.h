#pragma once
#include <Arduino.h>

/**
 * MÓDULO DE SENSORES - Calisto
 * Interface pública para leitura de temperatura e vibração
 * 
 * Hardware:
 *   - Vibração: MPU9250_asukiaaa (acelerômetro I2C 9-eixos)
 *   - Temperatura: MAX6675 + Termopar Tipo K (SPI)
 */

/**
 * Inicializa os sensores (MPU9250 e MAX6675)
 */
void setupSensors();

/**
 * Lê a temperatura do termopar via MAX6675
 * @return Temperatura em °C (ou 0.0 se falha na leitura)
 */
float getTemperature();

/**
 * Calcula o RMS (Root Mean Square) de vibração em cada eixo
 * Remove a componente DC (gravidade) e retorna a energia vibracional pura
 * 
 * @param rmsX Referência para armazenar RMS do eixo X (g's)
 * @param rmsY Referência para armazenar RMS do eixo Y (g's)
 * @param rmsZ Referência para armazenar RMS do eixo Z (g's)
 * 
 * Nota: Em repouso sobre superfície estável, RMS deve estar próximo a 0 g
 */
void getVibrationRMS(float &rmsX, float &rmsY, float &rmsZ);