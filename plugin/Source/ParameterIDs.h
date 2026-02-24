#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace excite::id {
const juce::ParameterID GAIN{"gain", 1};
const juce::ParameterID TONE{"tone", 1};
const juce::ParameterID MIX{"mix", 1};
const juce::ParameterID OUTPUT_GAIN{"outputGain", 1};
const juce::ParameterID BYPASS{"bypass", 1};
}
