# HydroWatch

O **HydroWatch** é um protótipo de sistema embarcado para monitoramento do nível de líquido em um tanque. O projeto utiliza um sensor ultrassônico HC-SR04 para medir a distância entre o sensor e a superfície do líquido e, a partir dessa medida, calcula o nível preenchido do tanque. Um buzzer é acionado quando o nível fica abaixo ou acima dos limites configurados.

## Aplicativo mobile

O projeto também inclui um protótipo estático e responsivo do aplicativo de monitoramento. A interface foi construída somente com HTML, CSS e JavaScript, sem frameworks ou etapa de compilação.

**Acesse o protótipo:** [maccachera.github.io/ProjetodeSistemasEmbarcados-G4](https://maccachera.github.io/ProjetodeSistemasEmbarcados-G4/)

Atualmente, as medições exibidas são simuladas. Em uma etapa futura, a camada de dados poderá ser conectada ao Supabase/PostgreSQL.

## Objetivo

O projeto tem como objetivo demonstrar uma solução simples de monitoramento local capaz de:

- medir periodicamente a distância até a superfície do líquido;
- converter a distância medida em nível do tanque;
- identificar níveis baixo e alto considerados críticos;
- emitir um alerta sonoro em situações críticas;
- exibir medições e falhas pelo Monitor Serial.

## Componentes

- 1 placa compatível com o framework Arduino;
- 1 sensor ultrassônico HC-SR04;
- 1 buzzer piezoelétrico;
- protoboard e jumpers;
- cabo USB para alimentação e gravação do programa.

## Ligações

| Componente | Terminal | Pino configurado |
| --- | --- | --- |
| HC-SR04 | TRIG | 9 |
| HC-SR04 | ECHO | 10 |
| Buzzer | Sinal | 11 |

Os terminais de alimentação devem ser ligados de acordo com as especificações da placa e dos componentes utilizados. Em placas que trabalham com lógica de 3,3 V, o sinal `ECHO` do HC-SR04 deve ser reduzido para uma tensão segura, por exemplo com um divisor resistivo.

## Funcionamento

A cada segundo, o microcontrolador envia um pulso de 10 microssegundos ao pino `TRIG`. O HC-SR04 responde mantendo o pino `ECHO` em nível alto durante o tempo de ida e volta da onda ultrassônica.

A distância é calculada por:

```text
distância (cm) = duração do eco × 0,0343 / 2
```

O valor é dividido por dois porque a onda percorre o caminho até a superfície e retorna ao sensor. Considerando um tanque com 15 cm de altura, o nível é obtido por:

```text
nível (cm) = altura do tanque − distância medida
```

O resultado é limitado ao intervalo de 0 a 15 cm para impedir valores fisicamente incompatíveis com o tanque configurado.

### Regras de alerta

| Condição | Comportamento |
| --- | --- |
| Nível menor ou igual a 3 cm | Buzzer ligado a 1200 Hz |
| Nível entre 3 cm e 12 cm | Buzzer desligado |
| Nível maior ou igual a 12 cm | Buzzer ligado a 1200 Hz |
| Falha ou tempo esgotado na leitura | Mensagem de erro e buzzer desligado |

## Customizações realizadas

O software padrão da plataforma Arduino fornece funções genéricas para controlar pinos, medir pulsos, temporizar tarefas e gerar tons. Para transformar esses recursos em uma aplicação específica de monitoramento de tanque, foram realizados os seguintes esforços de desenvolvimento:

1. **Mapeamento do hardware:** foram definidos pinos exclusivos para o gatilho e o eco do HC-SR04 e para o buzzer, centralizando a configuração no início do programa.

2. **Rotina própria de leitura ultrassônica:** foi criada a função `medirDistanciaCm()`, responsável por gerar a sequência de disparo exigida pelo HC-SR04, medir a duração do eco e convertê-la de microssegundos para centímetros.

3. **Proteção contra bloqueio e falha do sensor:** a chamada de `pulseIn()` recebeu um tempo limite de 30.000 microssegundos. Quando nenhum eco é detectado nesse período, a função retorna `NAN`, permitindo que o restante do sistema reconheça uma leitura inválida.

4. **Conversão de distância em nível:** a leitura bruta do sensor foi adaptada ao contexto do tanque. O software subtrai a distância medida da altura útil de 15 cm e usa `constrain()` para manter o resultado entre 0 e 15 cm.

5. **Parâmetros específicos do protótipo:** foram configurados a altura do tanque em 15 cm, o limite baixo em 3 cm, o limite alto em 12 cm e o intervalo entre leituras em 1 segundo. Esses valores estão definidos como constantes para facilitar calibrações futuras.

6. **Lógica de alarme para dois extremos:** foi desenvolvida a função `atualizarBuzzer()`, que considera críticos tanto o nível muito baixo quanto o nível muito alto. Nessas condições, o buzzer produz um tom de 1200 Hz.

7. **Execução periódica sem `delay()` no laço principal:** o controle do intervalo de leitura utiliza `millis()`. Essa adaptação evita interromper o microcontrolador durante um segundo inteiro e permite adicionar outras tarefas futuramente.

8. **Diagnóstico pelo Monitor Serial:** o programa informa a distância medida e o nível calculado com uma casa decimal, além de apresentar uma mensagem específica quando ocorre falha na leitura do HC-SR04.

9. **Comportamento seguro em caso de erro:** quando a medição é inválida, o buzzer é desligado para evitar que uma falha de comunicação com o sensor seja interpretada como um nível crítico real.

Essas customizações constituem a lógica própria do HydroWatch sobre as funções básicas fornecidas pelo ambiente Arduino.

## Parâmetros configuráveis

Os principais parâmetros podem ser alterados no início do arquivo [`HydroWatch/HydroWatch.ino`](HydroWatch/HydroWatch.ino):

```cpp
const float ALTURA_TANQUE_CM = 15.0;
const float NIVEL_BAIXO_CM = 3.0;
const float NIVEL_ALTO_CM = 12.0;
const unsigned long INTERVALO_LEITURA_MS = 1000;
```

Os valores devem ser ajustados conforme a altura útil do tanque, a posição do sensor e os limites desejados para o alerta.

## Como executar

1. Clone este repositório:

   ```bash
   git clone https://github.com/maccachera/ProjetodeSistemasEmbarcados-G4.git
   ```

2. Abra o arquivo `HydroWatch/HydroWatch.ino` na Arduino IDE.
3. Selecione a placa e a porta serial correspondentes.
4. Monte o circuito conforme a tabela de ligações.
5. Compile e envie o programa para a placa.
6. Abra o Monitor Serial na velocidade de **115200 baud**.

Uma leitura válida será exibida no formato:

```text
Distancia: 7.4 cm | Nivel: 7.6 cm
```

Se o sensor não retornar um eco dentro do tempo limite, será exibido:

```text
Falha na leitura do sensor HC-SR04.
```

## Estrutura do repositório

```text
.
├── .github/workflows/
│   └── pages.yml
├── HydroWatch/
│   └── HydroWatch.ino
├── hidrowatch-app/
│   ├── dist/
│   │   ├── index.html
│   │   ├── styles.css
│   │   └── app.js
│   └── README.md
└── README.md
```

## Limitações atuais

- os limites e a altura do tanque são definidos diretamente no código;
- o projeto considera um tanque de geometria simples e informa o nível em centímetros, não o volume;
- as leituras não são armazenadas em memória ou enviadas para um serviço externo;
- o buzzer utiliza o mesmo padrão sonoro para nível baixo e nível alto;
- ondulações, espuma, inclinação da superfície e posicionamento inadequado do sensor podem afetar a medição.

## Melhorias futuras

- aplicar filtragem ou média de várias leituras para reduzir oscilações;
- usar padrões sonoros diferentes para nível baixo, nível alto e falha do sensor;
- adicionar display local e indicadores luminosos;
- permitir calibração sem recompilar o programa;
- converter o nível em volume conforme a geometria do reservatório;
- registrar um histórico ou transmitir os dados por Wi-Fi/Bluetooth.
