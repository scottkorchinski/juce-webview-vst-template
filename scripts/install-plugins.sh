#!/usr/bin/env bash
# Copy built AU and VST3 artefacts into the user's plugin folders.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
CONFIG_PATH="${REPO_ROOT}/template.config.json"

if [[ ! -f "$CONFIG_PATH" ]]; then
  echo "Missing template.config.json at $CONFIG_PATH" >&2
  exit 1
fi

readarray -t CONFIG_VALUES < <(python3 - <<'PY' "$CONFIG_PATH"
import json
import sys

with open(sys.argv[1], "r", encoding="utf-8") as handle:
    config = json.load(handle)

print(config["projectName"])
print(config["productName"])
print("\n".join(config["formats"]))
PY
)

PROJECT_NAME="${CONFIG_VALUES[0]}"
PRODUCT_NAME="${CONFIG_VALUES[1]}"
FORMATS=$(printf '%s\n' "${CONFIG_VALUES[@]:2}")

resolve_build_dir() {
  local candidates=(
    "${REPO_ROOT}/build/plugin/${PROJECT_NAME}_artefacts/Debug"
    "${REPO_ROOT}/build/${PROJECT_NAME}_artefacts/Debug"
    "${REPO_ROOT}/release-build/plugin/${PROJECT_NAME}_artefacts/Release"
    "${REPO_ROOT}/release-build/${PROJECT_NAME}_artefacts/Release"
  )

  local candidate
  for candidate in "${candidates[@]}"; do
    if [[ -d "$candidate" ]]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  done

  return 1
}

BUILD_DIR="$(resolve_build_dir || true)"
if [[ -z "$BUILD_DIR" ]]; then
  echo "Could not find a built artefacts directory for ${PROJECT_NAME}." >&2
  echo "Run cmake --build --preset default or cmake --build --preset release first." >&2
  exit 1
fi

install_component() {
  local source_path="$1"
  local destination_dir="$2"
  local display_name="$3"

  if [[ ! -e "$source_path" ]]; then
    echo "Skipping ${display_name}; not found at ${source_path}" >&2
    return 0
  fi

  mkdir -p "$destination_dir"
  rm -rf "${destination_dir}/$(basename "$source_path")"
  cp -R "$source_path" "$destination_dir/"
  echo "Installed ${display_name}"
}

if printf '%s\n' "$FORMATS" | rg '^AU$' >/dev/null; then
  install_component \
    "${BUILD_DIR}/AU/${PRODUCT_NAME}.component" \
    "${HOME}/Library/Audio/Plug-Ins/Components" \
    "AU"
fi

if printf '%s\n' "$FORMATS" | rg '^VST3$' >/dev/null; then
  install_component \
    "${BUILD_DIR}/VST3/${PRODUCT_NAME}.vst3" \
    "${HOME}/Library/Audio/Plug-Ins/VST3" \
    "VST3"
fi

echo "Done. Rescan plugins in your DAW if the updated build does not appear immediately."
