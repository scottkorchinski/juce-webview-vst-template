import { useEffect, useMemo, useRef, useState } from 'react';
import './App.css';

const clamp = (v, min, max) => Math.min(max, Math.max(min, v));
const BASE_WIDTH = 320;
const BASE_HEIGHT = 720;

function polarToCartesian(centerX, centerY, radius, angleInDegrees) {
  const angleInRadians = ((angleInDegrees - 90) * Math.PI) / 180;
  return {
    x: centerX + radius * Math.cos(angleInRadians),
    y: centerY + radius * Math.sin(angleInRadians),
  };
}

function describeArc(x, y, radius, startAngle, endAngle) {
  const start = polarToCartesian(x, y, radius, endAngle);
  const end = polarToCartesian(x, y, radius, startAngle);
  const largeArcFlag = endAngle - startAngle <= 180 ? '0' : '1';
  return [
    'M',
    start.x,
    start.y,
    'A',
    radius,
    radius,
    0,
    largeArcFlag,
    0,
    end.x,
    end.y,
  ].join(' ');
}

function useSliderState(paramId, initialPercent) {
  const [percent, setPercent] = useState(initialPercent);

  useEffect(() => {
    if (typeof Juce === 'undefined') return;
    const state = Juce.getSliderState(paramId);
    if (!state) return;

    setPercent(Math.round(state.getNormalisedValue() * 100));
    const listener = () => setPercent(Math.round(state.getNormalisedValue() * 100));
    const listenerId = state.valueChangedEvent.addListener(listener);
    return () => state.valueChangedEvent.removeListener(listenerId);
  }, [paramId]);

  const setFromUi = (newPercent) => {
    const safe = clamp(Math.round(newPercent), 0, 100);
    setPercent(safe);
    if (typeof Juce === 'undefined') return;
    const state = Juce.getSliderState(paramId);
    if (state) state.setNormalisedValue(safe / 100);
  };

  const resetToDefault = () => {
    setFromUi(initialPercent);
  };

  const dragStart = () => {
    if (typeof Juce === 'undefined') return;
    const state = Juce.getSliderState(paramId);
    if (state) state.sliderDragStarted();
  };

  const dragEnd = () => {
    if (typeof Juce === 'undefined') return;
    const state = Juce.getSliderState(paramId);
    if (state) state.sliderDragEnded();
  };

  return { percent, setFromUi, resetToDefault, dragStart, dragEnd };
}

function useBypassState() {
  const [checked, setChecked] = useState(false);

  useEffect(() => {
    if (typeof Juce === 'undefined') return;
    const state = Juce.getToggleState('bypass');
    if (!state) return;
    setChecked(Boolean(state.getValue()));
    const listener = () => setChecked(Boolean(state.getValue()));
    const listenerId = state.valueChangedEvent.addListener(listener);
    return () => state.valueChangedEvent.removeListener(listenerId);
  }, []);

  const setFromUi = (next) => {
    const safe = Boolean(next);
    setChecked(safe);
    if (typeof Juce === 'undefined') return;
    const state = Juce.getToggleState('bypass');
    if (state) state.setValue(safe);
  };

  return { checked, setFromUi };
}

function Knob({
  label,
  percent,
  onChange,
  onReset,
  onDragStart,
  onDragEnd,
  color,
  dark,
  size = 90,
  bipolar = false,
  centerPercent = 50,
  compactLabel = false,
  disabled = false,
}) {
  const center = size / 2;
  const radius = size / 2 - 4;
  const rotation = percent * 2.7 - 135;
  const bgPath = describeArc(center, center, radius, -135, 135);
  const centerAngle = centerPercent * 2.7 - 135;
  const isCentered = Math.abs(percent - centerPercent) < 0.5;
  const activeStart = bipolar ? Math.min(centerAngle, rotation) : -135;
  const activeEnd = bipolar ? Math.max(centerAngle, rotation) : rotation;
  const activePath = describeArc(center, center, radius, activeStart, activeEnd);
  const activeStroke = bipolar ? (isCentered ? '#d1d5db' : color) : color;
  const capSize = size * 0.7;
  const pointerWidth = Math.max(1.5, size * 0.03);
  const pointerHeight = Math.max(12, size * 0.26);
  const pointerMarginTop = Math.max(4, size * 0.08);

  const startYRef = useRef(0);
  const startPercentRef = useRef(percent);

  const beginDrag = (clientY) => {
    startYRef.current = clientY;
    startPercentRef.current = percent;
    onDragStart();
  };

  const handleMove = (clientY) => {
    const deltaY = startYRef.current - clientY;
    const sensitivity = 0.35;
    onChange(startPercentRef.current + deltaY * sensitivity);
  };

  const onMouseDown = (e) => {
    if (disabled) return;
    e.preventDefault();
    beginDrag(e.clientY);

    const move = (ev) => handleMove(ev.clientY);
    const up = () => {
      window.removeEventListener('mousemove', move);
      window.removeEventListener('mouseup', up);
      onDragEnd();
    };

    window.addEventListener('mousemove', move);
    window.addEventListener('mouseup', up);
  };

  return (
    <div className="knob-wrap" style={{ width: size + 8 }}>
      <div
        className="knob"
        style={{ width: size, height: size }}
        onMouseDown={onMouseDown}
        onDoubleClick={disabled ? undefined : onReset}
      >
        <svg width={size} height={size} className="knob-svg">
          <path d={bgPath} fill="none" stroke="#d1d5db" strokeWidth="3" strokeLinecap="round" />
          {!bipolar || !isCentered ? (
            <path d={activePath} fill="none" stroke={activeStroke} strokeWidth="3" strokeLinecap="round" />
          ) : null}
        </svg>
        <div
          className={`knob-cap ${dark ? 'knob-cap-dark' : 'knob-cap-light'}`}
          style={{
            width: capSize,
            height: capSize,
            transform: `translate(-50%, -50%) rotate(${rotation}deg)`,
          }}
        >
          <div
            className={`knob-pointer ${dark ? 'knob-pointer-dark' : 'knob-pointer-light'}`}
            style={{
              width: pointerWidth,
              height: pointerHeight,
              marginTop: pointerMarginTop,
            }}
          />
        </div>
      </div>
      {label ? <span className={`knob-label ${compactLabel ? 'knob-label-compact' : ''}`}>{label}</span> : null}
    </div>
  );
}

function HeaderGainKnob({ percent, onChange, onReset, onDragStart, onDragEnd, disabled }) {
  const size = 30;
  const center = size / 2;
  const rotation = percent * 2.7 - 135;
  const startYRef = useRef(0);
  const startPercentRef = useRef(percent);

  const beginDrag = (clientY) => {
    startYRef.current = clientY;
    startPercentRef.current = percent;
    onDragStart();
  };

  const handleMove = (clientY) => {
    const deltaY = startYRef.current - clientY;
    onChange(startPercentRef.current + deltaY * 0.35);
  };

  const onMouseDown = (e) => {
    if (disabled) return;
    e.preventDefault();
    beginDrag(e.clientY);

    const move = (ev) => handleMove(ev.clientY);
    const up = () => {
      window.removeEventListener('mousemove', move);
      window.removeEventListener('mouseup', up);
      onDragEnd();
    };

    window.addEventListener('mousemove', move);
    window.addEventListener('mouseup', up);
  };

  return (
    <button
      type="button"
      className="header-gain-knob"
      onMouseDown={onMouseDown}
      onDoubleClick={disabled ? undefined : onReset}
      aria-label="Output gain"
      title="Output gain (double-click to reset)"
    >
      <svg width={size} height={size}>
        <circle cx={center} cy={center} r={13.5} fill="#efefef" />
        <g transform={`rotate(${rotation} ${center} ${center})`}>
          <line x1={center} y1={center} x2={center} y2={5} stroke="#8b8b8b" strokeWidth="2" strokeLinecap="round" />
        </g>
      </svg>
    </button>
  );
}

function SpectrumPanel({ bins }) {
  const width = 248;
  const height = 110;
  const usableHeight = 78;
  const floorY = 30 + usableHeight;
  const pointStep = width / (bins.length - 1);

  const linePoints = bins
    .map((value, index) => {
      const x = index * pointStep;
      const y = floorY - clamp(value, 0, 1) * usableHeight;
      return `${x},${y}`;
    })
    .join(' ');

  const fillPoints = `${linePoints} ${width},${height} 0,${height}`;

  return (
    <div className="eq-panel">
      <div className="eq-controls">
        <div className="eq-circle" />
        <div className="eq-chip">Effect</div>
      </div>
      <svg width="100%" height="100%" viewBox="0 0 248 110" preserveAspectRatio="none">
        <line x1="62" y1="0" x2="62" y2="110" stroke="#c8c8d0" strokeWidth="1" />
        <line x1="124" y1="0" x2="124" y2="110" stroke="#c8c8d0" strokeWidth="1" />
        <line x1="186" y1="0" x2="186" y2="110" stroke="#c8c8d0" strokeWidth="1" />
        <polygon points={fillPoints} fill="rgba(160,152,185,0.35)" />
        <polyline points={linePoints} fill="none" stroke="#a098b9" strokeWidth="1.5" />
      </svg>
    </div>
  );
}

export default function App() {
  const rootRef = useRef(null);
  const hasJuce =
    typeof Juce !== 'undefined' &&
    typeof window.__JUCE__ !== 'undefined' &&
    window.__JUCE__.initialisationData?.__juce__sliders?.length > 0;

  const gain = useSliderState('gain', 0);
  const tone = useSliderState('tone', 50);
  const mix = useSliderState('mix', 100);
  const outputGain = useSliderState('outputGain', 50);
  const bypass = useBypassState();
  const [noise, setNoise] = useState(35);
  const [smooth, setSmooth] = useState(35);
  const [isMain, setIsMain] = useState(true);
  const [meters, setMeters] = useState({ left: 0, right: 0 });
  const [spectrumBins, setSpectrumBins] = useState(() => new Array(72).fill(0));
  const [uiScale, setUiScale] = useState(1);

  useEffect(() => {
    const updateScaleFromRect = (rect) => {
      const availableWidth = Math.max(1, rect.width);
      const availableHeight = Math.max(1, rect.height);
      const scale = Math.min(availableWidth / BASE_WIDTH, availableHeight / BASE_HEIGHT);
      setUiScale(scale);
    };

    const node = rootRef.current;
    if (!node) return;

    const observer = new ResizeObserver((entries) => {
      if (entries[0]) updateScaleFromRect(entries[0].contentRect);
    });
    observer.observe(node);
    updateScaleFromRect(node.getBoundingClientRect());

    return () => observer.disconnect();
  }, []);

  useEffect(() => {
    if (typeof window === 'undefined' || typeof window.__JUCE__ === 'undefined' || !window.__JUCE__.backend)
      return;

    const meterToken = window.__JUCE__.backend.addEventListener('meterData', (event) => {
      setMeters({
        left: clamp(Number(event?.left ?? 0), 0, 1.2),
        right: clamp(Number(event?.right ?? 0), 0, 1.2),
      });
    });

    const spectrumToken = window.__JUCE__.backend.addEventListener('spectrumData', (event) => {
      if (!Array.isArray(event)) return;
      setSpectrumBins(event.map((v) => clamp(Number(v ?? 0), 0, 1)));
    });

    return () => {
      window.__JUCE__.backend.removeEventListener(meterToken);
      window.__JUCE__.backend.removeEventListener(spectrumToken);
    };
  }, []);

  const resetToDefaults = () => {
    gain.setFromUi(0);
    tone.setFromUi(50);
    mix.setFromUi(100);
    outputGain.setFromUi(50);
  };

  const stageStyle = useMemo(
    () => ({
      width: `${BASE_WIDTH}px`,
      height: `${BASE_HEIGHT}px`,
      transform: `scale(${uiScale})`,
    }),
    [uiScale]
  );

  return (
    <div className="format-root" ref={rootRef}>
      <div className="format-stage" style={stageStyle}>
      <div className={`format-unit ${bypass.checked ? 'is-disabled' : ''}`}>
        <div className="format-header">
          <button
            type="button"
            className={`power-dot-button ${bypass.checked ? 'is-off' : ''}`}
            onClick={() => bypass.setFromUi(!bypass.checked)}
            aria-label="Bypass"
            title="Bypass"
          >
            <span className="power-dot" />
          </button>
          <div className="format-title">FORMAT</div>
          <div className="header-icons">
            <HeaderGainKnob
              percent={outputGain.percent}
              onChange={outputGain.setFromUi}
              onReset={outputGain.resetToDefault}
              onDragStart={outputGain.dragStart}
              onDragEnd={outputGain.dragEnd}
              disabled={bypass.checked}
            />
            <div className="meter-pair" aria-label="Input level meters">
              <div className="meter-rail">
                <div className="meter-fill" style={{ transform: `scaleY(${meters.left})` }} />
              </div>
              <div className="meter-rail">
                <div className="meter-fill" style={{ transform: `scaleY(${meters.right})` }} />
              </div>
            </div>
          </div>
        </div>

        <div className="preset-block">
          <button type="button" className="arrow-btn" aria-label="Previous">‹</button>
          <span className="preset-name">Degrade</span>
          <button type="button" className="arrow-btn" aria-label="Next">›</button>
        </div>
        <div className="dots">
          <span className="dot dot-active" />
          <span className="dot" />
          <span className="dot" />
          <span className="dot" />
        </div>

        {isMain ? (
          <>
            <div className="visualizer">
              <svg width="120" height="60" viewBox="0 0 120 60">
                <path d="M10 30 Q 20 5 30 30 T 50 30" fill="none" stroke="#a59eb5" strokeWidth="4" strokeLinecap="round" />
                <rect x="60" y="20" width="8" height="20" fill="#a59eb5" opacity="0.8" />
                <rect x="72" y="10" width="8" height="40" fill="#a59eb5" opacity="0.6" />
                <rect x="84" y="25" width="8" height="10" fill="#a59eb5" opacity="0.4" />
                <rect x="96" y="15" width="8" height="30" fill="#a59eb5" opacity="0.8" />
              </svg>
            </div>

            <div className="knob-column">
              <Knob
                label="Crush"
                percent={gain.percent}
                onChange={gain.setFromUi}
                onReset={gain.resetToDefault}
                onDragStart={gain.dragStart}
                onDragEnd={gain.dragEnd}
                color="#a59eb5"
                size={90}
                disabled={bypass.checked}
              />
              <Knob
                label="Tone"
                percent={tone.percent}
                onChange={tone.setFromUi}
                onReset={tone.resetToDefault}
                onDragStart={tone.dragStart}
                onDragEnd={tone.dragEnd}
                color="#a59eb5"
                bipolar
                centerPercent={50}
                size={90}
                disabled={bypass.checked}
              />
              <Knob
                label="Mix"
                percent={mix.percent}
                onChange={mix.setFromUi}
                onReset={mix.resetToDefault}
                onDragStart={mix.dragStart}
                onDragEnd={mix.dragEnd}
                color="#5b5b5b"
                dark
                size={90}
                disabled={bypass.checked}
              />
            </div>
            <div className="main-bottom-spacer" />
          </>
        ) : (
          <>
            <div className="adv-curve-wrap">
              <SpectrumPanel bins={spectrumBins} />
            </div>
            <div className="adv-knob-column">
              <Knob
                label="Noise"
                percent={noise}
                onChange={(v) => setNoise(clamp(Math.round(v), 0, 100))}
                onReset={() => setNoise(35)}
                onDragStart={() => {}}
                onDragEnd={() => {}}
                color="#b0aabf"
                size={85}
                disabled={bypass.checked}
              />
              <Knob
                label="Smooth"
                percent={smooth}
                onChange={(v) => setSmooth(clamp(Math.round(v), 0, 100))}
                onReset={() => setSmooth(35)}
                onDragStart={() => {}}
                onDragEnd={() => {}}
                color="#b0aabf"
                size={85}
                disabled={bypass.checked}
              />
            </div>
            <div className="adv-mini-row">
              <Knob
                label="Crush"
                percent={gain.percent}
                onChange={gain.setFromUi}
                onReset={gain.resetToDefault}
                onDragStart={gain.dragStart}
                onDragEnd={gain.dragEnd}
                color="#a59eb5"
                size={56}
                compactLabel
                disabled={bypass.checked}
              />
              <Knob
                label="Tone"
                percent={tone.percent}
                onChange={tone.setFromUi}
                onReset={tone.resetToDefault}
                onDragStart={tone.dragStart}
                onDragEnd={tone.dragEnd}
                color="#a59eb5"
                bipolar
                centerPercent={50}
                size={56}
                compactLabel
                disabled={bypass.checked}
              />
              <Knob
                label="Mix"
                percent={mix.percent}
                onChange={mix.setFromUi}
                onReset={mix.resetToDefault}
                onDragStart={mix.dragStart}
                onDragEnd={mix.dragEnd}
                color="#5b5b5b"
                dark
                size={56}
                compactLabel
                disabled={bypass.checked}
              />
            </div>
          </>
        )}

        {!hasJuce && <p className="fallback">JUCE backend not present. Run inside plugin for full control sync.</p>}

        <div className="format-footer">
          <div className="main-adv">
            <span className={isMain ? 'footer-on' : 'footer-off'}>Main</span>
            <button
              type="button"
              className="main-toggle"
              onClick={() => setIsMain((v) => !v)}
              aria-label="Main or advanced mode"
            >
              <span className={`main-toggle-dot ${isMain ? 'toggle-left' : 'toggle-right'}`} />
            </button>
            <span className={!isMain ? 'footer-on' : 'footer-off'}>Adv.</span>
          </div>
          <button
            type="button"
            className="icon-btn"
            aria-label="Reset (double-click)"
            onDoubleClick={resetToDefaults}
            title="Double-click to reset defaults"
          >
            ↻
          </button>
        </div>
      </div>
      </div>
    </div>
  );
}
