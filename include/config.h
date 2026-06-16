#pragma once

// ==========================================
// REDE E NUVEM (ADAFRUIT IO)
// ==========================================
#define WIFI_SSID     "NOME_DO_WIFI"
#define WIFI_PASSWORD "SENHA_DO_WIFI"

#define MQTT_SERVER   "io.adafruit.com"
#define MQTT_PORT     1883
#define IO_USERNAME   "SEU_USUARIO_ADAFRUIT"
#define IO_KEY        "SUA_CHAVE_AIO_KEY"

// ==========================================
// PINAGEM DO HARDWARE
// ==========================================
// MAX6675 (SPI)
#define THERMO_SO     19
#define THERMO_CS     15
#define THERMO_SCK    18

// ==========================================
// PARÂMETROS DE PROCESSAMENTO
// ==========================================
#define NUM_AMOSTRAS      100    // Tamanho da janela para o RMS Dinâmico
#define DELAY_AMOSTRA     2      // ms entre leituras (Janela total = ~200ms)
#define PUBLISH_INTERVAL  20000  // Publica a cada 10 segundos