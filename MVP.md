# Projeto Calisto

### Sistema de Monitoramento de Condensadores de Ar Condicionado via IoT

---

**Instituição:** Universidade Federal do Rio Grande do Norte (UFRN) — Instituto Metrópole Digital (IMD)  
**Local de Implementação:** NPITI  
**Equipe de Desenvolvimento:** Franklin Soares, Hebert Franla, Francisco Matheus

---

## Sumário

- [Projeto Calisto](#projeto-calisto)
    - [Sistema de Monitoramento de Condensadores de Ar Condicionado via IoT](#sistema-de-monitoramento-de-condensadores-de-ar-condicionado-via-iot)
    - [Sumário](#sumário)
    - [Visão Geral](#visão-geral)
    - [Matriz de Requisitos](#matriz-de-requisitos)
    - [Arquitetura e Hardware](#arquitetura-e-hardware)
    - [Pilha de Protocolos e Comunicação](#pilha-de-protocolos-e-comunicação)
    - [Processamento de Sinais e Telemetria (Versão MVP)](#processamento-de-sinais-e-telemetria-versão-mvp)
        - [5.1. Cálculo do RMS Dinâmico (Remoção da Gravidade)](#51-cálculo-do-rms-dinâmico-remoção-da-gravidade)
        - [5.2. Delegação Lógica para a Nuvem (Adafruit IO)](#52-delegação-lógica-para-a-nuvem-adafruit-io)
    - [Estrutura de Nuvem e Alertas](#estrutura-de-nuvem-e-alertas)
        - [Feeds Adafruit IO](#feeds-adafruit-io)
        - [Regras de Alerta (Actions)](#regras-de-alerta-actions)
    - [Planejamento de Execução (Foco em Implantação Rápida)](#planejamento-de-execução-foco-em-implantação-rápida)
        - [Fase 1 — Prototipagem Básica (Bench Test)](#fase-1--prototipagem-básica-bench-test)
        - [Fase 2 — Integração Cloud e Lógica de Alertas](#fase-2--integração-cloud-e-lógica-de-alertas)
        - [Fase 3 — Implantação Física (NPITI)](#fase-3--implantação-física-npiti)

---

## Visão Geral

O **Projeto Calisto** é um sistema de Internet das Coisas (IoT) voltado para a **manutenção preditiva de equipamentos de refrigeração**. O objetivo principal é acoplar módulos de sensoriamento nos condensadores de ar condicionado do NPITI para realizar a **coleta em tempo real de dados de temperatura e vibração**.

Esses dados são cruciais para identificar anomalias operacionais — como desgaste de compressores, obstrução de ventilação ou superaquecimento — **antes que resultem em falhas críticas**.

---

## Matriz de Requisitos

| ID    | Requisito           | Descrição                                                                                                                               | Status      |
| ----- | ------------------- | --------------------------------------------------------------------------------------------------------------------------------------- | ----------- |
| RF01  | Leitura de Sensores | Coleta de vibração (MPU6050) e temperatura (MAX6675 + Termopar Tipo K)                                                                  | ✅ Atendido |
| RF02  | Processamento Local | Pré-processamento no microcontrolador: remoção de nível DC e cálculo de RMS dinâmico por janela de amostras                             | ✅ Atendido |
| RF03  | Conexão e Envio     | Conexão via Wi-Fi e transmissão padronizada para a nuvem utilizando MQTT sobre TCP/IP                                                   | ✅ Atendido |
| RF04  | Dashboards          | Interface gráfica em tempo real utilizando a plataforma Adafruit IO                                                                     | ✅ Atendido |
| RF05  | Emissão de Alertas  | Sistema automatizado de notificações por e-mail para valores anômalos, com regra híbrida de temperatura e vibração configurada na nuvem | ✅ Atendido |
| RNF01 | Hardware Específico | Utilização do microcontrolador ESP32 como núcleo de processamento                                                                       | ✅ Atendido |

---

## Arquitetura e Hardware

A arquitetura foi projetada com foco em **modularidade, baixo custo e estabilidade** para operação contínua.

| Componente                  | Descrição                                                                                              |
| --------------------------- | ------------------------------------------------------------------------------------------------------ |
| **Microcontrolador (Core)** | ESP32 — processamento, conectividade nativa e hospedagem do Access Point                               |
| **Sensor de Vibração**      | MPU6050 — acelerômetro/giroscópio via I2C; leitura de frequência e amplitude do chassi                 |
| **Sensor de Temperatura**   | Módulo MAX6675 + Termopar Tipo K — comunicação SPI, alta robustez industrial para tubulação/compressor |
| **Alimentação**             | Fonte DC 5V da rede local com regulador para 3.3V — operação sem dependência de baterias               |

---

## Pilha de Protocolos e Comunicação

O fluxo de comunicação do condensador até a nuvem é mapeado no **Modelo TCP/IP**:

```
┌──────────────────────────┬──────────────────────────────────┐
│  CAMADA DE APLICAÇÃO     │  MQTT (Publish/Subscribe)        │
│                          │  Dados em JSON → Adafruit IO     │
│                          │  Porta 1883 (TCP) / 8883 (TLS)   │
├──────────────────────────┼──────────────────────────────────┤
│  CAMADA DE TRANSPORTE    │  TCP                             │
│                          │  Integridade, ordem e            │
│                          │  retransmissão de pacotes        │
├──────────────────────────┼──────────────────────────────────┤
│  CAMADA DE REDE          │  IPv4                            │
│                          │  Endereçamento via DHCP local    │
│                          │  Roteamento até a nuvem          │
├──────────────────────────┼──────────────────────────────────┤
│  CAMADA FÍSICA/ENLACE    │  Wi-Fi 802.11 (2.4 GHz)          │
│                          │  Infraestrutura de APs do NPITI  │
└──────────────────────────┴──────────────────────────────────┘
```

> **Nota:** A utilização da infraestrutura Wi-Fi já existente no local elimina a necessidade de gateways adicionais.

---

## Processamento de Sinais e Telemetria (Versão MVP)

Nesta etapa de Produto Mínimo Viável, a arquitetura de firmware foi simplificada para acelerar a implantação no NPITI. O ESP32 atua estritamente como um **nó de telemetria**, sem processar regras lógicas de estado ou análises de frequência (FFT).

O processamento local foca apenas na extração da **energia de vibração bruta** para envio.

---

### 5.1. Cálculo do RMS Dinâmico (Remoção da Gravidade)

O único pré-processamento necessário no microcontrolador é a remoção da aceleração da gravidade (≈ 1g) lida pelo MPU6050, isolando a vibração mecânica do condensador.

O algoritmo roda diretamente no `loop()` principal, de forma **blocante e simples**:

1. O ESP32 coleta uma **janela pequena de amostras** (ex: 100 leituras com um pequeno `delay()` entre elas).
2. Calcula-se a **média** dessas 100 leituras (que representará a componente estática e a inclinação do eixo).
3. **Subtrai-se a média** de cada leitura e calcula-se o **RMS** (_Root Mean Square_) do resultado.
4. O valor final (**RMS dinâmico em g**) representa a intensidade total da vibração e é enviado à nuvem.

O pipeline de dados resultante é:

```
Leituras brutas (I2C) → Buffer de 100 amostras → Subtração da média → RMS dinâmico → MQTT
```

> **Verificação rápida:** Com o sensor em repouso sobre uma superfície estável, o RMS dinâmico deve resultar em ≈ **0 g**. Qualquer valor acima de `0,02 g` em repouso indica ruído elétrico ou vibração ambiental, e deve ser considerado no ajuste dos limiares.

---

### 5.2. Delegação Lógica para a Nuvem (Adafruit IO)

Todo o processamento de "Gate de Estado" (identificar se o compressor está ligado ou desligado) foi **transferido para a Adafruit IO**.

O ESP32 **não toma decisões**. Ele publica a Temperatura e a Vibração a cada 10 segundos. Na plataforma Adafruit, configuram-se as **Actions (Gatilhos)** com lógica simples:

- **Regra de Alerta Híbrida:** `SE Temperatura > 35°C` _(indica compressor ativo)_ **E** `Vibração RMS > Limiar_Crítico` por mais de **5 minutos** `ENTÃO` dispara e-mail de manutenção.

Essa delegação oferece três vantagens práticas:

| Vantagem                           | Detalhe                                                                            |
| ---------------------------------- | ---------------------------------------------------------------------------------- |
| **Menor complexidade de firmware** | Sem variáveis globais de estado nem lógica condicional no microcontrolador         |
| **Ajuste sem reprogramação**       | Os limiares de alerta são editados direto no painel da Adafruit IO, pelo navegador |
| **Menor uso de memória no ESP32**  | A RAM economizada pode ser usada pelo buffer de amostras e pela pilha MQTT/Wi-Fi   |

---

## Estrutura de Nuvem e Alertas

### Feeds Adafruit IO

| Feed             | Descrição                                   | Unidade  | Frequência |
| ---------------- | ------------------------------------------- | -------- | ---------- |
| `calisto-temp`   | Temperatura do condensador/tubulação        | °C       | A cada 10s |
| `calisto-vib-x`  | RMS dinâmico de aceleração no eixo X        | g        | A cada 10s |
| `calisto-vib-y`  | RMS dinâmico de aceleração no eixo Y        | g        | A cada 10s |
| `calisto-vib-z`  | RMS dinâmico de aceleração no eixo Z        | g        | A cada 10s |
| `calisto-status` | Status da conexão (1 = Online, 0 = Offline) | Booleano | A cada 30s |

> **Nota:** O feed `calisto-vib-fft-peak` foi removido nesta versão MVP. A análise por FFT poderá ser reintroduzida em uma iteração futura do projeto.

### Regras de Alerta (Actions)

Utilizando os recursos de automação da **Adafruit IO** (**RF05**):

- 🌡️ **Alerta Térmico Isolado:** Disparo de e-mail automático se `calisto-temp` superar o limiar crítico (ex: `> 80°C` na descarga do compressor) por mais de **2 minutos consecutivos**.
- ⚠️ **Alerta de Vibração Anômala (Regra Híbrida):** Disparo de e-mail se `calisto-temp > 35°C` **E** RMS dinâmico superar o limiar operacional por mais de **5 minutos** — garantindo que o alerta só ocorra com o compressor ativo.
- 📡 **Alerta de Conectividade:** Disparo de e-mail se `calisto-status` não receber publicação por mais de **5 minutos**, indicando perda de conexão do dispositivo.

---

## Planejamento de Execução (Foco em Implantação Rápida)

O ciclo de desenvolvimento foi reduzido para **três fases**, eliminando portais de configuração (Captive Portal) em favor de credenciais Wi-Fi fixas (_hardcoded_) nesta versão inicial.

```
Fase 1 (Semanas 1-2)       Fase 2 (Semanas 3-4)       Fase 3 (Semanas 5-6)
       │                          │                          │
       ▼                          ▼                          ▼
 Prototipagem Básica        Integração Cloud         Implantação Física
    (Bench Test)             (Adafruit IO)            (Condensador NPITI)
```

---

### Fase 1 — Prototipagem Básica (Bench Test)

- Montagem do ESP32, MPU6050 e MAX6675 em protoboard.
- Desenvolvimento do código sequencial simples (`loop` padrão, sem interrupções de hardware).
- Implementação do algoritmo de **RMS Dinâmico** e verificação via Monitor Serial.
    - Critério de aprovação: sensor em repouso deve ler **≈ 0 g**.
- Inserção fixa (_hardcoded_) do SSID e Senha da rede Wi-Fi do laboratório no código.

---

### Fase 2 — Integração Cloud e Lógica de Alertas

- Criação dos **Feeds** na Adafruit IO (Temperatura e Eixos de Vibração).
- Implementação do **cliente MQTT** no ESP32 para publicação a cada 10 segundos.
- Criação dos **Dashboards** para visualização em tempo real.
- Configuração das **Actions (Gatilhos de E-mail)** na nuvem, combinando limiares de temperatura e vibração simulados na bancada.
- Definição empírica dos valores de `Limiar_Crítico` de vibração com base nas leituras coletadas em bancada.

---

### Fase 3 — Implantação Física (NPITI)

- Acondicionamento da eletrônica em **case IP65** (resistente a intempéries).
- Fixação rígida do acelerômetro na carcaça do condensador (via ímã de neodímio ou fita dupla-face acrílica industrial) e do Termopar na tubulação de descarga.
- **Monitoramento passivo** durante os primeiros dias para identificar os valores normais de operação.
- Refinamento dos limiares de alerta diretamente no painel da **Adafruit IO**, sem necessidade de reprogramação do dispositivo.

---

_Documentação gerada para o Projeto Calisto — UFRN / IMD_
