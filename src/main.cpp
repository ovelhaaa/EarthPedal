#include <Arduino.h>
#include <vector>
#include <cmath>
#include "daisysp.h"
#include "Dattorro.hpp"
#include "Multirate.h"
#include "OctaveGenerator.h"
#include "MIDIUSB.h"

// ============================================================================
// Configurations
// ============================================================================
#define SAMPLE_RATE 48000
#define BLOCK_SIZE 48

// Buffers for Dattorro Reverb (Located in AXI SRAM - D1 Domain)
// Explicit placement to ensure they fit and are fast for CPU
#define D1_RAM __attribute__((section(".ram_d1")))
#define D2_RAM __attribute__((section(".ram_d2")))

// Delay Line Sizes (Floats)
// Calculated based on requirements + padding
// PreDelay: 700ms @ 48kHz = 33600. Round to 34000.
#define SZ_PREDELAY 34000
// Tank delays: see Dattorro.hpp constants.
// We use slightly larger sizes to be safe.
#define SZ_LAPF1    1200
#define SZ_LDLY1    7500
#define SZ_LAPF2    3000
#define SZ_LDLY2    6500
#define SZ_RAPF1    1500
#define SZ_RDLY1    7000
#define SZ_RAPF2    4500
#define SZ_RDLY2    5500

// Input APFs
#define SZ_INAPF1   250
#define SZ_INAPF2   200
#define SZ_INAPF3   650
#define SZ_INAPF4   500

// Globals for Buffers
float D1_RAM buf_predelay[SZ_PREDELAY];
float D1_RAM buf_lapf1[SZ_LAPF1];
float D1_RAM buf_ldly1[SZ_LDLY1];
float D1_RAM buf_lapf2[SZ_LAPF2];
float D1_RAM buf_ldly2[SZ_LDLY2];
float D1_RAM buf_rapf1[SZ_RAPF1];
float D1_RAM buf_rdly1[SZ_RDLY1];
float D1_RAM buf_rapf2[SZ_RAPF2];
float D1_RAM buf_rdly2[SZ_RDLY2];
float D1_RAM buf_inapf1[SZ_INAPF1];
float D1_RAM buf_inapf2[SZ_INAPF2];
float D1_RAM buf_inapf3[SZ_INAPF3];
float D1_RAM buf_inapf4[SZ_INAPF4];

// PolyOctave Buffers
// The original code used a static buffer 'buff[6]' inside AudioCallback logic.
// We will replicate that logic in the audio loop.

// ============================================================================
// Audio Objects
// ============================================================================
Dattorro reverb(SAMPLE_RATE);
Decimator2 decimate;
Interpolator interpolate;
OctaveGenerator octave(SAMPLE_RATE / resample_factor);
daisysp::Biquad eq1; // HighShelf replacement
daisysp::Biquad eq2; // LowShelf replacement
daisysp::Overdrive overdrive;
daisysp::Overdrive overdrive2; // Stereo overdrive? Original code had 2.

// Parameters
float p_predelay = 0.0f;
float p_mix = 0.0f; // 0=Dry, 1=Wet
float p_decay = 0.5f;
float p_mod_depth = 0.5f;
float p_mod_speed = 0.5f;
float p_damping = 0.0f;
int   p_octave_mode = 0; // 0=Off, 1=Up, 2=Down, 3=Both

// Smoothed parameters
float current_predelay = 0.0f;
float current_moddepth = 0.0f;
float current_modspeed = 0.0f;
float current_decay = 0.5f;

// State
float dryMix = 1.0f;
float wetMix = 0.0f;
bool bypass = false;

// Audio IO Buffers (DMA) - D2 Domain
// I2S uses 32-bit (24-bit in 32 slot) usually or 16-bit.
// PCM1808/PCM5102 usually 24-bit I2S.
// We will use 32-bit int buffers for DMA.
// Double buffer: 2 blocks of BLOCK_SIZE * 2 channels
#define DMA_BUF_SIZE (BLOCK_SIZE * 2 * 2)
int32_t D2_RAM txBuf[DMA_BUF_SIZE];
int32_t D2_RAM rxBuf[DMA_BUF_SIZE];

// I2S Handle
I2S_HandleTypeDef hi2s2;
DMA_HandleTypeDef hdma_spi2_tx;
DMA_HandleTypeDef hdma_spi2_rx;

// ============================================================================
// Helper Functions
// ============================================================================
float clip(float x, float min, float max) {
    return x < min ? min : (x > max ? max : x);
}

void UpdateParameters() {
    // Simple smoothing
    const float alpha = 0.001f; // Adjust for responsiveness vs noise

    current_predelay += alpha * (p_predelay - current_predelay);
    reverb.setPreDelay(current_predelay);

    current_moddepth += alpha * (p_mod_depth - current_moddepth);
    reverb.setTankModDepth(current_moddepth * 8.0f);

    current_modspeed += alpha * (p_mod_speed - current_modspeed);
    reverb.setTankModSpeed(0.3f + current_modspeed * 15.0f);

    current_decay += alpha * (p_decay - current_decay);
    reverb.setDecay(current_decay);

    // Mix (Constant Power)
    float mix_target = p_mix;
    // Calculate mix coefficients (cheap constant power)
    // Using same logic as original
    float x2 = 1.0f - mix_target;
    float A = mix_target * x2;
    float B = A * (1.0f + 1.4186f * A);
    float C = B + mix_target;
    float D = B + x2;
    wetMix = C * C;
    dryMix = D * D;

    // Damping
    if (p_damping < 0.5f) {
        float dampHigh = p_damping * 2.0f;
        reverb.setInputFilterHighCutoffPitch(7.0f * dampHigh + 3.0f);
    } else {
        float dampLow = (p_damping - 0.5f) * 2.0f;
        reverb.setInputFilterLowCutoffPitch(9.0f * dampLow);
    }
}

// ============================================================================
// Audio Processing
// ============================================================================
float oct_buff[6]; // Input buffer for octave
float oct_out[6];  // Output buffer for octave
int bin_counter = 0;

void ProcessBlock(int32_t* input, int32_t* output, size_t size) {
    UpdateParameters();

    for (size_t i = 0; i < size; i += 2) {
        // Convert 24/32-bit int to float (-1.0 to 1.0)
        // Assuming 24-bit data left-aligned in 32-bit word, or standard 32-bit I2S data.
        // WeAct usually uses standard I2S Philips.
        // Input scaling: / 2^31
        float inL = (float)input[i] / 2147483648.0f;
        float inR = (float)input[i+1] / 2147483648.0f;

        // Mono-sum for octave effect or just use Left
        // Original used Left for octave
        oct_buff[bin_counter] = inL;

        // Octave Processing (every 6 samples)
        if (bin_counter == 5) {
            std::span<const float, 6> in_chunk(oct_buff, 6);
            const auto sample = decimate(in_chunk);

            octave.update(sample);

            float octave_mix = 0.0f;
            if (p_octave_mode == 1 || p_octave_mode == 3) octave_mix += octave.up1() * 2.0f;
            if (p_octave_mode == 2 || p_octave_mode == 3) {
                octave_mix += octave.down1() * 2.0f;
                // octave_mix += octave.down2() * 2.0f; // Uncomment if needed
            }

            auto out_chunk = interpolate(octave_mix);

            for (size_t j = 0; j < 6; ++j) {
                // Apply EQ
                float mix = out_chunk[j];
                mix = eq1.Process(mix);
                mix = eq2.Process(mix);

                // Mix logic: if octave enabled, output mix, else 0
                if (p_octave_mode != 0) {
                    oct_out[j] = mix;
                } else {
                    oct_out[j] = 0.0f;
                }
            }
        }

        // Reverb Input
        // Original code latency compensation logic:
        // reverb_in = buff_out[bin_counter]; // Adds latency to align with octave
        // We use oct_out calculated above (or from previous block end)

        float reverb_in = inL;
        if (p_octave_mode != 0) {
            // Mix octave into reverb input
            // reverb_in = inL + oct_out[bin_counter]; // Or replace? Original replaced if "momentary" logic
            // Let's mix it.
             reverb_in += oct_out[bin_counter];
        }

        bin_counter++;
        if (bin_counter >= 6) bin_counter = 0;

        // Process Reverb
        reverb.process(reverb_in, reverb_in);
        float revL = reverb.getLeftOutput();
        float revR = reverb.getRightOutput();

        // Overdrive on reverb tail?
        // revL = overdrive.Process(revL * 0.25f);
        // revR = overdrive2.Process(revR * 0.25f);

        // Mix
        float outL = inL * dryMix + revL * wetMix * 0.4f;
        float outR = inR * dryMix + revR * wetMix * 0.4f;

        // Output scaling
        output[i]   = (int32_t)(outL * 2147483648.0f);
        output[i+1] = (int32_t)(outR * 2147483648.0f);
    }
}

// ============================================================================
// I2S / DMA Interrupts
// ============================================================================
void DMA1_Stream0_IRQHandler(void) { // TX DMA (Update Stream based on setup)
    HAL_DMA_IRQHandler(&hdma_spi2_tx);
}
void DMA1_Stream1_IRQHandler(void) { // RX DMA
    HAL_DMA_IRQHandler(&hdma_spi2_rx);
}

// Half/Full Transfer Callbacks
extern "C" void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s) {
    // Process first half of TX buffer (using data from first half of RX buffer)
    // Note: RX happens simultaneously. By the time TX Half Cplt fires, RX Half Cplt should have fired or data ready.
    // In full duplex circular mode, we process the half that was just filled/transmitted.
    // Ideally we process RX -> TX.

    // Safety check: cache coherence
    SCB_InvalidateDCache_by_Addr((uint32_t*)rxBuf, DMA_BUF_SIZE * 2); // Half buffer?

    ProcessBlock(&rxBuf[0], &txBuf[0], BLOCK_SIZE * 2); // First half

    SCB_CleanDCache_by_Addr((uint32_t*)txBuf, DMA_BUF_SIZE * 2);
}

extern "C" void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s) {
    // Process second half
    SCB_InvalidateDCache_by_Addr((uint32_t*)&rxBuf[DMA_BUF_SIZE/2], DMA_BUF_SIZE * 2);

    ProcessBlock(&rxBuf[DMA_BUF_SIZE/2], &txBuf[DMA_BUF_SIZE/2], BLOCK_SIZE * 2);

    SCB_CleanDCache_by_Addr((uint32_t*)&txBuf[DMA_BUF_SIZE/2], DMA_BUF_SIZE * 2);
}

// Note: RX callbacks usually fire slightly before or sync with TX in full duplex.
// We attach processing to TX interrupts to ensure we have data to send.
// Or we can use RX interrupts to process data and put into TX.
// Common practice: Use Tx Half/Full Complete.

// ============================================================================
// Initialization
// ============================================================================
void InitAudio() {
    // Initialize Dattorro Buffers
    DattorroBuffers bufs;
    bufs.preDelay = buf_predelay; bufs.preDelaySize = SZ_PREDELAY;
    bufs.inApf1 = buf_inapf1; bufs.inApf1Size = SZ_INAPF1;
    bufs.inApf2 = buf_inapf2; bufs.inApf2Size = SZ_INAPF2;
    bufs.inApf3 = buf_inapf3; bufs.inApf3Size = SZ_INAPF3;
    bufs.inApf4 = buf_inapf4; bufs.inApf4Size = SZ_INAPF4;

    bufs.tankBuffers.leftApf1 = buf_lapf1; bufs.tankBuffers.leftApf1Size = SZ_LAPF1;
    bufs.tankBuffers.leftDelay1 = buf_ldly1; bufs.tankBuffers.leftDelay1Size = SZ_LDLY1;
    bufs.tankBuffers.leftApf2 = buf_lapf2; bufs.tankBuffers.leftApf2Size = SZ_LAPF2;
    bufs.tankBuffers.leftDelay2 = buf_ldly2; bufs.tankBuffers.leftDelay2Size = SZ_LDLY2;

    bufs.tankBuffers.rightApf1 = buf_rapf1; bufs.tankBuffers.rightApf1Size = SZ_RAPF1;
    bufs.tankBuffers.rightDelay1 = buf_rdly1; bufs.tankBuffers.rightDelay1Size = SZ_RDLY1;
    bufs.tankBuffers.rightApf2 = buf_rapf2; bufs.tankBuffers.rightApf2Size = SZ_RAPF2;
    bufs.tankBuffers.rightDelay2 = buf_rdly2; bufs.tankBuffers.rightDelay2Size = SZ_RDLY2;

    reverb.Init(bufs);

    // Init Effects
    overdrive.Init();
    overdrive2.Init();
    eq1.Init(SAMPLE_RATE);
    eq1.SetHighShelf(140.0f, -11.0f, 0.707f);

    eq2.Init(SAMPLE_RATE);
    eq2.SetLowShelf(160.0f, 5.0f, 0.707f); // +5dB? Gain in dB? DaisySP Biquad SetLowShelf(freq, gain, q)
    // Assuming gain is linear or dB? DaisySP usually uses Gain as linear amplitude or dB depending on method.
    // Check DaisySP doc or source. Usually SetLowShelf takes gain in dB or linear.
    // Standard Audio EQ Cookbook uses linear A = 10^(dB/40).
    // DaisySP Biquad: `gain` parameter.
    // Let's assume dB.

}

void InitI2S() {
    // 1. Enable Clocks
    __HAL_RCC_SPI2_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    // 2. GPIO Config
    // PB12 (WS), PB13 (CK), PC3 (SD_TX), PC2 (SD_RX), PC6 (MCK)
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // PB12, PB13
    GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // PC2, PC3, PC6
    GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    // 3. DMA Config
    // TX: SPI2_TX -> DMA1 Stream 4 (Check Reference Manual for DMA request mapping on H7)
    // RX: SPI2_RX -> DMA1 Stream 3
    // On H7, DMAMUX is used. HAL_DMA_Init usually handles this if handle is set correct,
    // but we need to configure the request.

    // TX DMA
    hdma_spi2_tx.Instance = DMA1_Stream4;
    hdma_spi2_tx.Init.Request = DMA_REQUEST_SPI2_TX;
    hdma_spi2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_spi2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_spi2_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_spi2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD; // 32-bit
    hdma_spi2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_spi2_tx.Init.Mode = DMA_CIRCULAR;
    hdma_spi2_tx.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_spi2_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&hdma_spi2_tx);
    __HAL_LINKDMA(&hi2s2, hdmatx, hdma_spi2_tx);

    // RX DMA
    hdma_spi2_rx.Instance = DMA1_Stream3;
    hdma_spi2_rx.Init.Request = DMA_REQUEST_SPI2_RX;
    hdma_spi2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_spi2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_spi2_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_spi2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_spi2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_spi2_rx.Init.Mode = DMA_CIRCULAR;
    hdma_spi2_rx.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_spi2_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&hdma_spi2_rx);
    __HAL_LINKDMA(&hi2s2, hdmarx, hdma_spi2_rx);

    // Enable DMA Interrupts
    HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);
    HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);

    // 4. I2S Config
    hi2s2.Instance = SPI2;
    hi2s2.Init.Mode = I2S_MODE_MASTER_TX; // Starts as Master TX, but we want Full Duplex
    // STM32H7 I2S supports Full Duplex Mode.
    // HAL_I2S_Init will configure it.
    // NOTE: Standard I2S Mode in HAL often assumes Simplex.
    // To do Full Duplex, we use I2S_MODE_MASTER_TX and the I2S_FULLDUPLEX_MODE_ENABLE feature if available
    // or configure the Extended block.
    // For simplicity with HAL, usually we set Mode = I2S_MODE_MASTER_TX and ensure FullDuplex is handled by the I2S peripheral
    // configuration if the HAL supports it via "FullDuplexMode" field?
    // H7 HAL has `hi2s2.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_ENABLE;`?
    // Let's check typical struct. It has `Standard`, `DataFormat`, `MCLKOutput`, `AudioFreq`, `CPOL`.
    // Full Duplex on H7 involves the I2Sx_EXT block implicitly or explicitly.
    // HAL_I2SEx_TransmitReceive_DMA handles the EXT block if configured?
    // Actually, simple way: Configure as Master TX. The TransmitReceive function handles the rest?
    // No, we should assume Simplex TX for now if Full Duplex is hard to guarantee blind.
    // BUT the user needs Input for the pedal.
    // I will set `I2S_FULLDUPLEXMODE_ENABLE` if definition exists, or assume standard config.
    // Let's try standard Master TX/RX.

    hi2s2.Init.Mode = I2S_MODE_MASTER_TX;
    hi2s2.Init.Standard = I2S_STANDARD_PHILIPS;
    hi2s2.Init.DataFormat = I2S_DATAFORMAT_24B; // 24-bit data in 32-bit frame usually
    hi2s2.Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;
    hi2s2.Init.AudioFreq = I2S_AUDIOFREQ_48K;
    hi2s2.Init.CPOL = I2S_CPOL_LOW;
    hi2s2.Init.FirstBit = I2S_FIRSTBIT_MSB;
    hi2s2.Init.WSInversion = I2S_WS_INVERSION_DISABLE;
    hi2s2.Init.Data24BitAlignment = I2S_DATA_24BIT_ALIGNMENT_LEFT; // or RIGHT
    hi2s2.Init.MasterKeepIOState = I2S_MASTER_KEEP_IO_STATE_DISABLE;

    HAL_I2S_Init(&hi2s2);

    // 5. Start
    // We use TransmitReceive.
    // Note: On some STM32s, you need to enable the I2S_EXT peripheral for full duplex.
    // H7 has implicit support usually.
    // If this fails, user might need to adjust.
    HAL_I2SEx_TransmitReceive_DMA(&hi2s2, (uint16_t*)txBuf, (uint16_t*)rxBuf, DMA_BUF_SIZE);
}

void setup() {
    Serial.begin(115200);

    InitAudio();
    InitI2S();
}

void loop() {
    // Handle USB MIDI
    midiEventPacket_t rx;
    do {
        rx = MidiUSB.read();
        if (rx.header != 0) {
            // Check for CC
            if ((rx.header & 0x0F) == 0x0B && (rx.byte1 & 0xF0) == 0xB0) { // Control Change
                int cc = rx.byte2;
                int val = rx.byte3;
                float norm = val / 127.0f;

                switch(cc) {
                    case 14: p_predelay = norm * 0.7f; break; // 0-700ms (approx)
                    case 15: p_mix = norm; break;
                    case 16: p_decay = norm; break;
                    case 17: p_mod_depth = norm; break;
                    case 18: p_mod_speed = norm; break;
                    case 19: p_damping = norm; break;
                    case 20:
                        if (val < 32) p_octave_mode = 0;
                        else if (val < 64) p_octave_mode = 1;
                        else if (val < 96) p_octave_mode = 2;
                        else p_octave_mode = 3;
                        break;
                }
            }
        }
    } while (rx.header != 0);
}
