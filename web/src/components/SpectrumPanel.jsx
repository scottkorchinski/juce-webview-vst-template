const clamp = (value, min, max) => Math.min(max, Math.max(min, value));

export function SpectrumPanel({ bins }) {
  const width = 320;
  const height = 136;
  const baseline = 116;
  const pointStep = width / Math.max(1, bins.length - 1);

  const line = bins
    .map((value, index) => `${index * pointStep},${baseline - clamp(value, 0, 1) * 88}`)
    .join(' ');

  return (
    <div className="spectrum-panel">
      <div className="spectrum-panel-header">
        <span>Optional analyzer example</span>
        <span>Native event bridge</span>
      </div>
      <svg viewBox={`0 0 ${width} ${height}`} className="spectrum-svg" preserveAspectRatio="none">
        <line x1="80" y1="0" x2="80" y2={height} className="spectrum-grid" />
        <line x1="160" y1="0" x2="160" y2={height} className="spectrum-grid" />
        <line x1="240" y1="0" x2="240" y2={height} className="spectrum-grid" />
        <polyline points={line} className="spectrum-line" />
      </svg>
    </div>
  );
}
