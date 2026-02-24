#!/usr/bin/env bash
# Builds the web UI and copies it to plugin/WebViewAssets so the next
# plugin build embeds the production frontend. Run from repo root.
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
WEB_DIR="$REPO_ROOT/web"
ASSETS_DIR="$REPO_ROOT/plugin/WebViewAssets"

cd "$REPO_ROOT"
if [[ ! -d "$WEB_DIR" ]]; then
  echo "Missing web/ directory." >&2
  exit 1
fi

echo "Building web UI..."
cd "$WEB_DIR"
npm ci --omit=optional 2>/dev/null || npm install
npm run build

echo "Copying dist to WebViewAssets..."
mkdir -p "$ASSETS_DIR"
# Copy built files; keep existing js/juce (JUCE frontend) if present
rsync -a --delete --exclude='js/juce' "$WEB_DIR/dist/" "$ASSETS_DIR/" 2>/dev/null || {
  rm -rf "$ASSETS_DIR/index.html" "$ASSETS_DIR/assets" 2>/dev/null
  cp -R "$WEB_DIR/dist/"* "$ASSETS_DIR/"
}
echo "Done. Re-run CMake build to embed updated assets."
