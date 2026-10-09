/* The DOS game remains unchanged; this shell supplies display, keys and FM audio. */
'use strict';
const $ = id => document.getElementById(id);
const canvas = $('screen');
const ctx = canvas.getContext('2d', {alpha: false});
let engine = null, generation = 0, loading = false, paused = false, audible = false;
let frame, dirty = false, audio, nextAudioTime = 0, scriptPromise;
const sources = new Set(), held = new Set();
const keys = {Enter:257, Escape:256, Backspace:259, Delete:261, ArrowRight:262,
  ArrowLeft:263, ArrowDown:264, ArrowUp:265, PageUp:266, PageDown:267,
  Home:268, End:269, F1:290, F2:291, F3:292, Space:32, BracketLeft:91,
  BracketRight:93, NumpadEnter:257};
for (let i = 0; i < 10; i++) { keys['Digit'+i] = 48+i; keys['Numpad'+i] = 48+i; }
for (let i = 0; i < 26; i++) keys['Key'+String.fromCharCode(65+i)] = 65+i;

function status(text) { $('status').textContent = text; }
function releaseKeys() {
  for (const key of held) engine?.sendKeyEvent(key, false);
  held.clear();
}
function flushAudio() {
  for (const source of sources) { source.onended = null; source.stop(); }
  sources.clear(); nextAudioTime = 0;
}
function buttons() {
  for (const id of ['sound', 'pause', 'restart']) $(id).disabled = !engine || loading;
  for (const button of document.querySelectorAll('[data-key]')) button.disabled = !engine || paused;
  $('sound').textContent = audible ? 'Sound on' : 'Sound off';
  $('sound').setAttribute('aria-pressed', String(audible));
  $('pause').textContent = paused ? 'Resume' : 'Pause';
  $('pause').setAttribute('aria-pressed', String(paused));
}
function resize(width, height) {
  if (width < 1 || height < 1) return;
  canvas.width = width; canvas.height = height;
  frame = ctx.createImageData(width, height);
}
function draw(rgb, rgba) {
  if (!frame) return;
  if (rgba) frame.data.set(rgba);
  else if (rgb) {
    for (let i=0, j=0; i<rgb.length; i+=3, j+=4) {
      frame.data[j]=rgb[i]; frame.data[j+1]=rgb[i+1];
      frame.data[j+2]=rgb[i+2]; frame.data[j+3]=255;
    }
  }
  dirty = true;
}
function paint() {
  if (dirty && frame) { ctx.putImageData(frame, 0, 0); dirty = false; }
  requestAnimationFrame(paint);
}
requestAnimationFrame(paint);
function sound(samples) {
  if (!engine || !audible || paused || !audio || audio.state !== 'running' || !samples.length) return;
  const now = audio.currentTime;
  // Bound latency instead of queueing seconds of sound after a background tab.
  if (nextAudioTime > now + .3) return;
  nextAudioTime = Math.max(now + .025, nextAudioTime);
  const buffer = audio.createBuffer(1, samples.length, engine.soundFrequency());
  buffer.copyToChannel(samples, 0);
  const source = audio.createBufferSource(); source.buffer = buffer;
  source.connect(audio.destination); sources.add(source);
  source.onended = () => sources.delete(source);
  source.start(nextAudioTime); nextAudioTime += buffer.duration;
}
function loadEmulator() {
  if (!scriptPromise) scriptPromise = new Promise((resolve, reject) => {
    const script = document.createElement('script'); script.src = 'emulator/emulators.js';
    script.onload = resolve;
    script.onerror = () => { script.remove(); scriptPromise = null;
      reject(new Error('Could not load the emulator. Check your connection and try again.')); };
    document.head.append(script);
  });
  return scriptPromise;
}
async function start() {
  if (loading) return;
  const ticket = ++generation;
  loading = true; $('start').disabled = true; $('error').hidden = true;
  status('Loading the DOS game…'); buttons(); releaseKeys(); flushAudio();
  try {
    if (engine) { const previous = engine; engine = null; await previous.exit(); }
    paused = false;
    await loadEmulator();
    window.emulators.pathPrefix = new URL('emulator/', location.href).href;
    const response = await fetch('KAKURO.jsdos');
    if (!response.ok) throw new Error('Could not load the game archive. Please try again.');
    const instance = await window.emulators.dosboxWorker(new Uint8Array(await response.arrayBuffer()));
    if (ticket !== generation) { await instance.exit(); return; }
    engine = instance; resize(engine.width(), engine.height());
    engine.events().onFrameSize(resize); engine.events().onFrame(draw);
    // An exiting worker can still deliver queued audio during a restart.
    engine.events().onSoundPush(samples => {
      if (engine === instance && !loading) sound(samples);
    });
    const finished = () => {
      if (engine !== instance) return;
      releaseKeys(); engine = null; paused = false; flushAudio(); buttons();
      $('cover').hidden = false; status('Ready to start');
    };
    engine.events().onExit(finished);
    // DOSBox's worker does not emit onExit when AUTOEXEC's EXIT halts DOS.
    // The shell reports completion, then we explicitly terminate the worker.
    engine.events().onStdout(message => {
      if (message.includes('KAKURO-BROWSER-SESSION-END') && engine === instance) {
        finished(); instance.exit().catch(console.error);
      }
    });
    if (!audible) engine.mute();
    $('cover').hidden = true; status('Game running'); canvas.focus();
  } catch (error) {
    console.error(error); engine = null; flushAudio(); $('cover').hidden = false;
    $('error').textContent = error.message || 'The game could not start. Please try again.';
    $('error').hidden = false; status('Unable to start the game');
  } finally {
    loading = false; $('start').disabled = false; buttons();
  }
}
$('start').addEventListener('click', start);
$('restart').addEventListener('click', start);
$('sound').addEventListener('click', async () => {
  audible = !audible; flushAudio();
  if (audible) {
    try {
      audio ??= new (window.AudioContext || window.webkitAudioContext)();
      await audio.resume(); engine?.unmute();
    } catch (error) {
      audible = false;
      $('error').textContent = 'Sound could not start in this browser.'; $('error').hidden = false;
    }
  } else engine?.mute();
  buttons(); canvas.focus();
});
$('pause').addEventListener('click', () => {
  releaseKeys(); paused = !paused; flushAudio();
  if (paused) engine.pause(); else engine.resume();
  status(paused ? 'Game paused' : 'Game running');
  buttons(); canvas.focus();
});
$('fullscreen').addEventListener('click', async () => {
  try {
    if (document.fullscreenElement) await document.exitFullscreen();
    else await $('player').requestFullscreen();
    canvas.focus();
  } catch (_) { status('Fullscreen is unavailable in this browser'); }
});
if (!$('player').requestFullscreen) $('fullscreen').hidden = true;
canvas.addEventListener('keydown', event => {
  const key = keys[event.code];
  if (!engine || paused || key === undefined || event.ctrlKey || event.metaKey || event.altKey) return;
  event.preventDefault();
  if (!held.has(key)) { held.add(key); engine.sendKeyEvent(key, true); }
});
canvas.addEventListener('keyup', event => {
  const key = keys[event.code];
  if (!held.has(key)) return;
  event.preventDefault(); held.delete(key); engine?.sendKeyEvent(key, false);
});
canvas.addEventListener('blur', releaseKeys);
window.addEventListener('blur', releaseKeys);
document.addEventListener('visibilitychange', () => {
  if (document.hidden) { releaseKeys(); flushAudio(); }
});
for (const button of document.querySelectorAll('[data-key]')) {
  button.addEventListener('click', () => {
    if (engine && !paused) { engine.simulateKeyPress(Number(button.dataset.key)); canvas.focus(); }
  });
}
window.addEventListener('pagehide', () => { generation++; releaseKeys(); engine?.exit(); audio?.close(); });
fetch('build.json').then(response => response.json()).then(info => {
  $('version').textContent = 'v' + info.version + ' · ' + info.commit.slice(0, 7);
}).catch(() => {});
buttons();
