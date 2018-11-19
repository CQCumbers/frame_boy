// register offline service worker

if (!navigator.serviceWorker.controller) {
  navigator.serviceWorker.register('worker.js', {scope: './'});
}

// setup file system

const setupFS = () => {
  // load files from indexedDB
  FS.mkdir('/data');
  FS.mount(IDBFS, {}, '/data');
  FS.syncfs(true, err => {
    FS.currentPath = '/data';
    if (!FS.analyzePath('filename.txt').exists) return;
    lastFilename = FS.readFile('filename.txt', {encoding: 'utf8'});
    document.getElementById('rom').labels[0].innerHTML = lastFilename + '.gb';
    document.getElementById('ram').labels[0].innerHTML = lastFilename + '.sav';
  });
}

window.onbeforeunload = () => {
  // sync files to indexedDB
  Module.ccall('save', null, ['string'], ['ram.sav']);
  FS.syncfs(false, err => {});
}

var lastFilename = '';
var Module = {
  canvas: document.getElementById('lcd'),
  onRuntimeInitialized: setupFS
};

// create event listeners

const AudioContext = window.AudioContext || window.webkitAudioContext;
const context = new AudioContext();

const loadFile = (input, filename) => {
  if (context.state === 'suspended') context.resume();
  if (input.files.length == 0) return;
  const file = input.files[0];
  let fr = new FileReader();
  fr.readAsArrayBuffer(file);
  fr.onload = () => {
    const data = new Uint8Array(fr.result);
    FS.writeFile(filename, data);
  };
  input.labels[0].innerHTML = file.name;
  lastFilename = file.name.replace(/\.[^/.]+$/, '');
  FS.writeFile('filename.txt', lastFilename);
};

const save = () => {
  Module.ccall('save', null, ['string'], ['ram.sav']);
  if (!FS.analyzePath('ram.sav').exists) return;
  const data = FS.readFile('ram.sav');
  const blob = new Blob([data.buffer], {type: 'application/octet-binary'});
  saveAs(blob, lastFilename + '.sav');
};

const load = () => {
  if (!FS.analyzePath('rom.gb').exists) return;
  Module.ccall('load', null, ['string', 'string'], ['rom.gb', 'ram.sav']);
};

const simulateKey = (type, code) => {
  var event = new KeyboardEvent(type, {
    'keyCode': code, 'charCode': code, 'view': window,
    'bubbles': true, 'cancelable': true,
  });
  document.body.dispatchEvent(event);
}

// attach event listeners

document.getElementById('rom')
  .addEventListener('change', e => loadFile(e.target, 'rom.gb'));
document.getElementById('ram')
  .addEventListener('change', e => loadFile(e.target, 'ram.sav'));
document.getElementById('load')
  .addEventListener('click', e => load());
document.getElementById('save')
  .addEventListener('click', e => save());

const bindings = new Map([
  ['select', 8], ['start', 13], ['a', 88], ['b', 90],
  ['left', 37], ['right', 39], ['up', 38], ['down', 40]
]);
bindings.forEach((code, id) => {
  document.getElementById(id + '-btn')
    .addEventListener('pointerenter', e => simulateKey('keydown', code));
  document.getElementById(id + '-btn')
    .addEventListener('pointerleave', e => simulateKey('keyup', code));
});
