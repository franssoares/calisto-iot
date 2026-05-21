# Projeto Calisto

### Sistema de Monitoramento de Condensadores de Ar Condicionado via IoT

---

**Instituição:** Universidade Federal do Rio Grande do Norte (UFRN) — Instituto Metrópole Digital (IMD)  
**Local de Implementação:** NPITI  
**Equipe de Desenvolvimento:** Franklin Soares, Hebert Franla, Francisco Matheus

---

## Sumário

1. [Visão Geral](#visão-geral)
2. [Matriz de Requisitos](#matriz-de-requisitos)
3. [Arquitetura e Hardware](#arquitetura-e-hardware)
4. [Pilha de Protocolos e Comunicação](#pilha-de-protocolos-e-comunicação)
5. [Processamento de Sinais e Cálculo de Vibração](#processamento-de-sinais-e-cálculo-de-vibração)
6. [Estrutura de Nuvem e Alertas](#estrutura-de-nuvem-e-alertas)
7. [Planejamento de Execução](#planejamento-de-execução)

---

## Visão Geral

O **Projeto Calisto** é um sistema de Internet das Coisas (IoT) voltado para a **manutenção preditiva de equipamentos de refrigeração**. O objetivo principal é acoplar módulos de sensoriamento nos condensadores de ar condicionado do NPITI para realizar a **coleta em tempo real de dados de temperatura e vibração**.

Esses dados são cruciais para identificar anomalias operacionais — como desgaste de compressores, obstrução de ventilação ou superaquecimento — **antes que resultem em falhas críticas**.

---

## Matriz de Requisitos

| ID    | Requisito           | Descrição                                                                                                                            | Status      |
| ----- | ------------------- | ------------------------------------------------------------------------------------------------------------------------------------ | ----------- |
| RF01  | Leitura de Sensores | Coleta de vibração (MPU6050) e temperatura (MAX6675 + Termopar Tipo K)                                                               | ✅ Atendido |
| RF02  | Processamento Local | Pré-processamento no microcontrolador: remoção de nível DC, cálculo de RMS dinâmico e FFT com amostragem por interrupção de hardware | ✅ Atendido |
| RF03  | Conexão e Envio     | Conexão via Wi-Fi e transmissão padronizada para a nuvem utilizando MQTT sobre TCP/IP                                                | ✅ Atendido |
| RF04  | Dashboards          | Interface gráfica em tempo real utilizando a plataforma Adafruit IO                                                                  | ✅ Atendido |
| RF05  | Emissão de Alertas  | Sistema automatizado de notificações por e-mail para valores anômalos, com gate de estado para distinção compressor ligado/desligado | ✅ Atendido |
| RNF01 | Hardware Específico | Utilização do microcontrolador ESP32 como núcleo de processamento                                                                    | ✅ Atendido |

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

## Processamento de Sinais e Cálculo de Vibração

Esta seção detalha as decisões de engenharia de firmware necessárias para garantir que os dados de vibração transmitidos sejam **fisicamente significativos e confiáveis**. Três problemas críticos foram identificados e endereçados.

---

### 1. Remoção do Nível DC (Gravidade) antes do RMS

**O problema:** O MPU6050 mede aceleração total, que inclui a componente estática da gravidade (≈ 1g = 9,81 m/s²). Dependendo da orientação de montagem do sensor, a gravidade domina qualquer eixo e mascara completamente a componente de vibração real. Aplicar o RMS diretamente no valor bruto produz um número sem significado preditivo.

**A solução — RMS Dinâmico (equivalente ao Desvio Padrão):**

Dentro de cada janela de amostragem de N leituras, o processamento segue três etapas:

**Etapa 1 — Calcule a média da janela** (representa a componente estática: gravidade + inclinação do eixo):

$$\bar{x} = \frac{1}{N} \sum_{i=1}^{N} x_i$$

**Etapa 2 — Subtraia a média de cada leitura**, isolando apenas a componente dinâmica (a vibração real):

$$x_{\text{dinâmico}_i} = x_i - \bar{x}$$

**Etapa 3 — Aplique o RMS somente na componente dinâmica:**

$$X_{\text{rms}} = \sqrt{\frac{1}{N} \sum_{i=1}^{N} (x_i - \bar{x})^2}$$

> **Nota estatística:** Esta operação é matematicamente equivalente ao **desvio padrão** do sinal na janela de tempo — uma medida direta da energia de vibração, livre da gravidade.

**Pipeline de dados atualizado:**

```
Leituras brutas (I2C) → Buffer (janela N) → Subtração da média → RMS dinâmico → MQTT
```

---

### 2. Gate de Estado para Baseline e Alertas (Ciclos do Compressor)

**O problema:** Ar-condicionado não opera em regime contínuo. O compressor liga e desliga conforme o termostato atua, alternando entre dois estados com assinaturas de vibração completamente distintas: compressor ativo (vibração alta) e ventilador apenas ou inativo (vibração baixa/zero). Uma baseline calculada sobre 7 dias corridos será puxada para baixo pelos períodos de inatividade, tornando o limiar de alerta impreciso.

**A solução — Gate Lógico de Estado:**

Antes de alimentar a baseline histórica ou avaliar qualquer regra de severidade, o firmware (ou a lógica na nuvem) deve confirmar o estado operacional do equipamento.

```
┌─────────────────────────────────────────────────────────────────┐
│  Leitura do ciclo:  RMS_dinâmico  e  Temperatura (MAX6675)      │
└────────────────────────┬────────────────────────────────────────┘
                         │
              ┌──────────▼──────────┐
              │  RMS > limiar_min   │   (ex: 0.05 g)
              │  OU                 │
              │  Temp > limiar_min  │   (ex: 35°C)
              └──────────┬──────────┘
               NÃO       │       SIM
                │        │        │
                ▼        │        ▼
         Descarta:        │   Alimenta baseline
         compressor       │   Avalia severidade
         desligado        │   Dispara alertas
```

- **`limiar_min` de vibração** (ex: `0,05 g`): valor abaixo do qual o compressor é considerado parado.
- **`limiar_min` de temperatura** (ex: `35°C`): confirma operação ativa por via térmica, útil como segundo critério.
- Os dois critérios podem ser combinados com lógica `OU` para maior robustez.

---

### 3. Amostragem por Hardware Timer para FFT

**O problema:** A biblioteca `arduinoFFT` é viável no ESP32 (processador Xtensa dual-core), mas a Transformada de Fourier **exige frequência de amostragem (fs) estritamente constante**. Ler o MPU6050 dentro do `loop()` com `delay()` introduz _jitter_ — variações no tempo entre as leituras — causado pelo tempo de execução do código e pelas tarefas da pilha Wi-Fi/MQTT. Um fs variável destrói os bins de frequência, gerando vazamento espectral (_spectral leakage_) e tornando os resultados da FFT não confiáveis.

**A solução — Interrupção por Hardware Timer:**

```
┌──────────────────────────────────────────────────────────────────┐
│  Hardware Timer (ESP32)  →  ISR dispara a fs fixo (ex: 1000 Hz)  │
│  ├── Lê MPU6050 via I2C                                          │
│  └── Armazena amostra em buffer circular (512 ou 1024 pontos)    │
│                                                                  │
│  loop() principal                                                │
│  ├── Monitora flag "buffer cheio"                                │
│  ├── Executa FFT (arduinoFFT) na cópia do buffer                 │
│  ├── Identifica frequência dominante e amplitude                 │
│  └── Publica resultado via MQTT                                  │
└──────────────────────────────────────────────────────────────────┘
```

**Parâmetros de referência:**

| Parâmetro                  | Valor recomendado    | Justificativa                                                       |
| -------------------------- | -------------------- | ------------------------------------------------------------------- |
| `fs` (freq. de amostragem) | 1000 Hz              | Cobre vibrações mecânicas até 500 Hz pelo critério de Nyquist       |
| Tamanho do buffer (N)      | 512 ou 1024 amostras | Potência de 2 — obrigatório para o algoritmo FFT (Cooley-Tukey)     |
| Resolução de frequência    | fs / N = ~1–2 Hz     | Suficiente para distinguir frequências de compressores e exaustores |

> **Regra prática:** O tamanho do buffer deve ser sempre uma **potência de 2** (256, 512, 1024…). Nunca use `delay()` para controlar o tempo de amostragem quando a FFT estiver envolvida.

---

## Estrutura de Nuvem e Alertas

### Feeds Adafruit IO

| Feed                   | Descrição                                   | Unidade  | Frequência       |
| ---------------------- | ------------------------------------------- | -------- | ---------------- |
| `calisto-temp`         | Temperatura do condensador/tubulação        | °C       | A cada 10s       |
| `calisto-vib-x`        | RMS dinâmico de aceleração no eixo X        | g        | A cada 10s       |
| `calisto-vib-y`        | RMS dinâmico de aceleração no eixo Y        | g        | A cada 10s       |
| `calisto-vib-z`        | RMS dinâmico de aceleração no eixo Z        | g        | A cada 10s       |
| `calisto-vib-fft-peak` | Frequência dominante identificada pela FFT  | Hz       | A cada ciclo FFT |
| `calisto-status`       | Status da conexão (1 = Online, 0 = Offline) | Booleano | A cada 30s       |

> **Importante:** Os feeds `calisto-vib-*` publicam o **RMS dinâmico** (componente AC, sem gravidade), conforme descrito na Seção 5.1. Os alertas abaixo só são avaliados quando o **gate de estado** confirmar o compressor ativo (Seção 5.2).

### Regras de Alerta (Actions)

Utilizando os recursos de automação da **Adafruit IO** (**RF05**):

- 🌡️ **Alerta Térmico:** Disparo de e-mail automático se `calisto-temp` superar o limiar crítico (ex: `> 80°C` na descarga do compressor) por mais de **2 minutos consecutivos** — **somente quando o gate de estado indicar compressor ativo**.
- ⚠️ **Alerta de Vibração:** Disparo de e-mail se o RMS dinâmico apresentar picos consistentes **fora do desvio padrão operacional da baseline**, indicando desbalanceamento do exaustor ou soltura mecânica — **somente quando o gate de estado indicar compressor ativo**.
- 📡 **Alerta de Frequência Anômala:** Disparo de e-mail se `calisto-vib-fft-peak` apresentar desvio relevante da frequência fundamental esperada do compressor, podendo indicar desgaste de rolamentos ou desbalanceamento.

---

## Planejamento de Execução

```
Fase 1              Fase 2              Fase 3              Fase 4
  │                   │                   │                   │
  ▼                   ▼                   ▼                   ▼
Prototipagem     Integração Cloud    Implantação Física   Captive Portal
(Bench Test)                            (NPITI)            (WiFiManager)
```

### Fase 1 — Prototipagem (Bench Test)

- Montagem do hardware em protoboard
- Codificação inicial no **PlatformIO**
- Calibração dos sensores (MPU6050 e MAX6675) via monitor serial
- Validação do **RMS dinâmico** com remoção de DC: comparar leitura estática (deve resultar em ≈ 0 g) com vibração induzida manualmente
- Validação da **amostragem por Hardware Timer**: confirmar fs constante medindo intervalo entre interrupções via osciloscópio ou GPIO toggle

### Fase 2 — Integração Cloud

- Configuração dos dashboards na Adafruit IO (incluindo feed de pico FFT)
- Programação da pilha MQTT no ESP32
- Implementação e teste do **gate de estado** (compressor ligado/desligado)
- Definição empírica dos limiares `limiar_min` de vibração e temperatura
- Validação do tráfego de rede

### Fase 3 — Implantação Física

- Acondicionamento da eletrônica em **case IP65** (resistente a intempéries)
- Fixação não destrutiva no condensador do NPITI
- Fixação do Termopar na tubulação alvo
- Coleta da **baseline de 7 dias** com gate de estado ativo para garantir que apenas ciclos de compressor ativo alimentem o histórico

### Fase 4 — Portal de Configuração (Captive Portal)

- Implementação da biblioteca **WiFiManager**
- Em caso de perda de acesso à rede (ex: troca de senha), o ESP32 levanta a rede **`Calisto-Config`**
- A equipe de manutenção insere novas credenciais via **navegador de smartphone**
- Sem necessidade de reprogramação por cabo

---

_Documentação gerada para o Projeto Calisto — UFRN / IMD_
