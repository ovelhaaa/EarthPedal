let audioCtx;
let earthNode;
let micSourceNode;
let micStream;
let audioBuffer;
let fileSourceNode;
let uiBindingsInitialized = false;

const startButton = document.getElementById('start-audio');
const statusText = document.getElementById('status-text');

const audioSourceSelect = document.getElementById('audioSource');
const filePlayerControls = document.getElementById('file-player-controls');
const audioFileInput = document.getElementById('audioFile');
const playFileBtn = document.getElementById('playFileBtn');
const stopFileBtn = document.getElementById('stopFileBtn');

const WORKLET_URL = new URL('./earth-worklet-processor.js', import.meta.url).href;
const WASM_URL = new URL('./earth-module.wasm', import.meta.url).href;

function setStatus(message) {
    statusText.textContent = message;
}

function logError(context, err) {
    console.error(`[EarthPedal] ${context}`, err);
}

function stopAndDisconnectFileSource() {
    if (!fileSourceNode) return;

    try {
        fileSourceNode.stop();
    } catch (err) {
        // already stopped, ignore
    }

    try {
        fileSourceNode.disconnect();
    } catch (err) {
        // already disconnected, ignore
    }

    fileSourceNode = null;
}

function disconnectMicSource() {
    if (!micSourceNode) return;

    try {
        micSourceNode.disconnect();
    } catch (err) {
        // already disconnected, ignore
    }
}

function disconnectAllSources() {
    stopAndDisconnectFileSource();
    disconnectMicSource();
}

async function ensureMicSource() {
    if (!audioCtx || !earthNode) {
        throw new Error('Audio graph is not initialized yet.');
    }

    if (!micStream) {
        setStatus('Requesting microphone access...');
        micStream = await navigator.mediaDevices.getUserMedia({
            audio: {
                echoCancellation: false,
                autoGainControl: false,
                noiseSuppression: false,
                latency: 0
            }
        });
    }

    if (!micSourceNode) {
        micSourceNode = audioCtx.createMediaStreamSource(micStream);
    }

    disconnectAllSources();
    micSourceNode.connect(earthNode);
    setStatus('Mic connected.');
}

function connectFileMode() {
    disconnectAllSources();
    filePlayerControls.style.display = 'block';
    setStatus(audioBuffer ? 'File loaded.' : 'File player active. Upload and play a file.');
}

async function switchSource(mode) {
    if (mode === 'mic') {
        filePlayerControls.style.display = 'none';

        if (!audioCtx || !earthNode) {
            setStatus('Press Start Audio to initialize first.');
            return;
        }

        await ensureMicSource();
        return;
    }

    connectFileMode();
}

// Handle Source Switching
audioSourceSelect.addEventListener('change', async (e) => {
    try {
        await switchSource(e.target.value);
    } catch (err) {
        logError('source switch failed', err);
        setStatus(`Source switch error: ${err.message}`);
    }
});

// Handle File Upload
audioFileInput.addEventListener('change', async (e) => {
    const file = e.target.files[0];
    if (!file) return;

    if (!audioCtx) {
        setStatus('Please Start Audio first.');
        return;
    }

    try {
        setStatus('Loading file...');
        const arrayBuffer = await file.arrayBuffer();
        setStatus('Decoding audio...');
        audioBuffer = await audioCtx.decodeAudioData(arrayBuffer);
        setStatus('File loaded.');
    } catch (err) {
        logError('error decoding audio file', err);
        setStatus(`Error decoding file: ${err.message}`);
    }
});

function playFile() {
    if (!audioCtx || !audioBuffer || !earthNode) return;

    disconnectAllSources();

    fileSourceNode = audioCtx.createBufferSource();
    fileSourceNode.buffer = audioBuffer;
    fileSourceNode.loop = true;
    fileSourceNode.connect(earthNode);
    fileSourceNode.start();

    setStatus('Playing file...');
}

playFileBtn.addEventListener('click', () => {
    if (audioSourceSelect.value !== 'file') {
        setStatus('Switch source to File Player first.');
        return;
    }

    if (!audioBuffer) {
        setStatus('Load a file before pressing play.');
        return;
    }

    playFile();
});

stopFileBtn.addEventListener('click', () => {
    stopAndDisconnectFileSource();
    setStatus('Playback stopped.');
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
        // Debug summary: initialize graph first (AudioContext -> worklet module -> wasm fetch -> node init),
        // then request microphone only if/when the active source is "mic" so file mode never blocks on mic permission.
        setStatus('Initializing AudioContext...');
        audioCtx = new (window.AudioContext || window.webkitAudioContext)({
            latencyHint: 'interactive'
        });

        setStatus('Loading worklet...');
        await audioCtx.audioWorklet.addModule(WORKLET_URL);

        setStatus('Fetching wasm...');
        const response = await fetch(WASM_URL);
        if (!response.ok) {
            throw new Error(`Failed to fetch wasm (${response.status} ${response.statusText}) from ${response.url || WASM_URL}`);
        }

        const wasmBytes = await response.arrayBuffer();
        setStatus('Wasm fetched.');

        earthNode = new AudioWorkletNode(audioCtx, 'earth-worklet-processor', {
            outputChannelCount: [2]
        });

        earthNode.port.onmessage = async (event) => {
            if (event.data.type === 'ready') {
                setStatus('Worklet ready.');
                if (!uiBindingsInitialized) {
                    setupUIBindings();
                    uiBindingsInitialized = true;
                }

                startButton.classList.add('active');
                startButton.innerHTML = 'Power<br>ON';

                try {
                    await switchSource(audioSourceSelect.value);
                } catch (err) {
                    logError('source activation failed after worklet ready', err);
                    setStatus(`Ready, but source activation failed: ${err.message}`);
                }
                return;
            }

            if (event.data.type === 'error') {
                const details = `[${event.data.stage || 'worklet'}] ${event.data.message || 'Unknown worklet error'}`;
                console.error('[EarthPedal] Worklet reported error', event.data);
                setStatus(`Worklet error: ${details}`);
            }
        };

        earthNode.connect(audioCtx.destination);

        earthNode.port.postMessage({
            type: 'init',
            wasmBytes
        });

        setStatus('Waiting for worklet ready...');
    } catch (err) {
        logError('audio initialization failed', err);
        setStatus(`Error: ${err.message}`);
        throw err;
    }
}

function setupUIBindings() {
    paramMappings.forEach(mapping => {
        const el = document.getElementById(mapping.id);
        const valDisplay = document.getElementById(`${mapping.id}-val`);

        if (!el) return;

        updateNodeParam(mapping.id, el.value);

        el.addEventListener('input', (e) => {
            const val = e.target.value;
            updateNodeParam(mapping.id, val);

            if (!mapping.isSwitch && valDisplay) {
                const numVal = parseFloat(val);
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
        try {
            await initAudio();
        } catch (err) {
            // initAudio already reports the error in status/console
        }
        return;
    }

    if (audioCtx.state === 'suspended') {
        await audioCtx.resume();
        startButton.classList.add('active');
        startButton.innerHTML = 'Power<br>ON';
        setStatus('Audio resumed.');
        return;
    }

    if (audioCtx.state === 'running') {
        await audioCtx.suspend();
        startButton.classList.remove('active');
        startButton.innerHTML = 'Start<br>Audio';
        setStatus('Audio paused.');
    }
});
