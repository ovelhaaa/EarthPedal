let audioCtx;
let earthNode;
let micSourceNode;
let stream;
let audioBuffer;
let fileSourceNode;

const startButton = document.getElementById('start-audio');
const statusText = document.getElementById('status-text');

const audioSourceSelect = document.getElementById('audioSource');
const filePlayerControls = document.getElementById('file-player-controls');
const audioFileInput = document.getElementById('audioFile');
const playFileBtn = document.getElementById('playFileBtn');
const stopFileBtn = document.getElementById('stopFileBtn');

// Handle Source Switching
audioSourceSelect.addEventListener('change', (e) => {
    const mode = e.target.value;

    // Stop and disconnect file source if playing
    if (fileSourceNode) {
        try { fileSourceNode.stop(); } catch (err) {}
        fileSourceNode.disconnect();
        fileSourceNode = null;
    }

    if (mode === 'mic') {
        filePlayerControls.style.display = 'none';
        if (micSourceNode && earthNode) {
            try { micSourceNode.connect(earthNode); } catch (err) {}
            statusText.textContent = "Mic input active.";
        }
    } else {
        filePlayerControls.style.display = 'block';
        if (micSourceNode) {
            try { micSourceNode.disconnect(); } catch (err) {}
        }
        statusText.textContent = "File player active. Upload and play a file.";
    }
});

// Handle File Upload
audioFileInput.addEventListener('change', async (e) => {
    const file = e.target.files[0];
    if (!file) return;

    if (!audioCtx) {
        statusText.textContent = "Please Start Audio first.";
        return;
    }

    try {
        statusText.textContent = "Loading file...";
        const arrayBuffer = await file.arrayBuffer();
        statusText.textContent = "Decoding audio...";
        audioBuffer = await audioCtx.decodeAudioData(arrayBuffer);
        statusText.textContent = "File ready to play.";
    } catch (err) {
        console.error("Error decoding audio file:", err);
        statusText.textContent = "Error decoding file.";
    }
});

function playFile() {
    if (!audioCtx || !audioBuffer || !earthNode) return;

    if (fileSourceNode) {
        try { fileSourceNode.stop(); } catch (err) {}
        fileSourceNode.disconnect();
    }

    fileSourceNode = audioCtx.createBufferSource();
    fileSourceNode.buffer = audioBuffer;
    fileSourceNode.loop = true;

    fileSourceNode.connect(earthNode);
    fileSourceNode.start();
    statusText.textContent = "Playing file...";
}

playFileBtn.addEventListener('click', () => {
    if (audioSourceSelect.value === 'file') {
        playFile();
    }
});

stopFileBtn.addEventListener('click', () => {
    if (fileSourceNode) {
        try { fileSourceNode.stop(); } catch (err) {}
        fileSourceNode.disconnect();
        fileSourceNode = null;
        statusText.textContent = "Playback stopped.";
    }
});

// Define UI element mappings to Worklet Parameters
const paramMappings = [
    { id: 'mix', isSwitch: false, suffix: '' },
    { id: 'decay', isSwitch: false, suffix: '' },
    { id: 'preDelay', isSwitch: false, suffix: '' },
    { id: 'filter', isSwitch: false, suffix: '' },
    { id: 'modDepth', isSwitch: false, suffix: '' },
    { id: 'modSpeed', isSwitch: false, suffix: '' },
    { id: 'eq1Gain', isSwitch: false, suffix: ' dB' },
    { id: 'eq2Gain', isSwitch: false, suffix: ' dB' },
    { id: 'reverbSize', isSwitch: true },
    { id: 'octaveMode', isSwitch: true },
    { id: 'disableInputDiffusion', isSwitch: true }
];

async function initAudio() {
    try {
        statusText.textContent = "Requesting microphone access...";

        // Request raw microphone input (disable browser processing)
        stream = await navigator.mediaDevices.getUserMedia({
            audio: {
                echoCancellation: false,
                autoGainControl: false,
                noiseSuppression: false,
                latency: 0
            }
        });

        statusText.textContent = "Initializing AudioContext...";
        audioCtx = new (window.AudioContext || window.webkitAudioContext)({
            latencyHint: 'interactive'
        });

        statusText.textContent = "Loading Earth Worklet...";
        await audioCtx.audioWorklet.addModule('earth-worklet-processor.js');

        statusText.textContent = "Fetching WASM binary...";
        const response = await fetch('earth-module.wasm');
        const wasmBytes = await response.arrayBuffer();

        earthNode = new AudioWorkletNode(audioCtx, 'earth-worklet-processor', {
            outputChannelCount: [2]
        });

        // Handle messages from the worklet
        earthNode.port.onmessage = (event) => {
            if (event.data.type === 'ready') {
                statusText.textContent = "Audio processing running.";
                setupUIBindings();
                startButton.classList.add('active');
                startButton.innerHTML = "Power<br>ON";
            }
        };

        // Send initialization data to the worklet
        earthNode.port.postMessage({
            type: 'init',
            wasmBytes: wasmBytes
        });

        // Setup Mic Input but don't connect immediately if not selected
        micSourceNode = audioCtx.createMediaStreamSource(stream);

        if (audioSourceSelect.value === 'mic') {
            micSourceNode.connect(earthNode);
        }
        earthNode.connect(audioCtx.destination);

    } catch (err) {
        console.error("Audio Initialization Error:", err);
        statusText.textContent = `Error: ${err.message}`;
    }
}

function setupUIBindings() {
    paramMappings.forEach(mapping => {
        const el = document.getElementById(mapping.id);
        const valDisplay = document.getElementById(`${mapping.id}-val`);

        if (!el) return;

        // Set initial value from UI to Node
        updateNodeParam(mapping.id, el.value);

        // Add event listener for real-time updates
        el.addEventListener('input', (e) => {
            const val = e.target.value;
            updateNodeParam(mapping.id, val);

            // Update value display if it's a slider
            if (!mapping.isSwitch && valDisplay) {
                const numVal = parseFloat(val);
                // Format appropriately based on magnitude
                const displayStr = mapping.id.startsWith('eq') ? numVal.toFixed(1) : numVal.toFixed(2);
                valDisplay.textContent = displayStr + (mapping.suffix || '');
            }
        });
    });
}

function updateNodeParam(paramName, value) {
    if (earthNode && earthNode.parameters.has(paramName)) {
        earthNode.parameters.get(paramName).value = parseFloat(value);
    }
}

startButton.addEventListener('click', async () => {
    if (!audioCtx) {
        await initAudio();
    } else if (audioCtx.state === 'suspended') {
        await audioCtx.resume();
        startButton.classList.add('active');
        startButton.textContent = "Power\nON";
        statusText.textContent = "Audio processing running.";
    } else if (audioCtx.state === 'running') {
        await audioCtx.suspend();
        startButton.classList.remove('active');
        startButton.textContent = "Start\nAudio";
        statusText.textContent = "Audio paused.";
    }
});
