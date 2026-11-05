#include <RoboCore_SMW_SX1262M0.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <esp_arduino_version.h>

#include "secrets.h"

// Troque para 1 para habilitar um buzzer passivo no GPIO 27.
#define USAR_BUZZER 0

HardwareSerial LoRaSerial(2);
SMW_SX1262M0 LoRa(LoRaSerial);
CommandResponse response;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

const int PIN_RX = 25;  // recebe do shield (pino 10)
const int PIN_TX = 26;  // envia ao shield (pino 11)
const uint32_t FREQUENCIA = 915200;  // kHz, igual ao transmissor
const uint32_t TEMPO_ESCUTA_MS = 1000;
const float ALTURA_TANQUE_CM = 15.0;
const float NIVEL_BAIXO_CM = 3.0;
const float NIVEL_ALTO_CM = 12.0;

const uint32_t INTERVALO_WIFI_MS = 10000;
const uint32_t INTERVALO_MQTT_MS = 5000;
const uint16_t MQTT_BUFFER_BYTES = 256;

#if USAR_BUZZER
const uint8_t PIN_BUZZER = 27;
const uint16_t FREQUENCIA_BUZZER_HZ = 1200;
const uint8_t RESOLUCAO_BUZZER_BITS = 8;
#if ESP_ARDUINO_VERSION_MAJOR < 3
const uint8_t CANAL_BUZZER = 0;
#endif
#endif

float rssi, snr;
Buffer data;
uint32_t ultimaTentativaWiFi = 0;
uint32_t ultimaTentativaMqtt = 0;
bool wifiEstavaConectado = false;

String topico(const char *sufixo) {
  String resultado(MQTT_TOPIC_PREFIX);
  resultado += '/';
  resultado += sufixo;
  return resultado;
}

#if USAR_BUZZER
void configurarBuzzer() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(PIN_BUZZER, FREQUENCIA_BUZZER_HZ, RESOLUCAO_BUZZER_BITS);
  ledcWriteTone(PIN_BUZZER, 0);
#else
  ledcSetup(CANAL_BUZZER, FREQUENCIA_BUZZER_HZ, RESOLUCAO_BUZZER_BITS);
  ledcAttachPin(PIN_BUZZER, CANAL_BUZZER);
  ledcWriteTone(CANAL_BUZZER, 0);
#endif
}

void atualizarBuzzer(bool alerta) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWriteTone(PIN_BUZZER, alerta ? FREQUENCIA_BUZZER_HZ : 0);
#else
  ledcWriteTone(CANAL_BUZZER, alerta ? FREQUENCIA_BUZZER_HZ : 0);
#endif
}
#else
void configurarBuzzer() {}
void atualizarBuzzer(bool alerta) { (void)alerta; }
#endif

bool publicar(const char *sufixo, const char *valor, bool retido = true) {
  String nomeTopico = topico(sufixo);
  bool ok = mqttClient.publish(nomeTopico.c_str(), valor, retido);
  if (!ok) {
    Serial.print(F("Falha ao publicar MQTT em "));
    Serial.println(nomeTopico);
  }
  return ok;
}

void conectarMqttSeNecessario() {
  if (WiFi.status() != WL_CONNECTED || mqttClient.connected()) return;

  uint32_t agora = millis();
  if (agora - ultimaTentativaMqtt < INTERVALO_MQTT_MS) return;
  ultimaTentativaMqtt = agora;

  char clientId[32];
  uint64_t chipId = ESP.getEfuseMac();
  snprintf(clientId, sizeof(clientId), "hydrowatch-%04X%08X",
           (uint16_t)(chipId >> 32), (uint32_t)chipId);

  String topicoStatus = topico("status");
  Serial.print(F("Conectando ao MQTT... "));

  bool conectado;
  if (MQTT_USER[0] != '\0') {
    conectado = mqttClient.connect(clientId, MQTT_USER, MQTT_PASSWORD,
                                   topicoStatus.c_str(), 1, true, "offline");
  } else {
    conectado = mqttClient.connect(clientId, topicoStatus.c_str(), 1, true,
                                   "offline");
  }

  if (conectado) {
    Serial.println(F("conectado"));
    publicar("status", "online");
  } else {
    Serial.print(F("falhou, rc="));
    Serial.println(mqttClient.state());
  }
}

void manterConexoes() {
  uint32_t agora = millis();
  bool wifiConectado = WiFi.status() == WL_CONNECTED;

  if (!wifiConectado && wifiEstavaConectado) {
    Serial.println(F("Wi-Fi desconectado"));
    wifiEstavaConectado = false;
  }

  if (!wifiConectado && agora - ultimaTentativaWiFi >= INTERVALO_WIFI_MS) {
    ultimaTentativaWiFi = agora;
    Serial.println(F("Tentando reconectar ao Wi-Fi..."));
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }

  if (wifiConectado && !wifiEstavaConectado) {
    wifiEstavaConectado = true;
    ultimaTentativaMqtt = agora - INTERVALO_MQTT_MS;
    Serial.print(F("Wi-Fi conectado, IP: "));
    Serial.println(WiFi.localIP());
  }

  conectarMqttSeNecessario();
  if (mqttClient.connected()) mqttClient.loop();
}

bool somenteDigitos(const String &texto) {
  if (texto.length() == 0) return false;
  for (size_t i = 0; i < texto.length(); ++i) {
    if (!isDigit(texto[i])) return false;
  }
  return true;
}

bool decimalValido(const String &texto) {
  if (texto.length() == 0) return false;

  bool encontrouDigito = false;
  bool encontrouPonto = false;
  for (size_t i = 0; i < texto.length(); ++i) {
    char c = texto[i];
    if (isDigit(c)) {
      encontrouDigito = true;
    } else if (c == '.' && !encontrouPonto) {
      encontrouPonto = true;
    } else {
      return false;
    }
  }
  return encontrouDigito && texto[0] != '.' && texto[texto.length() - 1] != '.';
}

bool interpretarMedicao(const String &msg, float &nivel, int &pct) {
  if (!msg.startsWith("N=")) return false;

  int separador = msg.indexOf(";P=");
  if (separador <= 2 || msg.indexOf(";P=", separador + 3) >= 0) return false;

  String textoNivel = msg.substring(2, separador);
  String textoPct = msg.substring(separador + 3);
  if (!decimalValido(textoNivel) || !somenteDigitos(textoPct)) return false;

  nivel = textoNivel.toFloat();
  pct = textoPct.toInt();
  return nivel >= 0.0 && nivel <= ALTURA_TANQUE_CM && pct >= 0 && pct <= 100;
}

void publicarMedicao(float nivel, int pct, const char *alerta) {
  if (!mqttClient.connected()) {
    Serial.println(F("MQTT desconectado: medicao nao publicada"));
    return;
  }

  char textoNivel[12];
  char textoPct[5];
  char textoRssi[16];
  snprintf(textoNivel, sizeof(textoNivel), "%.1f", nivel);
  snprintf(textoPct, sizeof(textoPct), "%d", pct);
  snprintf(textoRssi, sizeof(textoRssi), "%.1f", rssi);

  publicar("nivel_cm", textoNivel);
  publicar("nivel_pct", textoPct);
  publicar("alerta", alerta);
  publicar("rssi", textoRssi);
}

void processarMensagem(const String &msg) {
  if (msg == "ERRO") {
    Serial.println(F("Transmissor sem eco do sensor"));
    atualizarBuzzer(false);
    if (mqttClient.connected()) publicar("status", "sensor_sem_eco");
    else Serial.println(F("MQTT desconectado: status de erro nao publicado"));
    return;
  }

  float nivel;
  int pct;
  if (!interpretarMedicao(msg, nivel, pct)) {
    Serial.print(F("Mensagem LoRa invalida, ignorada: "));
    Serial.println(msg);
    return;
  }

  const char *alerta = "ideal";
  if (nivel <= NIVEL_BAIXO_CM) alerta = "baixo";
  else if (nivel >= NIVEL_ALTO_CM) alerta = "alto";

  Serial.print(F("Nivel: "));
  Serial.print(nivel, 1);
  Serial.print(F(" cm ("));
  Serial.print(pct);
  Serial.print(F("%) - "));
  Serial.println(alerta);

  atualizarBuzzer(nivel <= NIVEL_BAIXO_CM || nivel >= NIVEL_ALTO_CM);
  publicarMedicao(nivel, pct, alerta);
}

void setup() {
  Serial.begin(115200);
  LoRaSerial.begin(9600, SERIAL_8N1, PIN_RX, PIN_TX);
  configurarBuzzer();

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  ultimaTentativaWiFi = millis();

  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setBufferSize(MQTT_BUFFER_BYTES);
  mqttClient.setKeepAlive(15);
  mqttClient.setSocketTimeout(1);

  LoRa.reset();
  LoRa.P2P_start(FREQUENCIA, true);  // escuta continua
  Serial.println(F("Receptor ESP32 pronto"));
}

void loop() {
  manterConexoes();

  response = LoRa.P2P_listen(TEMPO_ESCUTA_MS, data, rssi, snr);

  // Alimenta o keep-alive MQTT entre as janelas bloqueantes de escuta LoRa.
  manterConexoes();
  if (response != CommandResponse::DATA) return;

  String msg;
  while (data.available()) msg += (char)data.read();

  Serial.print(F("Recebido: "));
  Serial.print(msg);
  Serial.print(F(" (RSSI "));
  Serial.print(rssi);
  Serial.println(F(")"));

  processarMensagem(msg);
}
