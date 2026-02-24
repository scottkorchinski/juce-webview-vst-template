#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterIDs.h"
#include <algorithm>
#include <cmath>

namespace excite {

LifelineProcessor::LifelineProcessor()
  : AudioProcessor(
      BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
  ),
  state(*this, nullptr, "Parameters", createParameterLayout(parameters)) {
  parameters.gain = static_cast<juce::AudioParameterFloat*>(state.getParameter(id::GAIN.getParamID()));
  parameters.tone = static_cast<juce::AudioParameterFloat*>(state.getParameter(id::TONE.getParamID()));
  parameters.mix = static_cast<juce::AudioParameterFloat*>(state.getParameter(id::MIX.getParamID()));
  parameters.outputGain = static_cast<juce::AudioParameterFloat*>(state.getParameter(id::OUTPUT_GAIN.getParamID()));
  parameters.bypass = static_cast<juce::AudioParameterBool*>(state.getParameter(id::BYPASS.getParamID()));
}

LifelineProcessor::~LifelineProcessor() {}

const juce::String LifelineProcessor::getName() const {
  return JucePlugin_Name;
}

bool LifelineProcessor::acceptsMidi() const { return false; }
bool LifelineProcessor::producesMidi() const { return false; }
bool LifelineProcessor::isMidiEffect() const { return false; }
double LifelineProcessor::getTailLengthSeconds() const { return 0.0; }

int LifelineProcessor::getNumPrograms() { return 1; }
int LifelineProcessor::getCurrentProgram() { return 0; }
void LifelineProcessor::setCurrentProgram(int) {}
const juce::String LifelineProcessor::getProgramName(int) { return {}; }
void LifelineProcessor::changeProgramName(int, const juce::String&) {}

void LifelineProcessor::prepareToPlay(double sampleRate, int) {
  currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
  ensureStateForChannels(getTotalNumOutputChannels());
}
void LifelineProcessor::releaseResources() {}

bool LifelineProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
      layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;
#if !JucePlugin_IsSynth
  if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
    return false;
#endif
  return true;
}

void LifelineProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
  juce::ignoreUnused(midi);
  juce::ScopedNoDenormals noDenormals;
  const int numChannels = getTotalNumOutputChannels();
  const int numSamples = buffer.getNumSamples();

  for (int i = getTotalNumInputChannels(); i < numChannels; ++i)
    buffer.clear(i, 0, numSamples);

  if (parameters.bypass->get() || numSamples == 0)
    return;

  const int mainChannels = juce::jmin(getTotalNumInputChannels(), numChannels);
  if (mainChannels <= 0)
    return;

  ensureStateForChannels(mainChannels);

  juce::AudioBuffer<float> dryBuffer(mainChannels, numSamples);
  for (int channel = 0; channel < mainChannels; ++channel)
    dryBuffer.copyFrom(channel, 0, buffer, channel, 0, numSamples);

  const float crush = parameters.gain->get(); // UI "Crush"
  const float tone = parameters.tone->get();
  const float mix = parameters.mix->get();
  const float outputGainNormalised = parameters.outputGain->get();
  const float outputGainDb = (outputGainNormalised - 0.5f) * 24.0f;
  const float outputGain = juce::Decibels::decibelsToGain(outputGainDb);

  const float drive = 1.0f + crush * 9.0f;
  const int bitDepth = juce::jlimit(10, 16, 16 - static_cast<int>(std::round(crush * 6.0f)));
  const int downsampleFactor = juce::jlimit(1, 14, 1 + static_cast<int>(std::round(crush * 13.0f)));
  const float levels = static_cast<float>((1 << bitDepth) - 1);

  const float toneAmount = (tone - 0.5f) * 2.0f; // -1 => boost lows, +1 => cut lows
  const float lowCutoffHz = 220.0f;
  const float lowPassAlpha = std::exp(-2.0f * juce::MathConstants<float>::pi * lowCutoffHz /
                                      static_cast<float>(juce::jmax(1.0, currentSampleRate)));

  for (int channel = 0; channel < mainChannels; ++channel) {
    auto* samples = buffer.getWritePointer(channel);
    const auto* dry = dryBuffer.getReadPointer(channel);

    float held = heldSamples[static_cast<size_t>(channel)];
    int holdCounter = holdCounters[static_cast<size_t>(channel)];
    float lowState = toneLowStates[static_cast<size_t>(channel)];

    for (int sample = 0; sample < numSamples; ++sample) {
      const float in = dry[sample];

      if (holdCounter <= 0) {
        const float saturated = std::tanh(in * drive);
        const float normalized = juce::jlimit(0.0f, 1.0f, (saturated * 0.5f) + 0.5f);
        const float quantized = std::round(normalized * levels) / levels;
        held = (quantized * 2.0f) - 1.0f;
        holdCounter = downsampleFactor;
      }
      --holdCounter;

      lowState = (1.0f - lowPassAlpha) * held + lowPassAlpha * lowState;
      const float high = held - lowState;

      float wet = held;
      if (toneAmount < 0.0f) {
        // Left side boosts lows aggressively.
        const float lowBoost = 1.0f + (-toneAmount) * 3.0f;
        wet = high + (lowState * lowBoost);
      } else if (toneAmount > 0.0f) {
        // Right side progressively removes low content, ending at mostly highs.
        const float remainingLow = 1.0f - toneAmount;
        wet = high + (lowState * remainingLow);
      }

      samples[sample] = (in + mix * (wet - in)) * outputGain;
    }

    heldSamples[static_cast<size_t>(channel)] = held;
    holdCounters[static_cast<size_t>(channel)] = holdCounter;
    toneLowStates[static_cast<size_t>(channel)] = lowState;
  }

  float peakL = 0.0f;
  float peakR = 0.0f;
  if (mainChannels > 0) {
    const auto* left = buffer.getReadPointer(0);
    const auto* right = buffer.getReadPointer(mainChannels > 1 ? 1 : 0);
    for (int i = 0; i < numSamples; ++i) {
      peakL = juce::jmax(peakL, std::abs(left[i]));
      peakR = juce::jmax(peakR, std::abs(right[i]));
    }

    const float smoothL = juce::jmax(peakL, meterLeft.load() * 0.92f);
    const float smoothR = juce::jmax(peakR, meterRight.load() * 0.92f);
    meterLeft.store(smoothL);
    meterRight.store(smoothR);
  }

  const int samplesToWrite = juce::jmin(numSamples, analyzerFifo.getFreeSpace());
  if (samplesToWrite > 0) {
    const auto writeScope = analyzerFifo.write(samplesToWrite);
    const auto* left = buffer.getReadPointer(0);
    const auto* right = buffer.getReadPointer(mainChannels > 1 ? 1 : 0);

    for (int i = 0; i < writeScope.blockSize1; ++i) {
      analyzerStorage[static_cast<size_t>(writeScope.startIndex1 + i)] =
        0.5f * (left[i] + right[i]);
    }

    for (int i = 0; i < writeScope.blockSize2; ++i) {
      analyzerStorage[static_cast<size_t>(writeScope.startIndex2 + i)] =
        0.5f * (left[writeScope.blockSize1 + i] + right[writeScope.blockSize1 + i]);
    }
  }
}

bool LifelineProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* LifelineProcessor::createEditor() {
  return new LifelineEditor(*this);
}

void LifelineProcessor::getStateInformation(juce::MemoryBlock& destData) {
  auto tree = state.copyState().createXml();
  copyXmlToBinary(*tree, destData);
}

void LifelineProcessor::setStateInformation(const void* data, int sizeInBytes) {
  auto tree = getXmlFromBinary(data, sizeInBytes);
  if (tree.get() != nullptr && tree->hasTagName(state.state.getType()))
    state.replaceState(juce::ValueTree::fromXml(*tree));
}

juce::AudioProcessorValueTreeState::ParameterLayout
LifelineProcessor::createParameterLayout(Parameters&) {
  juce::AudioProcessorValueTreeState::ParameterLayout layout;
  layout.add(std::make_unique<juce::AudioParameterFloat>(
    id::GAIN.getParamID(), "Crush",
    juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 0.5f), 0.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
    id::TONE.getParamID(), "Tone",
    juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 0.5f), 0.5f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
    id::MIX.getParamID(), "Mix",
    juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 0.5f), 1.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
    id::OUTPUT_GAIN.getParamID(), "Output Gain",
    juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 1.0f), 0.5f));
  layout.add(std::make_unique<juce::AudioParameterBool>(
    id::BYPASS.getParamID(), "Bypass", false));
  return layout;
}

int LifelineProcessor::pullAnalyzerSamples(float* destination, int maxSamples) {
  if (destination == nullptr || maxSamples <= 0)
    return 0;

  const auto readScope = analyzerFifo.read(maxSamples);
  if (readScope.blockSize1 > 0) {
    std::copy_n(analyzerStorage.begin() + readScope.startIndex1,
                readScope.blockSize1,
                destination);
  }

  if (readScope.blockSize2 > 0) {
    std::copy_n(analyzerStorage.begin() + readScope.startIndex2,
                readScope.blockSize2,
                destination + readScope.blockSize1);
  }

  return readScope.blockSize1 + readScope.blockSize2;
}

void LifelineProcessor::ensureStateForChannels(int numChannels) {
  const auto count = static_cast<size_t>(juce::jmax(0, numChannels));
  heldSamples.resize(count, 0.0f);
  holdCounters.resize(count, 0);
  toneLowStates.resize(count, 0.0f);
}

} // namespace excite

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new excite::LifelineProcessor();
}
