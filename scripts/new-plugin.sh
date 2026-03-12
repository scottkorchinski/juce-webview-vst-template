#!/usr/bin/env bash
# Update the template config and starter-facing metadata for a new plugin.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
CONFIG_PATH="${REPO_ROOT}/template.config.json"

if [[ ! -f "$CONFIG_PATH" ]]; then
  echo "Missing template.config.json at $CONFIG_PATH" >&2
  exit 1
fi

usage() {
  cat <<'EOF'
Usage:
  ./scripts/new-plugin.sh [options]

Options:
  --name "My Plugin"                Set product name and derive project/package names
  --project-name "MyPlugin"         Explicit CMake target/project name
  --product-name "My Plugin"        Explicit displayed product name
  --company "My Company"
  --bundle-id "com.me.my-plugin"
  --plugin-code "MyP1"              Four-character plugin code
  --manufacturer-code "Mine"        Four-character manufacturer code
  --is-synth true|false
  --formats "AU,VST3,Standalone"
  --editor-width 360
  --editor-height 640
  --dev-server-url "http://127.0.0.1:5173"
  --git-init                        Initialize git if this copy is not already a repo
  --help
EOF
}

sanitize_project_name() {
  python3 - <<'PY' "$1"
import re
import sys

name = re.sub(r'[^A-Za-z0-9]+', ' ', sys.argv[1]).title().replace(' ', '')
print(name or "MyPlugin")
PY
}

sanitize_package_name() {
  python3 - <<'PY' "$1"
import re
import sys

name = re.sub(r'[^a-z0-9]+', '-', sys.argv[1].lower()).strip('-')
print(name or "webview-plugin-starter-ui")
PY
}

PRODUCT_NAME=""
PROJECT_NAME=""
COMPANY_NAME=""
BUNDLE_ID=""
PLUGIN_CODE=""
MANUFACTURER_CODE=""
IS_SYNTH=""
FORMATS=""
EDITOR_WIDTH=""
EDITOR_HEIGHT=""
DEV_SERVER_URL=""
INIT_GIT=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --name)
      PRODUCT_NAME="${2:-}"
      shift 2
      ;;
    --project-name)
      PROJECT_NAME="${2:-}"
      shift 2
      ;;
    --product-name)
      PRODUCT_NAME="${2:-}"
      shift 2
      ;;
    --company)
      COMPANY_NAME="${2:-}"
      shift 2
      ;;
    --bundle-id)
      BUNDLE_ID="${2:-}"
      shift 2
      ;;
    --plugin-code)
      PLUGIN_CODE="${2:-}"
      shift 2
      ;;
    --manufacturer-code)
      MANUFACTURER_CODE="${2:-}"
      shift 2
      ;;
    --is-synth)
      IS_SYNTH="${2:-}"
      shift 2
      ;;
    --formats)
      FORMATS="${2:-}"
      shift 2
      ;;
    --editor-width)
      EDITOR_WIDTH="${2:-}"
      shift 2
      ;;
    --editor-height)
      EDITOR_HEIGHT="${2:-}"
      shift 2
      ;;
    --dev-server-url)
      DEV_SERVER_URL="${2:-}"
      shift 2
      ;;
    --git-init)
      INIT_GIT=1
      shift
      ;;
    --help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

if [[ -n "$PRODUCT_NAME" && -z "$PROJECT_NAME" ]]; then
  PROJECT_NAME="$(sanitize_project_name "$PRODUCT_NAME")"
fi

python3 - <<'PY' \
  "$CONFIG_PATH" \
  "$PROJECT_NAME" \
  "$PRODUCT_NAME" \
  "$COMPANY_NAME" \
  "$BUNDLE_ID" \
  "$PLUGIN_CODE" \
  "$MANUFACTURER_CODE" \
  "$IS_SYNTH" \
  "$FORMATS" \
  "$EDITOR_WIDTH" \
  "$EDITOR_HEIGHT" \
  "$DEV_SERVER_URL"
import json
import sys

(
    config_path,
    project_name,
    product_name,
    company_name,
    bundle_id,
    plugin_code,
    manufacturer_code,
    is_synth,
    formats,
    editor_width,
    editor_height,
    dev_server_url,
) = sys.argv[1:]

with open(config_path, "r", encoding="utf-8") as handle:
    config = json.load(handle)

if project_name:
    config["projectName"] = project_name
if product_name:
    config["productName"] = product_name
if company_name:
    config["companyName"] = company_name
if bundle_id:
    config["bundleId"] = bundle_id
if plugin_code:
    if len(plugin_code) != 4:
        raise SystemExit("pluginCode must be exactly 4 characters")
    config["pluginCode"] = plugin_code
if manufacturer_code:
    if len(manufacturer_code) != 4:
        raise SystemExit("manufacturerCode must be exactly 4 characters")
    config["manufacturerCode"] = manufacturer_code
if is_synth:
    if is_synth.lower() not in {"true", "false"}:
        raise SystemExit("--is-synth must be true or false")
    config["isSynth"] = is_synth.lower() == "true"
if formats:
    config["formats"] = [item.strip() for item in formats.split(",") if item.strip()]
if editor_width:
    config["editorWidth"] = int(editor_width)
if editor_height:
    config["editorHeight"] = int(editor_height)
if dev_server_url:
    config["devServerUrl"] = dev_server_url

with open(config_path, "w", encoding="utf-8") as handle:
    json.dump(config, handle, indent=2)
    handle.write("\n")

print(config["projectName"])
print(config["productName"])
print(config["companyName"])
PY

readarray -t UPDATED_VALUES < <(python3 - <<'PY' "$CONFIG_PATH"
import json
import sys

with open(sys.argv[1], "r", encoding="utf-8") as handle:
    config = json.load(handle)

print(config["projectName"])
print(config["productName"])
print(config["companyName"])
PY
)

UPDATED_PROJECT_NAME="${UPDATED_VALUES[0]}"
UPDATED_PRODUCT_NAME="${UPDATED_VALUES[1]}"
UPDATED_COMPANY_NAME="${UPDATED_VALUES[2]}"
PACKAGE_NAME="$(sanitize_package_name "$UPDATED_PRODUCT_NAME")"

python3 - <<'PY' \
  "$REPO_ROOT/README.md" \
  "$REPO_ROOT/web/index.html" \
  "$REPO_ROOT/web/package.json" \
  "$UPDATED_PRODUCT_NAME" \
  "$UPDATED_COMPANY_NAME" \
  "$PACKAGE_NAME"
import json
import sys
from pathlib import Path

readme_path = Path(sys.argv[1])
index_path = Path(sys.argv[2])
package_json_path = Path(sys.argv[3])
product_name = sys.argv[4]
company_name = sys.argv[5]
package_name = sys.argv[6]

for path in (readme_path, index_path):
    content = path.read_text(encoding="utf-8")
    content = content.replace("WebView Plugin Starter", product_name)
    content = content.replace("Your Company", company_name)
    path.write_text(content, encoding="utf-8")

package_json = json.loads(package_json_path.read_text(encoding="utf-8"))
package_json["name"] = package_name
package_json_path.write_text(json.dumps(package_json, indent=2) + "\n", encoding="utf-8")
PY

if [[ "$INIT_GIT" -eq 1 ]]; then
  if [[ -d "$REPO_ROOT/.git" ]]; then
    echo "Git repository already exists; skipping git init."
  else
    git init "$REPO_ROOT"
    echo "Initialized a new git repository."
  fi
fi

echo "Updated template config for ${UPDATED_PRODUCT_NAME} (${UPDATED_PROJECT_NAME})."
echo "Next steps:"
echo "  1. Review template.config.json"
echo "  2. Run npm install in web/ if dependencies changed"
echo "  3. Reconfigure CMake: cmake --preset default"
