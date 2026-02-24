// Load JUCE frontend as ES module and expose as window.Juce so main.js can use it.
import * as Juce from './js/juce/javascript/index.js';
window.Juce = Juce;
// Load main UI script after Juce is ready
const s = document.createElement('script');
s.src = 'main.js';
document.body.appendChild(s);
