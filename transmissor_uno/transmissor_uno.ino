#include <RoboCore_SMW_SX1262M0.h>
#include <SoftwareSerial.h>

SoftwareSerial LoRaSerial(10, 11);
SMW_SX1262M0 LoRa(LoRaSerial);
CommandResponse response;

const uint32_t FREQUENCIA = 915200;  // kHz, igual nos dois
const uint8_t PIN_TRIG = 6;
const uint8_t PIN_ECHO = 7;
const float ALTURA_TANQUE_CM = 15.0;
const unsigned long INTERVALO_MS = 5000;
unsigned long ultimoEnvio = 0;

// Retorna a distancia em cm, ou -1 se nao houve eco.
float medirDistanciaCm() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  unsigned long t = pulseIn(PIN_ECHO, HIGH, 30000);
  if (t == 0) return -1;
  return t * 0.0343 / 2.0;
}

void setup() {
  Serial.begin(9600);
  LoRaSerial.begin(9600);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  LoRa.reset();
  Serial.println(F("Transmissor pronto"));
}

void loop() {
  if (millis() - ultimoEnvio < INTERVALO_MS) return;
  ultimoEnvio = millis();

  float dist = medirDistanciaCm();
  String msg;
  if (dist < 0) {
    msg = "ERRO";
  } else {
    float nivel = constrain(ALTURA_TANQUE_CM - dist, 0.0, ALTURA_TANQUE_CM);
    int pct = (int)round(nivel / ALTURA_TANQUE_CM * 100.0);
    msg = "N=" + String(nivel, 1) + ";P=" + String(pct);
    Serial.print(F("Nivel: "));
    Serial.print(nivel, 1);
    Serial.print(F(" cm | "));
    Serial.print(pct);
    Serial.println(F("% cheio"));
  }

  response = LoRa.P2P_start(FREQUENCIA, false, msg.c_str());
  Serial.print(F("Enviando: "));
  Serial.print(msg);
  Serial.println(response == CommandResponse::OK ? F(" (ok)") : F(" (falhou)"));
}
