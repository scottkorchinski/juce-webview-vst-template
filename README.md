# WebView Plugin Starter

Reusable JUCE 8 + React WebView starter for building audio plugins with a localhost dev flow, embedded production UI, parameter attachments, and a small native-to-web realtime event bridge.

## Prerequisites

- CMake 3.22+
- A C++20 toolchain with Ninja or Xcode/Visual Studio
- Node.js 18+
- On Windows: WebView2 installed and available to JUCE

## First Run

```bash
cmake --preset default
cmake --build --preset default
```

The target name, bundle ID, plugin codes, formats, editor size, and dev server URL all come from `template.config.json`.

## Bootstrap A New Plugin

Update the template defaults directly in `template.config.json`, or use the bootstrap script:

```bash
./scripts/new-plugin.sh \
  --name "My Plugin" \
  --company "Your Company" \
  --bundle-id "com.yourcompany.my-plugin" \
  --plugin-code "MyP1" \
  --manufacturer-code "Your"
```

Useful notes:

- `--name` sets the visible product name and derives a CMake-safe `projectName`
- `pluginCode` and `manufacturerCode` must both be exactly 4 characters
- after bootstrapping, rerun `cmake --preset default` so JUCE picks up the new target metadata

## Localhost Dev Workflow

Use this while building the React UI:

```bash
cd web
npm install
npm run dev
```

Then build the plugin against the dev server:

```bash
cmake --preset default -DUSE_LOCALHOST_UI=ON
cmake --build --preset default
./scripts/install-plugins.sh
```

When the plugin opens inside the DAW, the editor loads from the dev server and hot reload works while you edit `web/src`.

## Embedded Build Workflow

Use this when you want the UI packaged inside the plugin binary:

```bash
./scripts/prepare-webview-assets.sh
cmake --build --preset release
```

`prepare-webview-assets.sh` builds the Vite app and copies `web/dist/` into `plugin/WebViewAssets/` while preserving JUCE's browser bridge assets.

## Add A Parameter

1. Add the new parameter ID in `plugin/Source/ParameterIDs.h`.
2. Add the JUCE parameter in `TemplatePluginProcessor::createParameterLayout()` in `plugin/Source/PluginProcessor.cpp`.
3. Create the relay and attachment in `plugin/Source/PluginEditor.h` and `plugin/Source/PluginEditor.cpp`.
4. Bind the new control in `web/src/App.jsx` with `useJuceSlider()` or `useJuceToggle()`.

## Connect A Web Control To JUCE

For sliders:

1. Call `useJuceSlider('paramId', defaultValue)` in `web/src/App.jsx`.
2. Pass `setValue`, `reset`, `beginGesture`, and `endGesture` into your component.
3. Use the returned `value` to render the UI.

For toggles:

1. Call `useJuceToggle('paramId', false)`.
2. Use `checked`, `setChecked`, or `toggle` from the hook.

## Repo Layout

- `template.config.json`: starter metadata used by CMake and scripts
- `plugin/Source/`: processor, editor, parameter IDs, and template config helpers
- `plugin/WebViewAssets/`: embedded web assets bundled into BinaryData
- `web/`: Vite + React development app
- `scripts/new-plugin.sh`: non-interactive bootstrap script
- `scripts/install-plugins.sh`: copies built plugins into local AU/VST3 folders
- `scripts/prepare-webview-assets.sh`: builds and stages embedded assets

## macOS Universal Build Note

The root `CMakeLists.txt` forces `x86_64;arm64` on Apple so a single build can load in both Intel and Apple Silicon hosts.

## Common Failure Modes

- Localhost UI never loads: confirm `npm run dev` is running and the URL in `template.config.json` matches the `USE_LOCALHOST_UI` build.
- Blank WebView window: ensure the host supports JUCE's browser backend and that WebView2 is available on Windows.
- Embedded assets missing: rerun `./scripts/prepare-webview-assets.sh` before the plugin build so `plugin/WebViewAssets/` contains the latest Vite output.
- Plugin not showing in the DAW: rerun `./scripts/install-plugins.sh`, then rescan plugins in the host.
