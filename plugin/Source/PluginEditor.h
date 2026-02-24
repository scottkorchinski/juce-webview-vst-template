#pragma once

#include "PluginProcessor.h"
#include "ParameterIDs.h"
#include <array>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

namespace excite {

class LifelineEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
  explicit LifelineEditor(LifelineProcessor&);
  ~LifelineEditor() override;

  void resized() override;

private:
  static constexpr int fftOrder = 11;
  static constexpr int fftSize = 1 << fftOrder;
  static constexpr int spectrumBins = 72;

  void timerCallback() override;
  using Resource = juce::WebBrowserComponent::Resource;
  std::optional<Resource> getResource(const juce::String& url) const;
  void pushNextSampleIntoFifo(float sample) noexcept;
  void emitSpectrumEvent();
  void emitMeterEvent();

  LifelineProcessor& processorRef;

  juce::WebSliderRelay webGainRelay;
  juce::WebSliderRelay webToneRelay;
  juce::WebSliderRelay webMixRelay;
  juce::WebSliderRelay webOutputGainRelay;
  juce::WebToggleButtonRelay webBypassRelay;

  juce::WebBrowserComponent webView;

  juce::WebSliderParameterAttachment webGainAttachment;
  juce::WebSliderParameterAttachment webToneAttachment;
  juce::WebSliderParameterAttachment webMixAttachment;
  juce::WebSliderParameterAttachment webOutputGainAttachment;
  juce::WebToggleButtonParameterAttachment webBypassAttachment;

  juce::dsp::FFT forwardFFT{fftOrder};
  juce::dsp::WindowingFunction<float> window{fftSize, juce::dsp::WindowingFunction<float>::hann};
  std::array<float, fftSize> fifo{};
  std::array<float, 2 * fftSize> fftData{};
  std::array<float, spectrumBins> spectrumSmoothed{};
  int fifoIndex{0};
  bool nextFftBlockReady{false};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LifelineEditor)
};

} // namespace excite
