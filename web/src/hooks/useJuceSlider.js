import { useEffect, useMemo, useState } from 'react';

const clamp = (value, min, max) => Math.min(max, Math.max(min, value));

export function useJuceSlider(paramId, defaultValue = 0) {
  const [value, setValue] = useState(defaultValue);

  useEffect(() => {
    if (typeof Juce === 'undefined') return undefined;

    const sliderState = Juce.getSliderState(paramId);
    if (!sliderState) return undefined;

    const syncFromHost = () => setValue(clamp(sliderState.getNormalisedValue(), 0, 1));
    syncFromHost();

    const listenerId = sliderState.valueChangedEvent.addListener(syncFromHost);
    return () => sliderState.valueChangedEvent.removeListener(listenerId);
  }, [paramId]);

  const api = useMemo(
    () => {
      const setNormalisedValue = (nextValue) => {
        const safeValue = clamp(nextValue, 0, 1);
        setValue(safeValue);

        if (typeof Juce === 'undefined') return;
        const sliderState = Juce.getSliderState(paramId);
        sliderState?.setNormalisedValue(safeValue);
      };

      return {
        value,
        setValue: setNormalisedValue,
        reset() {
          setNormalisedValue(defaultValue);
        },
        beginGesture() {
          if (typeof Juce === 'undefined') return;
          Juce.getSliderState(paramId)?.sliderDragStarted();
        },
        endGesture() {
          if (typeof Juce === 'undefined') return;
          Juce.getSliderState(paramId)?.sliderDragEnded();
        },
      };
    },
    [defaultValue, paramId, value]
  );

  return api;
}
