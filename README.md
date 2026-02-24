# Excite Lifeline Module

JUCE audio plugin with a **webview-based UI** for rapid prototyping. Part of the [Lifeline](https://www.pluginboutique.com/meta_product/2-Effects/53-Multi-Effect-/10031-Excite-Audio-Lifeline-Expanse) series by Excite Audio.

## Requirements

- CMake 3.22+
- C++20 toolchain (Xcode, Visual Studio, Ninja + GCC/Clang)
- Node.js 18+ (for web UI dev and optional release embed)
- On Windows: WebView2 (JUCE will prompt if missing)

## Build (plugin only)

```bash
cmake --preset default
cmake --build --preset default
```

Plugin artefacts (AU, VST3, Standalone) appear under `build/ExciteLifelineModule_artefacts/`.

## Debug UI (hot reload) — use this if the plugin window is blank

If the plugin opens to a blank or grey window, load the UI from the dev server instead:

1. Start the web dev server (in a terminal, from the repo root):
   ```bash
   cd web && npm install && npm run dev
   ```
   Leave it running (you should see “Local: http://127.0.0.1:5173”).

2. Build the plugin with localhost UI enabled, then install:
   ```bash
   cmake --preset default -DUSE_LOCALHOST_UI=ON
   cmake --build --preset default
   ./scripts/install-plugins.sh
   ```
3. Open the plugin in your DAW. The editor will load the UI from the dev server and you get hot reload when you edit `web/src`.

## Release UI (embedded)

The plugin ships with a minimal embedded UI from `plugin/WebViewAssets/` (HTML/CSS/JS). To embed the full Vite/React build instead:

```bash
./scripts/prepare-webview-assets.sh
cmake --build --preset release
```

This builds `web/`, copies `web/dist/` into `plugin/WebViewAssets/` (preserving `js/juce/`), and the next plugin build embeds the updated assets.

## Layout

- `CMakeLists.txt` – root; fetches JUCE via CPM.
- `plugin/` – JUCE plugin (processor + editor with `WebBrowserComponent`).
- `plugin/Source/` – C++ (processor, editor, parameter IDs).
- `plugin/WebViewAssets/` – static UI for embedding (and JUCE frontend `js/juce/`).
- `web/` – Vite + React app for development (localhost).
- `scripts/prepare-webview-assets.sh` – copies web build into WebViewAssets for release.

## Parameters

- **Gain**, **Tone**, **Mix** (0–100%)
- **Bypass**

All are wired to the web UI via JUCE’s WebView parameter attachments (sliders and toggle).
