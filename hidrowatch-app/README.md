# HydroWatch — painel mobile

Frontend responsivo em HTML, CSS e JavaScript puro. O resumo da tela inicial recebe dados reais do broker MQTT por WebSockets e apresenta nível, porcentagem, alerta, RSSI e estado do receptor. O histórico permanece demonstrativo até que seja adicionado armazenamento das medições.

## Executar com o ambiente local

Na raiz do repositório:

```bash
./scripts/install-local-mosquitto.sh
./scripts/start-local.sh
```

O script mostra o endereço do painel, normalmente semelhante a:

```text
http://192.168.1.50:8080
```

Abra esse endereço em um navegador conectado à mesma rede. O painel assina automaticamente `hydrowatch/#` no broker WebSocket da mesma máquina.

Para encerrar:

```bash
./scripts/stop-local.sh
```

## Arquivos principais

- `dist/index.html`: estrutura das telas e carregamento do MQTT.js;
- `dist/styles.css`: identidade visual e estados online/offline;
- `dist/mqtt-config.js`: endereço WebSocket e prefixo dos tópicos;
- `dist/app.js`: navegação, dados MQTT em tempo real e histórico demonstrativo.

## Limitações

- A integração local usa `ws://`, sem TLS, e deve permanecer em uma rede controlada.
- O GitHub Pages exige `wss://`; portanto, a publicação pública precisa de broker WebSocket com TLS e domínio válido.
- O broker retém somente a leitura mais recente. Um backend ou banco de dados será necessário para produzir histórico real.
