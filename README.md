# Projeto Calisto
### Sistema de Monitoramento de Condensadores de Ar Condicionado via IoT

---

**Instituição:** Universidade Federal do Rio Grande do Norte (UFRN) — Instituto Metrópole Digital (IMD)  
**Local de Implementação:** NPITI  
**Equipe de Desenvolvimento:** Franklin Soares, Hebert, Francisco

---

## Sumário

1. [Visão Geral](#visão-geral)
2. [Matriz de Requisitos](#matriz-de-requisitos)
3. [Arquitetura e Hardware](#arquitetura-e-hardware)
4. [Pilha de Protocolos e Comunicação](#pilha-de-protocolos-e-comunicação)
5. [Estrutura de Nuvem e Alertas](#estrutura-de-nuvem-e-alertas)
6. [Planejamento de Execução](#planejamento-de-execução)

---

## Visão Geral

O **Projeto Calisto** é um sistema de Internet das Coisas (IoT) voltado para a **manutenção preditiva de equipamentos de refrigeração**. O objetivo principal é acoplar módulos de sensoriamento nos condensadores de ar condicionado do NPITI para realizar a **coleta em tempo real de dados de temperatura e vibração**.

Esses dados são cruciais para identificar anomalias operacionais — como desgaste de compressores, obstrução de ventilação ou superaquecimento — **antes que resultem em falhas críticas**.

---

## Matriz de Requisitos

| ID | Requisito | Descrição | Status |
|----|-----------|-----------|--------|
| RF01 | Leitura de Sensores | Coleta de vibração (MPU6050) e temperatura (MAX6675 + Termopar Tipo K) | ✅ Atendido |
| RF02 | Processamento Local | Aplicação de filtros digitais (média móvel) no microcontrolador antes da transmissão | ✅ Atendido |
| RF03 | Conexão e Envio | Conexão via Wi-Fi e transmissão padronizada para a nuvem utilizando MQTT sobre TCP/IP | ✅ Atendido |
| RF04 | Dashboards | Interface gráfica em tempo real utilizando a plataforma Adafruit IO | ✅ Atendido |
| RF05 | Emissão de Alertas | Sistema automatizado de notificações por e-mail para valores anômalos | ✅ Atendido |
| RNF01 | Hardware Específico | Utilização do microcontrolador ESP32 como núcleo de processamento | ✅ Atendido |

---

## Arquitetura e Hardware

A arquitetura foi projetada com foco em **modularidade, baixo custo e estabilidade** para operação contínua.

| Componente | Descrição |
|------------|-----------|
| **Microcontrolador (Core)** | ESP32 — processamento, conectividade nativa e hospedagem do Access Point |
| **Sensor de Vibração** | MPU6050 — acelerômetro/giroscópio via I2C; leitura de frequência e amplitude do chassi |
| **Sensor de Temperatura** | Módulo MAX6675 + Termopar Tipo K — comunicação SPI, alta robustez industrial para tubulação/compressor |
| **Alimentação** | Fonte DC 5V da rede local com regulador para 3.3V — operação sem dependência de baterias |

### Processamento na Borda (Edge Computing)

Para otimizar o uso da banda de rede e garantir a consistência dos dados (**RF02**), o ESP32 realiza um pré-processamento local:

- O acelerômetro é sensível a ruídos efêmeros, tornando o filtro essencial
- O firmware implementa um **Filtro de Média Móvel**
- O dispositivo acumula um buffer de leituras em uma **janela de 1 segundo**
- Calcula a média aritmética e transmite apenas o **valor consolidado**

```
Leituras brutas → Buffer (1s) → Média Móvel → Transmissão MQTT
```

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

## Estrutura de Nuvem e Alertas

### Feeds Adafruit IO

| Feed | Descrição | Unidade | Frequência |
|------|-----------|---------|------------|
| `calisto-temp` | Temperatura do condensador/tubulação | °C | A cada 10s |
| `calisto-vib-x` | Aceleração no eixo X | g (ou m/s²) | A cada 10s |
| `calisto-vib-y` | Aceleração no eixo Y | g (ou m/s²) | A cada 10s |
| `calisto-vib-z` | Aceleração no eixo Z | g (ou m/s²) | A cada 10s |
| `calisto-status` | Status da conexão (1 = Online, 0 = Offline) | Booleano | A cada 30s |

### Regras de Alerta (Actions)

Utilizando os recursos de automação da **Adafruit IO** (**RF05**):

- 🌡️ **Alerta Térmico:** Disparo de e-mail automático se `calisto-temp` superar o limiar crítico (ex: `> 80°C` na descarga do compressor) por mais de **2 minutos consecutivos**.
- ⚠️ **Alerta de Vibração:** Disparo de e-mail se as médias de aceleração apresentarem picos consistentes **fora do desvio padrão operacional**, indicando desbalanceamento do exaustor ou soltura mecânica.

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

### Fase 2 — Integração Cloud

- Configuração dos dashboards na Adafruit IO
- Programação da pilha MQTT no ESP32
- Validação do tráfego de rede

### Fase 3 — Implantação Física

- Acondicionamento da eletrônica em **case IP65** (resistente a intempéries)
- Fixação não destrutiva no condensador do NPITI
- Fixação do Termopar na tubulação alvo

### Fase 4 — Portal de Configuração (Captive Portal)

- Implementação da biblioteca **WiFiManager**
- Em caso de perda de acesso à rede (ex: troca de senha), o ESP32 levanta a rede **`Calisto-Config`**
- A equipe de manutenção insere novas credenciais via **navegador de smartphone**
- Sem necessidade de reprogramação por cabo

---

*Documentação gerada para o Projeto Calisto — UFRN / IMD*