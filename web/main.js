let audioCtx;
let earthNode;
let micSourceNode;
let micStream;
let audioBuffer;
let fileSourceNode;
let uiBindingsInitialized = false;

// UI Elements - Engine
const startButton = document.getElementById('start-audio');
const statusText = document.getElementById('status-text');
const statusLed = document.getElementById('status-led');
const controlsOverlay = document.getElementById('controls-overlay');
const routingOverlay = document.getElementById('routing-overlay');

// UI Elements - Routing
const audioSourceSelect = document.getElementById('audioSource');
const sourceBtns = document.querySelectorAll('.source-btn');

// UI Elements - File Player
const filePlayerControls = document.getElementById('file-player-controls');
const audioFileInput = document.getElementById('audioFile');
const fileNameDisplay = document.getElementById('file-name-display');
const playFileBtn = document.getElementById('playFileBtn');
const pauseFileBtn = document.getElementById('pauseFileBtn');
const stopFileBtn = document.getElementById('stopFileBtn');
const progressBg = document.getElementById('progress-bg');
const progressFill = document.getElementById('progress-fill');
const timeCurrent = document.getElementById('time-current');
const timeTotal = document.getElementById('time-total');

// File Player State
let isPlaying = false;
let isPaused = false;
let startTime = 0;
let pauseTimeOffset = 0;
let progressRAF = null;

const WORKLET_URL = new URL('./earth-worklet-processor.js', import.meta.url).href;
const WASM_URL = new URL('./earth-module.wasm', import.meta.url).href;

function setStatus(message, state = 'info') {
    statusText.textContent = message;

    // Clear led classes
    statusLed.className = 'status-led';
    if (state === 'ready') statusLed.classList.add('ready');
    if (state === 'running') statusLed.classList.add('running');
    if (state === 'error') statusLed.classList.add('error');
    if (state === 'initializing') statusLed.classList.add('initializing');
}

function updateEngineStateUI(isActive) {
    if (isActive) {
        controlsOverlay.classList.add('hidden');
        routingOverlay.classList.add('hidden');
        startButton.classList.add('active');
        startButton.textContent = 'ON';
    } else {
        controlsOverlay.classList.remove('hidden');
        routingOverlay.classList.remove('hidden');
        startButton.classList.remove('active');
        startButton.textContent = 'PWR';
    }
}

function logError(context, err) {
    console.error(`[EarthPedal] ${context}`, err);
    setStatus(`Error: ${err.message}`, 'error');
}

function formatTime(seconds) {
    if (!seconds || isNaN(seconds)) return '00:00';
    const m = Math.floor(seconds / 60);
    const s = Math.floor(seconds % 60);
    return `${m.toString().padStart(2, '0')}:${s.toString().padStart(2, '0')}`;
}

// --- FILE PLAYER ---

function resetTransport() {
    isPlaying = false;
    isPaused = false;
    startTime = 0;
    pauseTimeOffset = 0;

    if (progressRAF) {
        cancelAnimationFrame(progressRAF);
        progressRAF = null;
    }

    progressFill.style.width = '0%';
    timeCurrent.textContent = '00:00';

    playFileBtn.classList.remove('active');
    pauseFileBtn.classList.remove('active');

    updateTransportButtons();
}

function updateTransportButtons() {
    const hasBuffer = !!audioBuffer;
    playFileBtn.disabled = !hasBuffer;
    pauseFileBtn.disabled = !hasBuffer || (!isPlaying && !isPaused);
    stopFileBtn.disabled = !hasBuffer || (!isPlaying && !isPaused);
}

function updateProgress() {
    if (!isPlaying || !audioBuffer || !audioCtx) return;

    const elapsed = audioCtx.currentTime - startTime + pauseTimeOffset;
    const duration = audioBuffer.duration;

    // Handle loop
    const currentPosition = elapsed % duration;

    const percent = (currentPosition / duration) * 100;
    progressFill.style.width = `${percent}%`;
    timeCurrent.textContent = formatTime(currentPosition);

    progressRAF = requestAnimationFrame(updateProgress);
}

function stopAndDisconnectFileSource() {
    if (!fileSourceNode) return;

    try {
        fileSourceNode.stop();
    } catch (err) { }

    try {
        fileSourceNode.disconnect();
    } catch (err) { }

    fileSourceNode = null;
}

function playFile() {
    if (!audioCtx || !audioBuffer || !earthNode) return;

    disconnectAllSources();

    fileSourceNode = audioCtx.createBufferSource();
    fileSourceNode.buffer = audioBuffer;
    fileSourceNode.loop = true;
    fileSourceNode.connect(earthNode);

    // Start playback taking into account any pause offset
    const offset = pauseTimeOffset % audioBuffer.duration;
    fileSourceNode.start(0, offset);

    startTime = audioCtx.currentTime;
    isPlaying = true;
    isPaused = false;

    playFileBtn.classList.add('active');
    pauseFileBtn.classList.remove('active');

    updateTransportButtons();
    setStatus('Telemetry playback active', 'running');

    if (progressRAF) cancelAnimationFrame(progressRAF);
    progressRAF = requestAnimationFrame(updateProgress);
}

function pauseFile() {
    if (!isPlaying) return;

    pauseTimeOffset += (audioCtx.currentTime - startTime);
    stopAndDisconnectFileSource();

    isPlaying = false;
    isPaused = true;

    if (progressRAF) {
        cancelAnimationFrame(progressRAF);
        progressRAF = null;
    }

    playFileBtn.classList.remove('active');
    pauseFileBtn.classList.add('active');

    updateTransportButtons();
    setStatus('Playback paused', 'ready');
}

function stopFile() {
    stopAndDisconnectFileSource();
    resetTransport();
    setStatus(audioBuffer ? 'Telemetry loaded' : 'Awaiting data', 'ready');
}

// Transport Event Listeners
playFileBtn.addEventListener('click', () => {
    if (!audioBuffer) return;
    if (isPlaying) return;
    playFile();
});

pauseFileBtn.addEventListener('click', () => {
    if (isPlaying) {
        pauseFile();
    } else if (isPaused) {
        playFile();
    }
});

stopFileBtn.addEventListener('click', stopFile);

// Handle progress bar clicking to seek
progressBg.addEventListener('click', (e) => {
    if (!audioBuffer) return;

    const rect = progressBg.getBoundingClientRect();
    const percent = (e.clientX - rect.left) / rect.width;
    const targetTime = percent * audioBuffer.duration;

    pauseTimeOffset = targetTime;

    if (isPlaying) {
        // Restart playing from new position
        stopAndDisconnectFileSource();
        fileSourceNode = audioCtx.createBufferSource();
        fileSourceNode.buffer = audioBuffer;
        fileSourceNode.loop = true;
        fileSourceNode.connect(earthNode);
        fileSourceNode.start(0, targetTime);
        startTime = audioCtx.currentTime;
    } else {
        // Just update UI if not playing
        progressFill.style.width = `${percent * 100}%`;
        timeCurrent.textContent = formatTime(targetTime);
    }
});

// --- AUDIO SOURCES ---

function disconnectMicSource() {
    if (!micSourceNode) return;
    try { micSourceNode.disconnect(); } catch (err) { }
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
        setStatus('Acquiring mic signal...', 'initializing');
        try {
            micStream = await navigator.mediaDevices.getUserMedia({
                audio: {
                    echoCancellation: false,
                    autoGainControl: false,
                    noiseSuppression: false,
                    latency: 0
                }
            });
        } catch (err) {
            setStatus('Mic access denied', 'error');
            throw err;
        }
    }

    if (!micSourceNode) {
        micSourceNode = audioCtx.createMediaStreamSource(micStream);
    }

    disconnectAllSources();
    micSourceNode.connect(earthNode);
    setStatus('Mic active', 'running');
}

function connectFileMode() {
    disconnectAllSources();
    filePlayerControls.classList.add('active');

    if (audioBuffer) {
        setStatus('Telemetry loaded', 'ready');
        updateTransportButtons();
    } else {
        setStatus('Awaiting data file', 'ready');
    }
}

async function switchSource(mode) {
    if (mode === 'mic') {
        filePlayerControls.classList.remove('active');
        if (isPlaying || isPaused) stopFile();

        if (!audioCtx || !earthNode) return;

        await ensureMicSource();
        return;
    }

    if (mode === 'file') {
        connectFileMode();
    }
}

// Handle Source Switching (UI buttons)
sourceBtns.forEach(btn => {
    btn.addEventListener('click', async (e) => {
        // UI update
        sourceBtns.forEach(b => b.classList.remove('active'));
        e.target.classList.add('active');

        const mode = e.target.dataset.source;
        audioSourceSelect.value = mode;

        if (!audioCtx) {
            if (mode === 'file') {
                filePlayerControls.classList.add('active');
            } else {
                filePlayerControls.classList.remove('active');
            }
            return;
        }

        try {
            await switchSource(mode);
        } catch (err) {
            logError('source switch failed', err);
        }
    });
});

// Handle Source Switching (Hidden Select - fallback)
audioSourceSelect.addEventListener('change', async (e) => {
    sourceBtns.forEach(btn => {
        if (btn.dataset.source === e.target.value) {
            btn.classList.add('active');
        } else {
            btn.classList.remove('active');
        }
    });

    if (!audioCtx) return;
    try {
        await switchSource(e.target.value);
    } catch (err) {
        logError('source switch failed', err);
    }
});

// Handle File Upload
audioFileInput.addEventListener('change', async (e) => {
    const file = e.target.files[0];
    if (!file) return;

    if (!audioCtx) {
        alert('Please turn the engine ON first.');
        audioFileInput.value = '';
        return;
    }

    fileNameDisplay.textContent = file.name;

    try {
        setStatus('Loading data...', 'initializing');
        const arrayBuffer = await file.arrayBuffer();
        setStatus('Decoding stream...', 'initializing');
        audioBuffer = await audioCtx.decodeAudioData(arrayBuffer);

        timeTotal.textContent = formatTime(audioBuffer.duration);
        stopFile(); // ensures previous playback stops
        setStatus('Telemetry loaded', 'ready');

    } catch (err) {
        logError('error decoding audio file', err);
        setStatus(`Decode error`, 'error');
        fileNameDisplay.textContent = 'Load Audio Data...';
        audioBuffer = null;
        updateTransportButtons();
    }
});

// --- ENGINE INIT & BINDINGS ---

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
        setStatus('Booting...', 'initializing');
        audioCtx = new (window.AudioContext || window.webkitAudioContext)({
            latencyHint: 'interactive'
        });

        setStatus('Loading kernel...', 'initializing');
        await audioCtx.audioWorklet.addModule(WORKLET_URL);

        setStatus('Fetching DSP core...', 'initializing');
        const response = await fetch(WASM_URL);
        if (!response.ok) {
            throw new Error(`Failed to fetch wasm (${response.status})`);
        }

        const wasmBytes = await response.arrayBuffer();

        earthNode = new AudioWorkletNode(audioCtx, 'earth-worklet-processor', {
            outputChannelCount: [2]
        });

        earthNode.port.onmessage = async (event) => {
            if (event.data.type === 'ready') {
                if (!uiBindingsInitialized) {
                    setupUIBindings();
                    uiBindingsInitialized = true;
                }

                updateEngineStateUI(true);

                try {
                    await switchSource(audioSourceSelect.value);
                } catch (err) {
                    logError('source activation failed after worklet ready', err);
                }
                return;
            }

            if (event.data.type === 'error') {
                const details = `[${event.data.stage || 'worklet'}] ${event.data.message || 'Unknown worklet error'}`;
                console.error('[EarthPedal] Worklet reported error', event.data);
                setStatus(`DSP Fault`, 'error');
                updateEngineStateUI(false);
            }
        };

        earthNode.connect(audioCtx.destination);

        earthNode.port.postMessage({
            type: 'init',
            wasmBytes
        });

    } catch (err) {
        logError('audio initialization failed', err);
        updateEngineStateUI(false);
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
            // initAudio handles status
        }
        return;
    }

    if (audioCtx.state === 'suspended') {
        await audioCtx.resume();
        updateEngineStateUI(true);
        setStatus('Engine Online', 'ready');
        // re-activate current source if needed
        if (audioSourceSelect.value === 'mic' && !micSourceNode) {
            await switchSource('mic');
        } else if (audioSourceSelect.value === 'mic') {
            setStatus('Mic active', 'running');
        } else if (isPlaying) {
             setStatus('Telemetry playback active', 'running');
        }
        return;
    }

    if (audioCtx.state === 'running') {
        await audioCtx.suspend();
        updateEngineStateUI(false);
        setStatus('Engine Suspended', 'ready');
    }
});
