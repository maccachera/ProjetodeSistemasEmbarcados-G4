#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
INSTALL_DIR="$PROJECT_DIR/.local-tools/mosquitto"
DOWNLOAD_DIR="$(mktemp -d)"
trap 'rm -rf "$DOWNLOAD_DIR"' EXIT

for command_name in dnf rpm2cpio cpio; do
  if ! command -v "$command_name" >/dev/null; then
    echo "Comando obrigatorio ausente: $command_name" >&2
    exit 1
  fi
done

mkdir -p "$INSTALL_DIR"
dnf download --arch x86_64 --destdir "$DOWNLOAD_DIR" mosquitto
RPM_FILE="$(find "$DOWNLOAD_DIR" -maxdepth 1 -name 'mosquitto-*.x86_64.rpm' -print -quit)"

if [[ -z "$RPM_FILE" ]]; then
  echo "Pacote RPM do Mosquitto nao foi encontrado" >&2
  exit 1
fi

(
  cd "$INSTALL_DIR"
  rpm2cpio "$RPM_FILE" | cpio -idm --quiet
)

echo "Mosquitto instalado localmente em $INSTALL_DIR"
echo "Inicie o ambiente com: ./scripts/start-local.sh"
