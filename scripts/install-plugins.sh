#!/usr/bin/env bash
# Copy built AU and VST3 into user Plug-Ins folders so Ableton (or other hosts) can find them.
# Run from repo root after: cmake --build --preset default
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${REPO_ROOT}/build/plugin/ExciteLifelineModule_artefacts/Debug"
AU_SRC="${BUILD_DIR}/AU/Lifeline Module.component"
VST3_SRC="${BUILD_DIR}/VST3/Lifeline Module.vst3"
COMPONENTS="${HOME}/Library/Audio/Plug-Ins/Components"
VST3="${HOME}/Library/Audio/Plug-Ins/VST3"

if [[ ! -d "$AU_SRC" ]]; then
  echo "AU not found. Run a full build first: cmake --build --preset default (use --clean-first if needed)." >&2
  exit 1
fi

mkdir -p "$COMPONENTS" "$VST3"
rm -rf "${COMPONENTS}/Lifeline Module.component" "${VST3}/Lifeline Module.vst3"
cp -R "$AU_SRC" "$COMPONENTS/"
cp -R "$VST3_SRC" "$VST3/"
echo "Installed AU and VST3. Rescan plugins in Ableton (Preferences → Plug-ins → Rescan) if they don’t appear."
