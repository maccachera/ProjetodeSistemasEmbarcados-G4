#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MOSQUITTO_BIN="$PROJECT_DIR/.local-tools/mosquitto/usr/bin/mosquitto"
MOSQUITTO_LIB="$PROJECT_DIR/.local-tools/mosquitto/usr/lib64"
WEB_DIR="$PROJECT_DIR/hidrowatch-app/dist"

if [[ ! -x "$MOSQUITTO_BIN" ]]; then
  echo "Mosquitto local nao encontrado em $MOSQUITTO_BIN" >&2
  echo "Execute a instalacao local descrita no README." >&2
  exit 1
fi

if ! systemctl --user is-active --quiet hydrowatch-mqtt.service; then
  systemd-run --user --unit=hydrowatch-mqtt --collect \
    --property=Restart=on-failure \
    --setenv="LD_LIBRARY_PATH=$MOSQUITTO_LIB" \
    "$MOSQUITTO_BIN" -c "$PROJECT_DIR/mosquitto-local.conf" >/dev/null
fi

if ! systemctl --user is-active --quiet hydrowatch-web.service; then
  systemd-run --user --unit=hydrowatch-web --collect \
    --property=Restart=on-failure \
    python3 -m http.server 8080 --bind 0.0.0.0 \
    --directory "$WEB_DIR" >/dev/null
fi

sleep 1

if ! systemctl --user is-active --quiet hydrowatch-mqtt.service; then
  echo "Falha ao iniciar Mosquitto." >&2
  echo "Consulte: journalctl --user -u hydrowatch-mqtt" >&2
  exit 1
fi

if ! systemctl --user is-active --quiet hydrowatch-web.service; then
  echo "Falha ao iniciar o site." >&2
  echo "Consulte: journalctl --user -u hydrowatch-web" >&2
  exit 1
fi

LOCAL_IP="$(hostname -I | awk '{print $1}')"
echo "Broker MQTT:       mqtt://$LOCAL_IP:1883"
echo "Broker WebSocket:  ws://$LOCAL_IP:9001"
echo "Painel HydroWatch: http://$LOCAL_IP:8080"
echo "Logs MQTT:         journalctl --user -u hydrowatch-mqtt"
echo "Logs web:          journalctl --user -u hydrowatch-web"
