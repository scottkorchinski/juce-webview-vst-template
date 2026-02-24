(function () {
  if (typeof Juce === 'undefined') {
    document.body.innerHTML = '<div style="padding:16px;font-family:system-ui">JUCE backend not present (run inside the plugin for full UI).</div>';
    return;
  }

  function bindSlider(paramId, labelText) {
    var state = Juce.getSliderState(paramId);
    if (!state) return;

    var row = document.createElement('div');
    row.className = 'row';
    var label = document.createElement('label');
    label.textContent = labelText;
    var input = document.createElement('input');
    input.type = 'range';
    input.min = 0;
    input.max = 1000;
    var valueSpan = document.createElement('span');
    valueSpan.className = 'value';

    function updateInput() {
      var norm = state.getNormalisedValue();
      input.value = Math.round(norm * 1000);
      valueSpan.textContent = Math.round(norm * 100) + '%';
    }

    state.valueChangedEvent.addListener(updateInput);
    updateInput();

    input.addEventListener('input', function () {
      state.setNormalisedValue(Number(input.value) / 1000);
    });
    input.addEventListener('mousedown', function () { state.sliderDragStarted(); });
    input.addEventListener('mouseup', function () { state.sliderDragEnded(); });

    row.appendChild(label);
    row.appendChild(input);
    row.appendChild(valueSpan);
    return row;
  }

  function bindBypass() {
    var state = Juce.getToggleState('bypass');
    if (!state) return null;

    var row = document.createElement('div');
    row.className = 'bypass-row';
    var label = document.createElement('label');
    var input = document.createElement('input');
    input.type = 'checkbox';
    input.checked = Boolean(state.getValue());

    state.valueChangedEvent.addListener(function () {
      input.checked = Boolean(state.getValue());
    });
    input.addEventListener('change', function () {
      state.setValue(input.checked);
    });

    label.appendChild(input);
    label.appendChild(document.createTextNode('Bypass'));
    row.appendChild(label);
    return row;
  }

  var strip = document.createElement('div');
  strip.className = 'module-strip';
  var title = document.createElement('h2');
  title.textContent = 'Lifeline Module';
  strip.appendChild(title);

  strip.appendChild(bindSlider('gain', 'Gain'));
  strip.appendChild(bindSlider('tone', 'Tone'));
  strip.appendChild(bindSlider('mix', 'Mix'));
  var bypass = bindBypass();
  if (bypass) strip.appendChild(bypass);

  var root = document.getElementById('root');
  if (root) { root.innerHTML = ''; root.appendChild(strip); }
})();
