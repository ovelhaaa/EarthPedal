import Module from './earth-module.js';

class EarthWorkletProcessor extends AudioWorkletProcessor {
  constructor() {
    super();

    this.module = null;
    this.processor = null;

    // We allocate buffers dynamically once we know the buffer size
    this.inLPtr = null;
    this.inRPtr = null;
    this.outLPtr = null;
    this.outRPtr = null;
    this.bufferSize = 0;

    this.port.onmessage = (event) => {
      if (event.data.type === 'init') {
        const wasmBytes = event.data.wasmBytes;
        Module({
          instantiateWasm: (info, receiveInstance) => {
            WebAssembly.instantiate(wasmBytes, info).then((result) => {
              receiveInstance(result.instance);
            });
            return {};
          }
        }).then((module) => {
          this.module = module;
          // Assume context sample rate is passed or available, here we just use sampleRate globally available in AudioWorkletGlobalScope
          this.processor = new module.EarthAudioProcessor(sampleRate);
          this.port.postMessage({ type: 'ready' });
        });
      }
    };
  }

  static get parameterDescriptors() {
    return [
      { name: 'preDelay', defaultValue: 0.0, minValue: 0.0, maxValue: 1.0 },
      { name: 'mix', defaultValue: 0.5, minValue: 0.0, maxValue: 1.0 },
      { name: 'decay', defaultValue: 0.5, minValue: 0.0, maxValue: 1.0 },
      { name: 'modDepth', defaultValue: 0.5, minValue: 0.0, maxValue: 1.0 },
      { name: 'modSpeed', defaultValue: 0.5, minValue: 0.0, maxValue: 1.0 },
      { name: 'filter', defaultValue: 0.5, minValue: 0.0, maxValue: 1.0 },
      // Switches
      { name: 'reverbSize', defaultValue: 1, minValue: 0, maxValue: 2 }, // 0: Small, 1: Med, 2: Big
      { name: 'octaveMode', defaultValue: 0, minValue: 0, maxValue: 2 }, // 0: None, 1: Up, 2: Up+Down
      { name: 'disableInputDiffusion', defaultValue: 0, minValue: 0, maxValue: 1 }, // 0: false, 1: true
    ];
  }

  allocateBuffers(size) {
    if (this.bufferSize !== size && this.module) {
      if (this.inLPtr) {
        this.module._free(this.inLPtr);
        this.module._free(this.inRPtr);
        this.module._free(this.outLPtr);
        this.module._free(this.outRPtr);
      }
      this.bufferSize = size;
      const bytes = size * 4; // 4 bytes per float32
      this.inLPtr = this.module._malloc(bytes);
      this.inRPtr = this.module._malloc(bytes);
      this.outLPtr = this.module._malloc(bytes);
      this.outRPtr = this.module._malloc(bytes);
    }
  }

  process(inputs, outputs, parameters) {
    if (!this.processor || !this.module) {
      return true; // Keep alive until WASM is ready
    }

    const input = inputs[0];
    const output = outputs[0];

    if (input.length === 0 || output.length === 0) {
      return true; // No connected inputs/outputs
    }

    const size = input[0].length;
    this.allocateBuffers(size);

    // Update parameters
    const getParam = (name) => {
        const param = parameters[name];
        return param.length > 1 ? param[0] : param[0]; // Take first value of block
    };

    this.processor.setPreDelay(getParam('preDelay'));
    this.processor.setMix(getParam('mix'));
    this.processor.setDecay(getParam('decay'));
    this.processor.setModDepth(getParam('modDepth'));
    this.processor.setModSpeed(getParam('modSpeed'));
    this.processor.setFilter(getParam('filter'));
    this.processor.setReverbSize(Math.round(getParam('reverbSize')));
    this.processor.setOctaveMode(Math.round(getParam('octaveMode')));
    this.processor.setDisableInputDiffusion(getParam('disableInputDiffusion') > 0.5);

    // Get Emscripten memory views
    const memView = new Float32Array(this.module.HEAPF32.buffer);

    // Write input buffers to WASM memory
    const inLArray = input[0];
    const inRArray = input.length > 1 ? input[1] : input[0]; // Handle mono input by duplicating to Right

    memView.set(inLArray, this.inLPtr >> 2);
    memView.set(inRArray, this.inRPtr >> 2);

    // Run processing
    this.processor.process(this.inLPtr, this.inRPtr, this.outLPtr, this.outRPtr, size);

    // Read output buffers from WASM memory
    const outLArray = memView.subarray(this.outLPtr >> 2, (this.outLPtr >> 2) + size);
    const outRArray = memView.subarray(this.outRPtr >> 2, (this.outRPtr >> 2) + size);

    // Output is usually stereo
    output[0].set(outLArray);
    if (output.length > 1) {
      output[1].set(outRArray);
    }

    return true;
  }
}

registerProcessor('earth-worklet-processor', EarthWorkletProcessor);
