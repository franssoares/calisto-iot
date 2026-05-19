# Diretrizes de Contribuição — Projeto Calisto

---

## Sumário

- [Diretrizes de Contribuição — Projeto Calisto](#diretrizes-de-contribuição--projeto-calisto)
  - [Sumário](#sumário)
  - [Estratégia de Branches](#estratégia-de-branches)
  - [Fluxo de Trabalho](#fluxo-de-trabalho)
    - [Exemplos de Commits](#exemplos-de-commits)
  - [Setup de Desenvolvimento](#setup-de-desenvolvimento)
    - [Pré-requisitos](#pré-requisitos)
    - [Configuração do Ambiente](#configuração-do-ambiente)
  - [Padrões de Código](#padrões-de-código)
  - [Diretrizes Específicas](#diretrizes-específicas)
    - [Firmware (ESP32)](#firmware-esp32)
    - [Dashboards (Adafruit IO)](#dashboards-adafruit-io)
  - [Testes](#testes)
    - [Teste de Bancada (Bench Test)](#teste-de-bancada-bench-test)
    - [Simulação de Sensores](#simulação-de-sensores)

---

## Estratégia de Branches

- A branch `main` é **protegida**. Commits diretos são proibidos — todo código deve ser testado em protoboard/bancada antes de ir para produção (condensadores).
- Todas as alterações devem ser enviadas via **Pull Request (PR)** apontando para a branch `main`.
- Utilize o padrão **GitFlow** para nomenclatura de branches:

| Prefixo     | Uso                                            | Exemplo                      |
| ----------- | ---------------------------------------------- | ---------------------------- |
| `feature/`  | Novas implementações (ex: novo sensor)         | `feature/sensor-umidade`     |
| `bugfix/`   | Correção de falhas lógicas ou de conectividade | `bugfix/reconexao-mqtt`      |
| `docs/`     | Atualizações na documentação                   | `docs/atualiza-pinagem`      |
| `hardware/` | Alterações de esquemáticos e pinagem           | `hardware/cs-max6675-gpio15` |

---

## Fluxo de Trabalho

**1. Atualize sua `main` local:**

```bash
git checkout main
git pull origin main
```

**2. Crie sua branch de trabalho:**

```bash
git checkout -b <tipo>/<descricao>
```

**3. Faça commits lógicos** seguindo o padrão [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/):

```bash
git add <arquivos>
git commit -m "<tipo>(<escopo>): <descricao>"
```

> Tipos válidos: `feat`, `fix`, `docs`, `refactor`, `hardware`

**4. Envie sua branch e abra o Pull Request:**

```bash
git push -u origin <tipo>/<descricao>
```

### Exemplos de Commits

```bash
feat(mqtt): implementa reconexão automática ao broker
fix(mpu6050): ajusta calibração do offset no eixo Z
hardware(pinagem): altera pino CS do MAX6675 para GPIO15
docs(readme): atualiza tabela de feeds do Adafruit IO
```

---

## Setup de Desenvolvimento

### Pré-requisitos

- **PlatformIO** integrado ao VS Code — ambiente de desenvolvimento oficial do projeto
- Conta configurada no **Adafruit IO** com as chaves `IO_USERNAME` e `IO_KEY` em mãos
- **Bibliotecas gerenciadas automaticamente** pelo PlatformIO via `platformio.ini`:

| Biblioteca         | Finalidade                                  |
| ------------------ | ------------------------------------------- |
| `WiFiManager`      | Captive portal para configuração de rede    |
| `PubSubClient`     | Pilha MQTT para comunicação com Adafruit IO |
| `Adafruit_MPU6050` | Driver do acelerômetro/giroscópio           |
| `MAX6675`          | Driver do módulo de leitura do termopar     |

### Configuração do Ambiente

**1.** Clone o repositório:

```bash
git clone <url-do-repositorio>
```

**2.** Abra a pasta no VS Code — o PlatformIO reconhecerá o `platformio.ini` automaticamente e baixará as dependências.

**3.** Configure as credenciais copiando o arquivo de exemplo:

```bash
cp include/config_example.h include/config.h
```

Edite `include/config.h` com suas chaves da Adafruit IO.

> ⚠️ **NUNCA comite o arquivo `config.h` com suas credenciais.** Ele já está listado no `.gitignore`.

**4.** Conecte o ESP32 via USB, compile e faça o upload clicando no ícone de **seta (→)** na barra inferior do PlatformIO.

---

## Padrões de Código

O projeto utiliza **C/C++ para sistemas embarcados**. Siga as diretrizes abaixo para manter estabilidade e legibilidade:

- **Evite o objeto `String`** — use `char arrays` ou reserve memória previamente para prevenir fragmentação de heap no ESP32.
- **Use constantes para pinos** — defina via `#define` ou `const uint8_t` para todos os mapeamentos de pinos lógicos (ex: pinos SPI do MAX6675: SO, CS, SCK).
- **Comente algoritmos de sinal** — inclua comentários explicativos em implementações de filtros como a Média Móvel.
- **Mantenha `loop()` limpa** — priorize chamadas não-bloqueantes. Evite `delay()`; prefira controle de tempo com `millis()`.

```cpp
// ✅ Correto — não-bloqueante
if (millis() - lastPublish >= PUBLISH_INTERVAL) {
    lastPublish = millis();
    publishSensorData();
}

// ❌ Evitar — bloqueante
delay(10000);
publishSensorData();
```

---

## Diretrizes Específicas

### Firmware (ESP32)

- **Valide os dados antes de publicar** no tópico MQTT — evite enviar ruídos, valores nulos ou falsos positivos gerados por desconexão do termopar.
- **O tratamento de quedas de Wi-Fi deve ser silencioso** — a perda de conexão não deve interromper a leitura local dos sensores.

### Dashboards (Adafruit IO)

- Ao adicionar um novo feed MQTT, **atualize a tabela de feeds no `README.md`**.
- Garanta que a taxa de Publish respeite o **limite da versão gratuita do Adafruit IO: 30 dados por minuto**.

---

## Testes

### Teste de Bancada (Bench Test)

- Toda modificação de firmware deve ser testada **primeiro na protoboard** utilizando o Serial Monitor do PlatformIO.
- **Simule quedas de conexão** desligando o roteador de testes para validar o comportamento do WiFiManager e a estabilidade do buffer local.

### Simulação de Sensores

Para testes **sem o hardware acoplado ao condensador real**:

| Sensor                 | Como simular                                                                                          |
| ---------------------- | ----------------------------------------------------------------------------------------------------- |
| **MPU6050**            | Variações manuais de inclinação da protoboard                                                         |
| **MAX6675 + Termopar** | Encoste uma fonte de calor na ponta do Termopar Tipo K para testar resposta e forçar alertas na nuvem |

---

_Diretrizes de contribuição para o Projeto Calisto — UFRN / IMD_
