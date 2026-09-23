const int PIN_TRIG = 9;
const int PIN_ECHO = 10;
const int PIN_BUZZER = 11;

const float ALTURA_TANQUE_CM = 15.0;
const float NIVEL_BAIXO_CM = 3.0;
const float NIVEL_ALTO_CM = 12.0;

const unsigned long INTERVALO_LEITURA_MS = 1000;

unsigned long ultimaLeitura = 0;
float ultimaDistancia = NAN;
float ultimoNivel = NAN;

float medirDistanciaCm() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  unsigned long duracao = pulseIn(PIN_ECHO, HIGH, 30000);

  if (duracao == 0) {
    return NAN;
  }

  return duracao * 0.0343 / 2.0;
}

void atualizarBuzzer(float nivelCm) {
  bool nivelCritico =
      nivelCm <= NIVEL_BAIXO_CM ||
      nivelCm >= NIVEL_ALTO_CM;

  if (nivelCritico) {
    tone(PIN_BUZZER, 1200);
  } else {
    noTone(PIN_BUZZER);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  noTone(PIN_BUZZER);

  Serial.println("HydroWatch iniciado.");
}

void loop() {
  unsigned long agora = millis();

  if (agora - ultimaLeitura >= INTERVALO_LEITURA_MS) {
    ultimaLeitura = agora;
    ultimaDistancia = medirDistanciaCm();

    if (isnan(ultimaDistancia)) {
      Serial.println("Falha na leitura do sensor HC-SR04.");
      noTone(PIN_BUZZER);
    } else {
      ultimoNivel =
          constrain(ALTURA_TANQUE_CM - ultimaDistancia,
                    0.0,
                    ALTURA_TANQUE_CM);

      Serial.print("Distancia: ");
      Serial.print(ultimaDistancia, 1);
      Serial.print(" cm | Nivel: ");
      Serial.print(ultimoNivel, 1);
      Serial.println(" cm");

      atualizarBuzzer(ultimoNivel);
    }
  }
}
