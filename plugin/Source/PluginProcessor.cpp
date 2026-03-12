#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterIDs.h"
#include <algorithm>
#include <cmath>

namespace template_plugin {

TemplatePluginProcessor::TemplatePluginProcessor()
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
  parameters.drive =
    static_cast<juce::AudioParameterFloat*>(state.getParameter(id::DRIVE.getParamID()));
  parameters.tone =
    static_cast<juce::AudioParameterFloat*>(state.getParameter(id::TONE.getParamID()));
  parameters.mix =
    static_cast<juce::AudioParameterFloat*>(state.getParameter(id::MIX.getParamID()));
  parameters.bypass =
    static_cast<juce::AudioParameterBool*>(state.getParameter(id::BYPASS.getParamID()));
}

TemplatePluginProcessor::~TemplatePluginProcessor() {}

const juce::String TemplatePluginProcessor::getName() const { return JucePlugin_Name; }

bool TemplatePluginProcessor::acceptsMidi() const { return false; }
bool TemplatePluginProcessor::producesMidi() const { return false; }
bool TemplatePluginProcessor::isMidiEffect() const { return false; }
double TemplatePluginProcessor::getTailLengthSeconds() const { return 0.0; }

int TemplatePluginProcessor::getNumPrograms() { return 1; }
int TemplatePluginProcessor::getCurrentProgram() { return 0; }
void TemplatePluginProcessor::setCurrentProgram(int) {}
const juce::String TemplatePluginProcessor::getProgramName(int) { return {}; }
void TemplatePluginProcessor::changeProgramName(int, const juce::String&) {}

void TemplatePluginProcessor::prepareToPlay(double sampleRate, int) {
  currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
  ensureStateForChannels(getTotalNumOutputChannels());
}

void TemplatePluginProcessor::releaseResources() {}

bool TemplatePluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
      layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;
#if !JucePlugin_IsSynth
  if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
    return false;
#endif
  return true;
}

void TemplatePluginProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                           juce::MidiBuffer& midi) {
  juce::ignoreUnused(midi);
  juce::ScopedNoDenormals noDenormals;

  const int numOutputChannels = getTotalNumOutputChannels();
  const int numInputChannels = getTotalNumInputChannels();
  const int numSamples = buffer.getNumSamples();

  for (int channel = numInputChannels; channel < numOutputChannels; ++channel)
    buffer.clear(channel, 0, numSamples);

  const int mainChannels = juce::jmin(numInputChannels, numOutputChannels);
  if (mainChannels <= 0 || numSamples <= 0)
    return;

  if (parameters.bypass->get()) {
    meterLeft.store(0.0f);
    meterRight.store(0.0f);
    return;
  }

  ensureStateForChannels(mainChannels);

  juce::AudioBuffer<float> dryBuffer(mainChannels, numSamples);
  for (int channel = 0; channel < mainChannels; ++channel)
    dryBuffer.copyFrom(channel, 0, buffer, channel, 0, numSamples);

  const float drive = parameters.drive->get();
  const float tone = parameters.tone->get();
  const float mix = parameters.mix->get();

  const float inputGain = 1.0f + drive * 7.0f;
  const float outputTrim = juce::jmap(drive, 0.0f, 1.0f, 1.0f, 0.7f);
  const float toneCutoffHz = juce::jmap(tone, 0.0f, 1.0f, 180.0f, 5200.0f);
  const float alpha = std::exp(-2.0f * juce::MathConstants<float>::pi * toneCutoffHz /
                               static_cast<float>(juce::jmax(1.0, currentSampleRate)));

  for (int channel = 0; channel < mainChannels; ++channel) {
    auto* wetSamples = buffer.getWritePointer(channel);
    const auto* drySamples = dryBuffer.getReadPointer(channel);
    auto& lowpass = lowpassStates[static_cast<size_t>(channel)];

    for (int sample = 0; sample < numSamples; ++sample) {
      const float dry = drySamples[sample];
      const float saturated = std::tanh(dry * inputGain) * outputTrim;
      lowpass = ((1.0f - alpha) * saturated) + (alpha * lowpass);
      const float highpass = saturated - lowpass;
      const float toneBlend = juce::jmap(tone, lowpass, highpass);
      wetSamples[sample] = dry + (mix * (toneBlend - dry));
    }
  }

  float peakL = 0.0f;
  float peakR = 0.0f;
  const auto* left = buffer.getReadPointer(0);
  const auto* right = buffer.getReadPointer(mainChannels > 1 ? 1 : 0);
  for (int sample = 0; sample < numSamples; ++sample) {
    peakL = juce::jmax(peakL, std::abs(left[sample]));
    peakR = juce::jmax(peakR, std::abs(right[sample]));
  }

  meterLeft.store(juce::jmax(peakL, meterLeft.load() * 0.92f));
  meterRight.store(juce::jmax(peakR, meterRight.load() * 0.92f));

  const int samplesToWrite = juce::jmin(numSamples, analyzerFifo.getFreeSpace());
  if (samplesToWrite <= 0)
    return;

  const auto writeScope = analyzerFifo.write(samplesToWrite);
  for (int sample = 0; sample < writeScope.blockSize1; ++sample) {
    analyzerStorage[static_cast<size_t>(writeScope.startIndex1 + sample)] =
      0.5f * (left[sample] + right[sample]);
  }

  for (int sample = 0; sample < writeScope.blockSize2; ++sample) {
    analyzerStorage[static_cast<size_t>(writeScope.startIndex2 + sample)] =
      0.5f * (left[writeScope.blockSize1 + sample] + right[writeScope.blockSize1 + sample]);
  }
}

bool TemplatePluginProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* TemplatePluginProcessor::createEditor() {
  return new TemplatePluginEditor(*this);
}

void TemplatePluginProcessor::getStateInformation(juce::MemoryBlock& destData) {
  auto tree = state.copyState().createXml();
  copyXmlToBinary(*tree, destData);
}

void TemplatePluginProcessor::setStateInformation(const void* data, int sizeInBytes) {
  auto tree = getXmlFromBinary(data, sizeInBytes);
  if (tree.get() != nullptr && tree->hasTagName(state.state.getType()))
    state.replaceState(juce::ValueTree::fromXml(*tree));
}

juce::AudioProcessorValueTreeState::ParameterLayout
TemplatePluginProcessor::createParameterLayout(Parameters&) {
  juce::AudioProcessorValueTreeState::ParameterLayout layout;
  layout.add(std::make_unique<juce::AudioParameterFloat>(
    id::DRIVE.getParamID(), "Drive",
    juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 0.5f), 0.35f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
    id::TONE.getParamID(), "Tone",
    juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 0.5f), 0.5f));
  layout.add(std::make_unique<juce::AudioParameterFloat>(
    id::MIX.getParamID(), "Mix",
    juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 0.5f), 1.0f));
  layout.add(std::make_unique<juce::AudioParameterBool>(
    id::BYPASS.getParamID(), "Bypass", false));
  return layout;
}

int TemplatePluginProcessor::pullAnalyzerSamples(float* destination, int maxSamples) {
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

void TemplatePluginProcessor::ensureStateForChannels(int numChannels) {
  lowpassStates.resize(static_cast<size_t>(juce::jmax(0, numChannels)), 0.0f);
}

} // namespace template_plugin

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new template_plugin::TemplatePluginProcessor();
}
