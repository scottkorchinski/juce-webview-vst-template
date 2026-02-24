import React from 'react';
import ReactDOM from 'react-dom/client';
import App from './App';
import './index.css';

async function bootstrap() {
  // Load JUCE frontend when running inside the plugin WebView (sets window.Juce
  // for getSliderState/getToggleState). If this fails (e.g. running in a normal
  // browser), fall back to no JUCE backend so the app can still render.
  try {
    const Juce = await import('@juce/index.js');
    window.Juce = Juce;
  } catch {
    window.Juce = undefined;
  }

  ReactDOM.createRoot(document.getElementById('root')).render(
    <React.StrictMode>
      <App />
    </React.StrictMode>
  );
}

bootstrap();
