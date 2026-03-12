import { useMemo, useRef } from 'react';

const clamp = (value, min, max) => Math.min(max, Math.max(min, value));

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

  return ['M', start.x, start.y, 'A', radius, radius, 0, largeArcFlag, 0, end.x, end.y].join(' ');
}

export function ParameterKnob({
  label,
  value,
  valueText,
  accent = '#67e8f9',
  disabled = false,
  onChange,
  onReset,
  onGestureStart,
  onGestureEnd,
}) {
  const startYRef = useRef(0);
  const startValueRef = useRef(value);

  const angle = useMemo(() => -135 + value * 270, [value]);
  const arcPath = useMemo(() => describeArc(48, 48, 42, -135, angle), [angle]);

  const beginDrag = (clientY) => {
    startYRef.current = clientY;
    startValueRef.current = value;
    onGestureStart?.();
  };

  const updateDrag = (clientY) => {
    const delta = (startYRef.current - clientY) * 0.004;
    onChange?.(clamp(startValueRef.current + delta, 0, 1));
  };

  const handleMouseDown = (event) => {
    if (disabled) return;

    event.preventDefault();
    beginDrag(event.clientY);

    const handleMove = (moveEvent) => updateDrag(moveEvent.clientY);
    const handleUp = () => {
      window.removeEventListener('mousemove', handleMove);
      window.removeEventListener('mouseup', handleUp);
      onGestureEnd?.();
    };

    window.addEventListener('mousemove', handleMove);
    window.addEventListener('mouseup', handleUp);
  };

  return (
    <div className={`parameter-knob ${disabled ? 'is-disabled' : ''}`}>
      <button
        type="button"
        className="parameter-knob-hit"
        onMouseDown={handleMouseDown}
        onDoubleClick={disabled ? undefined : onReset}
        aria-label={label}
      >
        <svg viewBox="0 0 96 96" className="parameter-knob-svg" aria-hidden="true">
          <path d={describeArc(48, 48, 42, -135, 135)} className="parameter-knob-track" />
          <path d={arcPath} className="parameter-knob-fill" style={{ stroke: accent }} />
          <g transform={`rotate(${angle} 48 48)`}>
            <circle cx="48" cy="48" r="28" className="parameter-knob-body" />
            <rect x="46" y="18" width="4" height="18" rx="2" className="parameter-knob-pointer" />
          </g>
        </svg>
      </button>
      <div className="parameter-knob-meta">
        <span className="parameter-knob-label">{label}</span>
        <span className="parameter-knob-value">{valueText}</span>
      </div>
    </div>
  );
}
