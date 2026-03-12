const clamp = (value, min, max) => Math.min(max, Math.max(min, value));

export function MeterPair({ left = 0, right = 0 }) {
  return (
    <div className="meter-pair" aria-label="Output meters">
      {[left, right].map((level, index) => (
        <div className="meter-rail" key={index}>
          <div className="meter-fill" style={{ transform: `scaleY(${clamp(level, 0, 1.2)})` }} />
        </div>
      ))}
    </div>
  );
}
