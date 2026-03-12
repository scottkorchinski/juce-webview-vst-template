import { useEffect, useMemo, useRef, useState } from 'react';
import { MeterPair } from './components/MeterPair';
import { ParameterKnob } from './components/ParameterKnob';
import { SpectrumPanel } from './components/SpectrumPanel';
import { useJuceSlider } from './hooks/useJuceSlider';
import { useJuceToggle } from './hooks/useJuceToggle';
import './App.css';

const clamp = (value, min, max) => Math.min(max, Math.max(min, value));
const BASE_WIDTH = 360;
const BASE_HEIGHT = 640;

const percentLabel = (value) => `${Math.round(value * 100)}%`;

export default function App() {
  const rootRef = useRef(null);
  const [uiScale, setUiScale] = useState(1);
  const [meters, setMeters] = useState({ left: 0, right: 0 });
  const [spectrumBins, setSpectrumBins] = useState(() => new Array(72).fill(0));

  const drive = useJuceSlider('drive', 0.35);
  const tone = useJuceSlider('tone', 0.5);
  const mix = useJuceSlider('mix', 1);
  const bypass = useJuceToggle('bypass', false);

  const hasJuce =
    typeof Juce !== 'undefined' &&
    typeof window.__JUCE__ !== 'undefined' &&
    window.__JUCE__.initialisationData?.__juce__sliders?.length > 0;

  const pluginName = window.__JUCE__?.initialisationData?.pluginName ?? 'WebView Plugin Starter';
  const pluginVersion = window.__JUCE__?.initialisationData?.pluginVersion ?? '0.1.0';
  const companyName = window.__JUCE__?.initialisationData?.companyName ?? 'Your Company';

  useEffect(() => {
    const node = rootRef.current;
    if (!node) return undefined;

    const updateScale = (rect) => {
      const scale = Math.min(rect.width / BASE_WIDTH, rect.height / BASE_HEIGHT);
      setUiScale(Math.max(0.6, scale));
    };

    const observer = new ResizeObserver((entries) => {
      if (entries[0]) updateScale(entries[0].contentRect);
    });

    observer.observe(node);
    updateScale(node.getBoundingClientRect());
    return () => observer.disconnect();
  }, []);

  useEffect(() => {
    if (typeof window === 'undefined' || !window.__JUCE__?.backend) return undefined;

    const meterToken = window.__JUCE__.backend.addEventListener('meterData', (event) => {
      setMeters({
        left: clamp(Number(event?.left ?? 0), 0, 1.2),
        right: clamp(Number(event?.right ?? 0), 0, 1.2),
      });
    });

    const spectrumToken = window.__JUCE__.backend.addEventListener('spectrumData', (event) => {
      if (!Array.isArray(event)) return;
      setSpectrumBins(event.map((value) => clamp(Number(value ?? 0), 0, 1)));
    });

    return () => {
      window.__JUCE__.backend.removeEventListener(meterToken);
      window.__JUCE__.backend.removeEventListener(spectrumToken);
    };
  }, []);

  useEffect(() => {
    document.title = pluginName;
  }, [pluginName]);

  const stageStyle = useMemo(
    () => ({
      width: `${BASE_WIDTH}px`,
      height: `${BASE_HEIGHT}px`,
      transform: `scale(${uiScale})`,
    }),
    [uiScale]
  );

  const resetParameters = () => {
    drive.reset();
    tone.reset();
    mix.reset();
    bypass.setChecked(false);
  };

  return (
    <div className="app-shell" ref={rootRef}>
      <div className={`plugin-stage ${bypass.checked ? 'is-bypassed' : ''}`} style={stageStyle}>
        <header className="hero-card">
          <div>
            <span className="eyebrow">JUCE 8 + WebView template</span>
            <h1>{pluginName}</h1>
            <p>
              Reuse this shell, swap the parameters in `ParameterIDs.h`, and replace the UI when your
              product needs something custom.
            </p>
          </div>
          <div className="hero-meta">
            <div className="hero-chip">{companyName}</div>
            <div className="hero-chip">v{pluginVersion}</div>
          </div>
        </header>

        <section className="status-row">
          <button type="button" className={`bypass-toggle ${bypass.checked ? 'is-on' : ''}`} onClick={bypass.toggle}>
            <span className="bypass-toggle-label">Bypass</span>
            <span className="bypass-toggle-knob" />
          </button>
          <div className="status-copy">
            <span className="status-title">Realtime bridge example</span>
            <span className="status-text">Meters and analyzer data are emitted from native code.</span>
          </div>
          <MeterPair left={meters.left} right={meters.right} />
        </section>

        <section className="controls-card">
          <div className="section-heading">
            <span>Starter parameters</span>
            <button type="button" className="reset-button" onClick={resetParameters}>
              Reset
            </button>
          </div>
          <div className="controls-grid">
            <ParameterKnob
              label="Drive"
              value={drive.value}
              valueText={percentLabel(drive.value)}
              accent="#67e8f9"
              disabled={bypass.checked}
              onChange={drive.setValue}
              onReset={drive.reset}
              onGestureStart={drive.beginGesture}
              onGestureEnd={drive.endGesture}
            />
            <ParameterKnob
              label="Tone"
              value={tone.value}
              valueText={percentLabel(tone.value)}
              accent="#a78bfa"
              disabled={bypass.checked}
              onChange={tone.setValue}
              onReset={tone.reset}
              onGestureStart={tone.beginGesture}
              onGestureEnd={tone.endGesture}
            />
            <ParameterKnob
              label="Mix"
              value={mix.value}
              valueText={percentLabel(mix.value)}
              accent="#f59e0b"
              disabled={bypass.checked}
              onChange={mix.setValue}
              onReset={mix.reset}
              onGestureStart={mix.beginGesture}
              onGestureEnd={mix.endGesture}
            />
          </div>
        </section>

        <section className="example-card">
          <div className="section-heading">
            <span>Optional example section</span>
            <span className="section-note">Delete this when you wire in your own UI.</span>
          </div>
          <SpectrumPanel bins={spectrumBins} />
        </section>

        {!hasJuce ? (
          <p className="fallback-banner">
            JUCE host bridge not detected. Run the Vite app inside the plugin to test parameter sync.
          </p>
        ) : null}
      </div>
    </div>
  );
}
