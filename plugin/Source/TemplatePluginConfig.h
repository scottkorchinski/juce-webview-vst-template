#pragma once

#include <juce_core/juce_core.h>

#ifndef TEMPLATE_EDITOR_WIDTH
#define TEMPLATE_EDITOR_WIDTH 360
#endif

#ifndef TEMPLATE_EDITOR_HEIGHT
#define TEMPLATE_EDITOR_HEIGHT 640
#endif

#ifndef TEMPLATE_DEV_SERVER_URL
#define TEMPLATE_DEV_SERVER_URL "http://127.0.0.1:5173"
#endif

namespace template_plugin::config {

inline constexpr int editorWidth = TEMPLATE_EDITOR_WIDTH;
inline constexpr int editorHeight = TEMPLATE_EDITOR_HEIGHT;
inline constexpr const char* devServerUrl = TEMPLATE_DEV_SERVER_URL;

inline const juce::Identifier meterEventId{"meterData"};
inline const juce::Identifier spectrumEventId{"spectrumData"};

} // namespace template_plugin::config
