#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "WebViewAssets.h"
#include <algorithm>
#include <cmath>
#include <juce_core/juce_core.h>
#include <juce_gui_extra/juce_gui_extra.h>

#ifndef ZIPPED_FILES_PREFIX
#error "ZIPPED_FILES_PREFIX must be set by CMake"
#endif

namespace excite {
namespace {

std::vector<std::byte> streamToVector(juce::InputStream& stream) {
  const auto sizeInBytes = static_cast<size_t>(stream.getTotalLength());
  std::vector<std::byte> result(sizeInBytes);
  stream.setPosition(0);
  [[maybe_unused]] auto read = stream.read(result.data(), result.size());
  jassert(read == static_cast<juce::int64>(sizeInBytes));
  return result;
}

const char* getMimeForExtension(const juce::String& extension) {
  static const std::unordered_map<juce::String, const char*> mimeMap = {
    {"htm", "text/html"},   {"html", "text/html"}, {"txt", "text/plain"},
    {"jpg", "image/jpeg"},   {"jpeg", "image/jpeg"}, {"svg", "image/svg+xml"},
    {"json", "application/json"}, {"png", "image/png"}, {"css", "text/css"},
    {"js", "text/javascript"}, {"woff2", "font/woff2"}, {"ico", "image/x-icon"}
  };
  auto it = mimeMap.find(extension.toLowerCase());
  return it != mimeMap.end() ? it->second : "application/octet-stream";
}

std::vector<std::byte> getWebViewFileAsBytes(const juce::String& filepath) {
  juce::MemoryInputStream zipStream(
    webview_assets::webview_assets_zip,
    webview_assets::webview_assets_zipSize,
    false);
  juce::ZipFile zipFile(zipStream);
  const juce::String pathInZip = juce::String(ZIPPED_FILES_PREFIX) + filepath;
  if (auto* entry = zipFile.getEntry(pathInZip)) {
    std::unique_ptr<juce::InputStream> is(zipFile.createStreamForEntry(*entry));
    if (is)
      return streamToVector(*is);
  }
  return {};
}

constexpr const char* LOCAL_DEV_SERVER = "http://127.0.0.1:5173";
constexpr int DEFAULT_EDITOR_WIDTH = 320;
constexpr int DEFAULT_EDITOR_HEIGHT = 720;
const juce::Identifier meterEventId{"meterData"};
const juce::Identifier spectrumEventId{"spectrumData"};

} // namespace

LifelineEditor::LifelineEditor(LifelineProcessor& p)
  : AudioProcessorEditor(&p),
    processorRef(p),
    webGainRelay(id::GAIN.getParamID()),
    webToneRelay(id::TONE.getParamID()),
    webMixRelay(id::MIX.getParamID()),
    webOutputGainRelay(id::OUTPUT_GAIN.getParamID()),
    webBypassRelay(id::BYPASS.getParamID()),
    webView([] {
      auto opts = juce::WebBrowserComponent::Options{}.withNativeIntegrationEnabled();
#if JUCE_WINDOWS
      opts = opts
        .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
        .withWinWebView2Options(
          juce::WebBrowserComponent::Options::WinWebView2{}
            .withUserDataFolder(juce::File::getSpecialLocation(
              juce::File::tempDirectory)));
#endif
      return opts;
    }()
      .withResourceProvider(
        [this](const juce::String& url) { return getResource(url); },
        juce::URL(LOCAL_DEV_SERVER).getOrigin())
      .withInitialisationData("pluginName", JUCE_PRODUCT_NAME)
      .withInitialisationData("pluginVersion", JUCE_PRODUCT_VERSION)
      .withOptionsFrom(webGainRelay)
      .withOptionsFrom(webToneRelay)
      .withOptionsFrom(webMixRelay)
      .withOptionsFrom(webOutputGainRelay)
      .withOptionsFrom(webBypassRelay)),
    webGainAttachment(
      *processorRef.getState().getParameter(id::GAIN.getParamID()),
      webGainRelay, nullptr),
    webToneAttachment(
      *processorRef.getState().getParameter(id::TONE.getParamID()),
      webToneRelay, nullptr),
    webMixAttachment(
      *processorRef.getState().getParameter(id::MIX.getParamID()),
      webMixRelay, nullptr),
    webOutputGainAttachment(
      *processorRef.getState().getParameter(id::OUTPUT_GAIN.getParamID()),
      webOutputGainRelay, nullptr),
    webBypassAttachment(
      *processorRef.getState().getParameter(id::BYPASS.getParamID()),
      webBypassRelay, nullptr) {
  addAndMakeVisible(webView);
#if defined(USE_LOCALHOST_UI) && USE_LOCALHOST_UI
  webView.goToURL(LOCAL_DEV_SERVER);
#else
  webView.goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
#endif

  setResizable(true, true);
  setResizeLimits(240, 540, 1200, 2700);
  if (auto* constrainer = getConstrainer())
    constrainer->setFixedAspectRatio(static_cast<double>(DEFAULT_EDITOR_WIDTH) /
                                     static_cast<double>(DEFAULT_EDITOR_HEIGHT));
  setSize(DEFAULT_EDITOR_WIDTH, DEFAULT_EDITOR_HEIGHT);
  if (resizableCorner != nullptr)
    resizableCorner->toFront(false);
  startTimerHz(30);
}

LifelineEditor::~LifelineEditor() { stopTimer(); }

void LifelineEditor::resized() {
  webView.setBounds(getLocalBounds());
  if (resizableCorner != nullptr) {
    auto cornerBounds = getLocalBounds().removeFromRight(18).removeFromBottom(18);
    resizableCorner->setBounds(cornerBounds);
    resizableCorner->toFront(false);
  }
}

void LifelineEditor::timerCallback() {
  std::array<float, 512> pulled{};
  for (;;) {
    const int got = processorRef.pullAnalyzerSamples(pulled.data(),
                                                     static_cast<int>(pulled.size()));
    if (got <= 0)
      break;

    for (int i = 0; i < got; ++i)
      pushNextSampleIntoFifo(pulled[static_cast<size_t>(i)]);
  }

  emitMeterEvent();

  if (nextFftBlockReady) {
    emitSpectrumEvent();
    nextFftBlockReady = false;
  }
}

void LifelineEditor::pushNextSampleIntoFifo(float sample) noexcept {
  if (fifoIndex == fftSize) {
    if (!nextFftBlockReady) {
      std::copy(fifo.begin(), fifo.end(), fftData.begin());
      std::fill(fftData.begin() + fftSize, fftData.end(), 0.0f);
      nextFftBlockReady = true;
    }
    fifoIndex = 0;
  }

  fifo[static_cast<size_t>(fifoIndex++)] = sample;
}

void LifelineEditor::emitMeterEvent() {
  auto* object = new juce::DynamicObject();
  object->setProperty("left", juce::jlimit(0.0f, 1.2f, processorRef.getMeterLeft()));
  object->setProperty("right", juce::jlimit(0.0f, 1.2f, processorRef.getMeterRight()));
  webView.emitEventIfBrowserIsVisible(meterEventId, juce::var(object));
}

void LifelineEditor::emitSpectrumEvent() {
  window.multiplyWithWindowingTable(fftData.data(), fftSize);
  forwardFFT.performFrequencyOnlyForwardTransform(fftData.data());

  juce::Array<juce::var> points;
  points.ensureStorageAllocated(spectrumBins);

  const auto nyquist = static_cast<float>(processorRef.getSampleRate() * 0.5);
  const auto safeNyquist = juce::jmax(1000.0f, nyquist);

  for (int i = 0; i < spectrumBins; ++i) {
    const float alpha0 = static_cast<float>(i) / static_cast<float>(spectrumBins);
    const float alpha1 = static_cast<float>(i + 1) / static_cast<float>(spectrumBins);
    const float freq0 = 20.0f * std::pow(safeNyquist / 20.0f, alpha0);
    const float freq1 = 20.0f * std::pow(safeNyquist / 20.0f, alpha1);
    const int bin0 = juce::jlimit(1, fftSize / 2 - 1,
                                  static_cast<int>(std::floor(freq0 / safeNyquist * (fftSize / 2))));
    const int bin1 = juce::jlimit(bin0, fftSize / 2 - 1,
                                  static_cast<int>(std::ceil(freq1 / safeNyquist * (fftSize / 2))));

    float maxMagnitude = 0.0f;
    for (int bin = bin0; bin <= bin1; ++bin)
      maxMagnitude = juce::jmax(maxMagnitude, fftData[static_cast<size_t>(bin)]);

    const float normalised = juce::jmap(
      juce::Decibels::gainToDecibels(maxMagnitude / static_cast<float>(fftSize), -100.0f),
      -90.0f, -12.0f, 0.0f, 1.0f);
    const float clipped = juce::jlimit(0.0f, 1.0f, normalised);
    auto& smoothed = spectrumSmoothed[static_cast<size_t>(i)];
    smoothed = 0.8f * smoothed + 0.2f * clipped;
    points.add(smoothed);
  }

  webView.emitEventIfBrowserIsVisible(spectrumEventId, juce::var(points));
}

std::optional<juce::WebBrowserComponent::Resource> LifelineEditor::getResource(
    const juce::String& url) const {
  juce::String path;
  if (url.startsWith("/"))
    path = url.substring(1);
  else {
    juce::URL u(url);
    path = u.getSubPath();
    if (path.startsWith("/"))
      path = path.substring(1);
  }
  if (path.isEmpty())
    path = "index.html";

  std::vector<std::byte> bytes = getWebViewFileAsBytes(path);
  if (bytes.empty())
    return std::nullopt;

  juce::String ext = path.fromLastOccurrenceOf(".", false, false);
  return juce::WebBrowserComponent::Resource{
    std::move(bytes), juce::String(getMimeForExtension(ext))};
}

} // namespace excite
