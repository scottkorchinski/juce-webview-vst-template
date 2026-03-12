#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>

namespace template_plugin {

class TemplatePluginProcessor : public juce::AudioProcessor {
public:
  TemplatePluginProcessor();
  ~TemplatePluginProcessor() override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;
  bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override;
  const juce::String getName() const override;

  bool acceptsMidi() const override;
  bool producesMidi() const override;
  bool isMidiEffect() const override;
  double getTailLengthSeconds() const override;

  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram(int index) override;
  const juce::String getProgramName(int index) override;
  void changeProgramName(int index, const juce::String& newName) override;

  void getStateInformation(juce::MemoryBlock& destData) override;
  void setStateInformation(const void* data, int sizeInBytes) override;

  juce::AudioProcessorValueTreeState& getState() noexcept { return state; }
  const juce::AudioProcessorValueTreeState& getState() const noexcept { return state; }
  int pullAnalyzerSamples(float* destination, int maxSamples);
  float getMeterLeft() const noexcept { return meterLeft.load(); }
  float getMeterRight() const noexcept { return meterRight.load(); }

private:
  struct Parameters {
    juce::AudioParameterFloat* drive{nullptr};
    juce::AudioParameterFloat* tone{nullptr};
    juce::AudioParameterFloat* mix{nullptr};
    juce::AudioParameterBool* bypass{nullptr};
  };
  static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout(Parameters&);

  Parameters parameters;
  juce::AudioProcessorValueTreeState state;

  void ensureStateForChannels(int numChannels);

  double currentSampleRate{44100.0};
  std::vector<float> lowpassStates;

  juce::AbstractFifo analyzerFifo{16384};
  std::vector<float> analyzerStorage{std::vector<float>(16384, 0.0f)};
  std::atomic<float> meterLeft{0.0f};
  std::atomic<float> meterRight{0.0f};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TemplatePluginProcessor)
};

} // namespace template_plugin
