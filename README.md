# 🍷 Checkpoint 05 – Vinheria Agnello (IoT com ESP32 e MQTT)

**Disciplina:** Edge Computing & Computer Systems – Engenharia de Software – FIAP  
**Professor:** Lucas Demetrius Augusto

---

Monitoramento da adega da Vinheria Agnello com um **ESP32** simulado no **Wokwi**, que lê temperatura, umidade e luminosidade e envia os dados a um **servidor de IoT via MQTT**. A leitura e a escrita de informações são feitas pelo aplicativo **MyMQTT**.

---

## 🔗 Links

| Recurso | Link |
|---|---|
| 🛠️ Projeto no Wokwi | [Acessar projeto](https://wokwi.com/projects/478680184133149697) |
| 🎥 Vídeo de demonstração https://youtu.be/9yrHAm18_mQ?si=voIenrk79HVglB-l
| 📦 Repositório no GitHub | [Acessar repositório](https://github.com/Braga1x/CP5-Edge-Computing) |

---

## 👥 Integrantes do grupo

| Nome | RM |
|---|---|
| Gustavo Braga | 569211 |
| Henry Gabriel | 570063 |
| Nicholas Belo | 571063 |

**Turma:** *1ESPJ*

---

## 📌 Contexto do projeto

A Vinheria Agnello precisa controlar as condições de armazenamento de seus vinhos. Segundo o material da disciplina, três fatores influenciam diretamente a qualidade do vinho:

| Fator | Condição ideal |
|---|---|
| 🌡️ **Temperatura** | Cerca de 13 °C, evitando flutuações maiores que 3 °C |
| 💧 **Umidade** | Próxima de 70% (faixa de 60% a 80%) |
| 💡 **Luminosidade** | Penumbra, pois a luz (principalmente UV) altera os compostos do vinho |

Este checkpoint implementa a camada de **captura e transmissão** desses dados: o ESP32 lê os sensores e publica as medições em um servidor MQTT, de onde podem ser consumidas por outras aplicações.

---

## 🎯 Objetivos do checkpoint

- Ler temperatura e umidade com o **DHT22**.
- Ler luminosidade com o **LDR**.
- Enviar os dados para o servidor de IoT via **MQTT**.
- Ler e escrever informações pelo aplicativo **MyMQTT**.
- Incluir o **timestamp** em cada leitura.

---

## 🧰 Componentes utilizados

| Componente | Função |
|---|---|
| ESP32 DevKit | Microcontrolador com Wi-Fi |
| Sensor DHT22 | Temperatura e umidade |
| Módulo LDR (fotoresistor) | Luminosidade |
| LED vermelho + resistor de 220 Ω | Atuador para demonstrar a **escrita** de comandos |

### 🔌 Ligações (pino a pino)

| Componente | Pino do componente | Pino do ESP32 |
|---|---|---|
| DHT22 | VCC | 3V3 |
| DHT22 | GND | GND |
| DHT22 | SDA (dados) | **D15** |
| LDR | VCC | 3V3 |
| LDR | GND | GND |
| LDR | AO (saída analógica) | **D34** |
| LED | Anodo (via resistor de 220 Ω) | **D2** |
| LED | Catodo | GND |

> 💡 O LDR usa o **D34**, que pertence ao conversor ADC1. Os pinos do ADC2 não funcionam para leitura analógica com o Wi-Fi ligado.

---

## 📁 Estrutura do repositório

| Arquivo | Descrição |
|---|---|
| `sketch.ino` | Código do ESP32 (sensores, Wi-Fi, MQTT, NTP) |
| `diagram.json` | Descrição do circuito no Wokwi |
| `libraries.txt` | Bibliotecas usadas no projeto |
| `README.md` | Este documento |

---

## ⚙️ Configuração do servidor MQTT

| Parâmetro | Valor |
|---|---|
| Servidor (host) | `3.135.234.101` |
| Porta | `1883` |
| Usuário / senha | Não utilizados |
| Wi-Fi (Wokwi) | Rede `Wokwi-GUEST`, sem senha |

### 📡 Tópicos

O identificador do dispositivo é `device001` (constante `DEVICE_ID` no código).

| Tópico | Sentido | Descrição |
|---|---|---|
| `/TEF/device001/attrs` | ESP32 → servidor | Publica as leituras dos sensores a cada 5 segundos |
| `/TEF/device001/cmd` | Servidor → ESP32 | Recebe comandos `on` e `off` para o LED |
| `/TEF/device001/state` | ESP32 → servidor | Confirma o estado do LED após cada comando |

### 📨 Formato da mensagem publicada

```json
{
  "temperature": 24.0,
  "humidity": 40.0,
  "luminosity": 2048,
  "timestamp": "2026-10-23T18:30:00Z"
}
```

| Campo | Unidade | Descrição |
|---|---|---|
| `temperature` | °C | Temperatura lida pelo DHT22 |
| `humidity` | % | Umidade relativa lida pelo DHT22 |
| `luminosity` | 0 a 4095 | Valor bruto do ADC (12 bits) lido do LDR |
| `timestamp` | ISO 8601 (UTC) | Data e hora da leitura, obtidas via NTP (`pool.ntp.org`) |

> ℹ️ Nos primeiros segundos após ligar, o timestamp pode aparecer como `sem-hora`, até o NTP sincronizar o relógio.

---

## 🧠 Como o código funciona

1. **Inicialização:** configura os pinos, o DHT22 e a resolução de 12 bits do ADC, e monta os tópicos a partir do `DEVICE_ID`.
2. **Wi-Fi:** conecta à rede `Wokwi-GUEST` e, se a conexão cair, reconecta automaticamente.
3. **Hora:** sincroniza o relógio via NTP para gerar o timestamp de cada leitura.
4. **MQTT:** conecta ao servidor com um Client ID aleatório (evita conflito entre conexões), assina o tópico `/cmd` e reconecta se cair.
5. **Publicação:** a cada 5 segundos lê os sensores, monta o JSON e publica em `/attrs`. O controle de tempo usa `millis()`, sem travar o programa.
6. **Recebimento de comandos:** quando chega `on` ou `off` em `/cmd`, o ESP32 liga ou desliga o LED e publica o estado em `/state`.

---

## ▶️ Como executar (replicar o projeto)

1. Abra o projeto no Wokwi pelo link da seção **Links**. Se preferir montar do zero, crie um projeto **ESP32** e copie para ele os arquivos `sketch.ino`, `diagram.json` e `libraries.txt` deste repositório.
2. Confirme que as bibliotecas **DHT sensor library** e **PubSubClient** estão instaladas (pelo `libraries.txt` ou pelo *Library Manager* do Wokwi).
3. Confira no início do `sketch.ino` o servidor (`MQTT_HOST`), a porta (`MQTT_PORT`) e o `DEVICE_ID`.
4. Clique em **Start the simulation** (botão verde).
5. Abra o monitor serial e verifique, nesta ordem:
   - `Wi-Fi conectado. IP: ...`
   - `Conectando ao broker MQTT... conectado.`
   - `Publicado em /TEF/device001/attrs: {...}` a cada 5 segundos.
6. Clique no DHT22 e no LDR dentro do Wokwi e altere os valores para ver as leituras mudarem.

---

## 📱 Testando com o MyMQTT

### 1. Conectar ao servidor

Em **Settings**, configure:

| Campo | Valor |
|---|---|
| Broker address | `3.135.234.101` |
| Port | `1883` |
| Client ID | Qualquer nome diferente do usado no código |
| Usuário / senha | Em branco |

### 2. Leitura (receber os dados do ESP32)

1. Na aba **Subscribe**, assine o tópico `/TEF/device001/attrs`.
2. Em até 5 segundos, o JSON com as leituras chega ao aplicativo.
3. Altere os sensores no Wokwi e observe os novos valores chegando.

### 3. Escrita (enviar comandos ao ESP32)

1. Assine também `/TEF/device001/state`, para ver a confirmação.
2. Na aba **Publish**, publique no tópico `/TEF/device001/cmd` a mensagem `on`: o LED acende e o `state` responde `on`.
3. Publique `off`: o LED apaga e o `state` responde `off`.

---

## 🛠️ Solução de problemas

| Sintoma | Possível causa e solução |
|---|---|
| `fatal error: PubSubClient.h: No such file` | Biblioteca não instalada. Adicione pelo *Library Manager* ou pelo `libraries.txt` |
| `Conectando ao Wi-Fi....` sem parar | Problema na rede simulada do Wokwi. Reinicie a simulação |
| `falhou, rc=-2` | Servidor inacessível. Confira IP e porta |
| `falhou, rc=4` ou `rc=5` | O servidor exige usuário e senha |
| MyMQTT conecta, mas não recebe dados | O tópico precisa ser idêntico ao do serial, incluindo a barra inicial `/` |
| Timestamp `sem-hora` | Aguarde alguns segundos pela sincronização do NTP |
| LED não reage ao comando | Confira se a mensagem é exatamente `on` ou `off` e se o tópico é o `/cmd` |

---

## ✅ Requisitos do checkpoint atendidos

- Leitura de temperatura e umidade com o DHT22.
- Leitura de luminosidade com o LDR.
- Envio dos dados ao servidor de IoT via MQTT.
- Timestamp em cada leitura.
- Leitura e escrita de informações pelo MyMQTT.
- Comunicação via Wi-Fi.
- README com instruções de replicação e arquivos disponibilizados.

---

## 🎥 Demonstração

O vídeo mostra o circuito no Wokwi, o monitor serial com as conexões, o MyMQTT recebendo os dados em tempo real e o envio de comandos `on` e `off` com o LED reagindo.

👉 https://youtu.be/9yrHAm18_mQ?si=voIenrk79HVglB-l
