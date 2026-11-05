#!/usr/bin/env bash
set -euo pipefail

for service in hydrowatch-web hydrowatch-mqtt; do
  if systemctl --user is-active --quiet "$service.service"; then
    systemctl --user stop "$service.service"
    echo "$service encerrado"
  fi
done
