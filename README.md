# HydroWatch

O HydroWatch monitora o nível de líquido de um tanque com 15 cm de altura. Um Arduino Uno mede a distância com um HC-SR04 e envia o nível por rádio LoRa P2P; um ESP32 recebe os dados, classifica o nível e os publica em um broker MQTT pela rede Wi-Fi.

Não há LoRaWAN nem gateway neste projeto. Os dois módulos SMW-SX1262M0 se comunicam diretamente em **915200 kHz**.

O repositório também mantém em `hidrowatch-app/` um protótipo estático da interface mobile. Ele ainda usa dados simulados e não faz parte do caminho MQTT implementado nos sketches.

## Arquitetura

```text
HC-SR04 → Arduino Uno + shield SMW-SX1262M0
         → rádio LoRa P2P (915200 kHz)
         → shield SMW-SX1262M0 + ESP32 WROVER
         → Wi-Fi → broker MQTT → consumidor/aplicação
```

O transmissor mede e envia uma mensagem a cada 5 segundos. O receptor mantém a escuta LoRa em janelas de até 1 segundo e executa o `loop()` do cliente MQTT entre essas janelas. As novas tentativas de Wi-Fi e MQTT são controladas com `millis()`, sem esperas longas com `delay()`.

## Hardware e ligações

### Transmissor — Arduino Uno

| Origem | Destino no Uno | Observação |
| --- | --- | --- |
| HC-SR04 TRIG | pino 6 | saída de disparo |
| HC-SR04 ECHO | pino 7 | entrada do eco |
| HC-SR04 VCC | 5V | alimentação |
| HC-SR04 GND | GND | terra comum |
| Shield LoRa pino 10 | pino 10 | RX de `SoftwareSerial` |
| Shield LoRa pino 11 | pino 11 | TX de `SoftwareSerial` |
| Shield LoRa | pino 13 e A0 | reservados para LED e botão do shield |

Os pinos 10, 11, 13 e A0 não devem ser compartilhados com outros componentes.

### Receptor — ESP32 WROVER Dev (38 pinos)

| ESP32 | Shield LoRa | Função |
| --- | --- | --- |
| 3.3V | IOREF | referência lógica |
| GPIO 25 (RX) | pino 10 | dados do shield para o ESP32 |
| GPIO 26 (TX) | pino 11 | dados do ESP32 para o shield |
| GND | GND | terra comum |
| VCC/5V | 5V | alimentação do shield |
| GPIO 27 | buzzer passivo, opcional | sinal a 1200 Hz |

Não use os GPIOs 6 a 11, ligados à flash, nem os GPIOs 16 e 17, reservados à PSRAM no WROVER. O buzzer vem desabilitado no código; para habilitá-lo, altere `#define USAR_BUZZER 0` para `1` em `receptor_esp32/receptor_esp32.ino`. A implementação seleciona automaticamente a API LEDC do core ESP32 2.x ou 3.x.

## Protocolo LoRa

O formato do payload é fixo:

```text
N=12.4;P=83
```

- `N`: nível em centímetros, com base na altura útil de 15 cm;
- `P`: porcentagem cheia, calculada por `nível / 15 × 100` e arredondada;
- `ERRO`: o HC-SR04 não retornou eco dentro de 30 ms.

O receptor aceita apenas a estrutura esperada e valores nos intervalos de 0 a 15 cm e de 0 a 100%. Payloads malformados são registrados no Monitor Serial e ignorados, sem publicação MQTT.

### Alertas

| Nível | Valor de `alerta` |
| --- | --- |
| `N ≤ 3,0 cm` | `baixo` |
| `3,0 cm < N < 12,0 cm` | `ideal` |
| `N ≥ 12,0 cm` | `alto` |

## MQTT

O prefixo padrão é `hydrowatch` e pode ser alterado em `secrets.h`.

| Tópico | Conteúdo | Quando é publicado |
| --- | --- | --- |
| `hydrowatch/nivel_cm` | nível, por exemplo `12.4` | medição LoRa válida |
| `hydrowatch/nivel_pct` | percentual, por exemplo `83` | medição LoRa válida |
| `hydrowatch/alerta` | `baixo`, `alto` ou `ideal` | medição LoRa válida |
| `hydrowatch/rssi` | RSSI do pacote, por exemplo `-78.0` | medição LoRa válida |
| `hydrowatch/status` | `online` | conexão ao broker estabelecida |
| `hydrowatch/status` | `sensor_sem_eco` | payload LoRa igual a `ERRO` |
| `hydrowatch/status` | `offline` | última vontade (LWT) do broker |

As publicações são retidas para que um novo assinante receba o estado mais recente. A LWT usa QoS 1 e também é retida. As medições recebidas durante uma indisponibilidade do MQTT são mostradas no Serial, mas não são armazenadas para reenvio; uma nova medição será publicada depois da reconexão.

## Dependências

Instale pelo Gerenciador de Bibliotecas da Arduino IDE:

- **RoboCore - SMW_SX1262M0** (a biblioteca instalada pode aparecer na pasta `arduino_997961`);
- **PubSubClient**, de Nick O'Leary.

Também são usadas bibliotecas fornecidas pelos respectivos cores:

- `SoftwareSerial`, no Arduino AVR/Uno;
- `WiFi` e `HardwareSerial`, no core **esp32 by Espressif Systems**.

Nenhuma biblioteca LoRaWAN é necessária, pois a comunicação usa P2P.

## Configuração das credenciais

O arquivo real de credenciais é ignorado pelo Git e não deve ser commitado.

1. Copie `receptor_esp32/secrets.example.h` para `receptor_esp32/secrets.h`.
2. Preencha os valores da rede Wi-Fi e do broker.
3. Deixe `MQTT_USER` e `MQTT_PASSWORD` vazios (`""`) se o broker aceitar conexão anônima.
4. Mantenha `MQTT_TOPIC_PREFIX` como `"hydrowatch"` ou escolha outro prefixo sem a barra final.

Exemplo da estrutura esperada:

```cpp
const char WIFI_SSID[] = "minha_rede";
const char WIFI_PASSWORD[] = "minha_senha";
const char MQTT_HOST[] = "broker.exemplo.com";
const uint16_t MQTT_PORT = 1883;
const char MQTT_USER[] = "usuario";
const char MQTT_PASSWORD[] = "senha";
const char MQTT_TOPIC_PREFIX[] = "hydrowatch";
```

O exemplo usa MQTT TCP na porta 1883. O sketch atual não configura TLS.

## Compilação e gravação

### Transmissor

1. Abra `transmissor_uno/transmissor_uno.ino` na Arduino IDE.
2. Selecione **Arduino Uno**, a porta correta e compile/envie.
3. Abra o Monitor Serial em **9600 baud**.

### Receptor

1. Crie `receptor_esp32/secrets.h` conforme a seção anterior.
2. Abra `receptor_esp32/receptor_esp32.ino`.
3. Use estas opções em **Ferramentas**:

| Opção | Valor |
| --- | --- |
| Board | ESP32 Wrover Module |
| Flash Mode | DIO |
| Flash Frequency | 40 MHz |
| PSRAM | Disabled |
| Partition Scheme | Default 4MB with spiffs |

4. Selecione a porta, compile/envie e abra o Monitor Serial em **115200 baud**.

O receptor usa `HardwareSerial(2)` nos GPIOs 25 e 26, conforme a montagem definida. Caso a versão instalada de **RoboCore - SMW_SX1262M0** rejeite `HardwareSerial`, preserve a biblioteca e as ligações e registre a mensagem exata do compilador para diagnóstico.

## Estrutura do repositório

```text
.
├── transmissor_uno/
│   └── transmissor_uno.ino
├── receptor_esp32/
│   ├── receptor_esp32.ino
│   └── secrets.example.h
├── legado/
│   └── HydroWatch.ino
├── hidrowatch-app/
│   └── dist/
└── README.md
```

`legado/HydroWatch.ino` preserva a implementação anterior, de uma única placa, apenas como referência histórica.

## Limitações e testes de hardware

- O cálculo supõe um tanque de geometria simples e informa nível, não volume.
- Ondulações, espuma e o posicionamento do sensor podem afetar as leituras.
- Não há fila persistente para mensagens recebidas enquanto o broker está indisponível.
- A alimentação, o alcance LoRa, as credenciais Wi-Fi/MQTT, a LWT e o buzzer precisam ser validados no hardware real.
