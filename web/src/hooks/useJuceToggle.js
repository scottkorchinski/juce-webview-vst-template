import { useEffect, useMemo, useState } from 'react';

export function useJuceToggle(paramId, defaultValue = false) {
  const [checked, setChecked] = useState(defaultValue);

  useEffect(() => {
    if (typeof Juce === 'undefined') return undefined;

    const toggleState = Juce.getToggleState(paramId);
    if (!toggleState) return undefined;

    const syncFromHost = () => setChecked(Boolean(toggleState.getValue()));
    syncFromHost();

    const listenerId = toggleState.valueChangedEvent.addListener(syncFromHost);
    return () => toggleState.valueChangedEvent.removeListener(listenerId);
  }, [paramId]);

  return useMemo(() => {
    const setToggleValue = (nextChecked) => {
        const safeValue = Boolean(nextChecked);
        setChecked(safeValue);

        if (typeof Juce === 'undefined') return;
        Juce.getToggleState(paramId)?.setValue(safeValue);
    };

    return {
      checked,
      setChecked: setToggleValue,
      toggle() {
        setToggleValue(!checked);
      },
    };
  }, [checked, paramId]);
}
