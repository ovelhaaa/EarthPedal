let audioCtx;
let earthNode;
let sourceNode;
let stream;

const startButton = document.getElementById('start-audio');
const statusText = document.getElementById('status-text');

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
                startButton.textContent = "Power\nON";
            }
        };

        // Send initialization data to the worklet
        earthNode.port.postMessage({
            type: 'init',
            wasmBytes: wasmBytes
        });

        // Connect the audio graph
        sourceNode = audioCtx.createMediaStreamSource(stream);
        sourceNode.connect(earthNode);
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
