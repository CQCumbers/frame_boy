// hack to unlock audio on iOS
// see https://hackernoon.com/unlocking-web-audio-the-smarter-way-8858218c0e09
const AudioContext = window.AudioContext || window.webkitAudioContext;
const context = new AudioContext();
if (context.state === 'suspended' && 'ontouchstart' in window) {
  const unlock = () => {
    context.resume().then(() => {
      document.body.removeEventListener('touchstart', unlock);
      document.body.removeEventListener('touchend', unlock);
    });
  };
  document.body.addEventListener('touchstart', unlock, false);
  document.body.addEventListener('touchend', unlock, false);
}

var Module = {
  preRun: [],
  postRun: [],
  canvas: document.getElementById('lcd'),
};

let lastFilename = '';

const loadFile = (input, filename) => {
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
};

const play = () => Module._play();

const save = () => {
  Module._save();
  if (!FS.analyzePath('ram.sav').exists) return;
  const data = FS.readFile('ram.sav');
  const blob = new Blob([data.buffer], {type: 'application/octet-binary'});
  saveAs(blob, lastFilename + '.sav');
};

const simulateKey = (type, code) => {
  var event = new KeyboardEvent(type, {
    'keyCode': code, 'charCode': code, 'view': window,
    'bubbles': true, 'cancelable': true,
  });
  document.body.dispatchEvent(event);
}
