#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <MIDI.h>
#include "MidiCC.h"
#include <math.h>
#include <RoxMux.h>
#include "Constants.h"
#include "AudioPatch.h"
#include "Parameters.h"
#include "PatchMgr.h"
#include "Button.h"
#include "HWControls.h"
#include "EepromMgr.h"

#define PARAMETER 0      //The main page for displaying the current patch and control (parameter) changes
#define RECALL 1         //Patches list
#define SAVE 2           //Save patch page
#define REINITIALISE 3   // Reinitialise message
#define PATCH 4          // Show current patch bypassing PARAMETER
#define PATCHNAMING 5    // Patch naming page
#define DELETE 6         //Delete patch page
#define DELETEMSG 7      //Delete patch message page
#define SETTINGS 8       //Settings page
#define SETTINGSVALUE 9  //Settings page

unsigned int state = PARAMETER;

#include "ST7735Display.h"

bool cardStatus = false;

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

#include "Settings.h"

int patchNo = 1;               //Current patch no
int voiceToReturn = -1;        //Initialise
long earliestTime = millis();  //For voice allocation - initialise to now

// --- put near the top of your .ino (after the GUI code) ---
enum ModSrc : uint8_t { MOD_LFO = 0,
                        MOD_ENV = 1,
                        MOD_INVENV = 2 };

AudioMixer4 *pitchA[9] = { nullptr, &pitchAmtA1, &pitchAmtA2, &pitchAmtA3, &pitchAmtA4, &pitchAmtA5, &pitchAmtA6, &pitchAmtA7, &pitchAmtA8 };
AudioMixer4 *pitchB[9] = { nullptr, &pitchAmtB1, &pitchAmtB2, &pitchAmtB3, &pitchAmtB4, &pitchAmtB5, &pitchAmtB6, &pitchAmtB7, &pitchAmtB8 };
AudioMixer4 *pitchC[9] = { nullptr, &pitchAmtC1, &pitchAmtC2, &pitchAmtC3, &pitchAmtC4, &pitchAmtC5, &pitchAmtC6, &pitchAmtC7, &pitchAmtC8 };

AudioMixer4 *pwmA[9] = { nullptr, &pwmAmtA1, &pwmAmtA2, &pwmAmtA3, &pwmAmtA4, &pwmAmtA5, &pwmAmtA6, &pwmAmtA7, &pwmAmtA8 };
AudioMixer4 *pwmB[9] = { nullptr, &pwmAmtB1, &pwmAmtB2, &pwmAmtB3, &pwmAmtB4, &pwmAmtB5, &pwmAmtB6, &pwmAmtB7, &pwmAmtB8 };
AudioMixer4 *pwmC[9] = { nullptr, &pwmAmtC1, &pwmAmtC2, &pwmAmtC3, &pwmAmtC4, &pwmAmtC5, &pwmAmtC6, &pwmAmtC7, &pwmAmtC8 };

AudioMixer4 *vMix[9] = { nullptr, &voiceMix1, &voiceMix2, &voiceMix3, &voiceMix4, &voiceMix5, &voiceMix6, &voiceMix7, &voiceMix8 };

AudioSynthWaveformModulated *dcoA[9] = { nullptr, &dco1A, &dco2A, &dco3A, &dco4A, &dco5A, &dco6A, &dco7A, &dco8A };
AudioSynthWaveformModulated *dcoB[9] = { nullptr, &dco1B, &dco2B, &dco3B, &dco4B, &dco5B, &dco6B, &dco7B, &dco8B };
AudioSynthWaveformModulated *dcoC[9] = { nullptr, &dco1C, &dco2C, &dco3C, &dco4C, &dco5C, &dco6C, &dco7C, &dco8C };

// convenience: zero all 4 inputs on a mixer
inline void zero4(AudioMixer4 &m) {
  m.gain(0, 0);
  m.gain(1, 0);
  m.gain(2, 0);
  m.gain(3, 0);
}

// ---------- Helpers ----------
static inline float midiNoteToHz(uint8_t note) {
  // 440 * 2^((n-69)/12)
  return 440.0f * powf(2.0f, (int(note) - 69) / 12.0f);
}
static inline float centsToRatio(float cents) {
  return powf(2.0f, cents / 1200.0f);
}
static inline float velToAmp(uint8_t vel) {  // 1..127 -> 0.0..1.0 curved
  float v = constrain(vel, 1, 127) / 127.0f;
  return powf(v, 0.6f);  // a bit hotter feel
}

static float g_latestEnv[9] = { 0 };     // 1..8 used
static float g_latestAmpEnv[9] = { 0 };  // 1..8 used
static float g_latestLFO = 0.0f;         // -1..+1
static float g_latestLFO_amp = 0.0f;     // -1..+1
volatile float g_latestLFO2 = 0.0f;      // -1..+1
volatile float g_latestLFO2_amp = 0.0f;  // -1..+1

inline float s16_to_float(int16_t s) {
  return s / 32767.0f;
}
inline float clamp01(float x) {
  return x < 0 ? 0 : (x > 1 ? 1 : x);
}
inline float ui255_lin(uint8_t v) {
  return v / 255.0f;
}
inline float bi_to_uni(float x) {
  return 0.5f * x + 0.5f;
}  // -1..+1 -> 0..1

inline float freq_to_midi(float f) {
  return 69.0f + 12.0f * log2f(f / 440.0f);
}
inline float midi_to01(float n) {
  return clamp01(n / 127.0f);
}

// map voice index -> that voice's current note frequency
inline float voiceFreq(int v) {
  switch (v) {
    case 1: return noteFreqs[note1freq];
    case 2: return noteFreqs[note2freq];
    case 3: return noteFreqs[note3freq];
    case 4: return noteFreqs[note4freq];
    case 5: return noteFreqs[note5freq];
    case 6: return noteFreqs[note6freq];
    case 7: return noteFreqs[note7freq];
    default: return noteFreqs[note8freq];
  }
}

// ---------- Pointers to your existing per-voice objects ----------
struct VoiceIO {
  AudioSynthWaveformModulated *A;
  AudioSynthWaveformModulated *B;
  AudioSynthWaveformModulated *C;
  AudioEffectEnvelope *env;        // pitch/PWM env (digital)
  AudioEffectEnvelope *envFilter;  // control tap to filter CV
  AudioEffectEnvelope *envAmp;     // control tap to VCA CV
};

// These MUST match the names from the big graph you pasted earlier
VoiceIO VO[8] = {
  { &dco1A, &dco1B, &dco1C, &env1, &filterEnv1, &ampEnv1 },
  { &dco2A, &dco2B, &dco2C, &env2, &filterEnv2, &ampEnv2 },
  { &dco3A, &dco3B, &dco3C, &env3, &filterEnv3, &ampEnv3 },
  { &dco4A, &dco4B, &dco4C, &env4, &filterEnv4, &ampEnv4 },
  { &dco5A, &dco5B, &dco5C, &env5, &filterEnv5, &ampEnv5 },
  { &dco6A, &dco6B, &dco6C, &env6, &filterEnv6, &ampEnv6 },
  { &dco7A, &dco7B, &dco7C, &env7, &filterEnv7, &ampEnv7 },
  { &dco8A, &dco8B, &dco8C, &env8, &filterEnv8, &ampEnv8 },
};

struct VoiceState {
  int8_t note = -1;  // -1 = free
  uint8_t vel = 0;
  uint32_t age = 0;  // increments each noteOn for simple stealing
} VS[8];

uint32_t noteAgeCounter = 1;
int nextVoiceRR = 0;
uint8_t voiceNote[9];  // per-voice note index (0..127 or into noteFreqs)

#define OCTO_TOTAL 3
#define BTN_DEBOUNCE 50
RoxOctoswitch<OCTO_TOTAL, BTN_DEBOUNCE> octoswitch;

// pins for 74HC165
#define PIN_DATA 33  // pin 9 on 74HC165 (DATA)
#define PIN_LOAD 34  // pin 1 on 74HC165 (LOAD)
#define PIN_CLK 35   // pin 2 on 74HC165 (CLK))

/* ============================================================
   SETUP / RUNTIME
   ============================================================ */

void pollAllMCPs();

void initRotaryEncoders();

//void initButtons();

int getEncoderSpeed(int id);

enum : uint8_t {
  CMD_WRITE_N = 0x1,           // write to input reg n, DO NOT update DAC
  CMD_WRITE_UPDATE_N = 0x3,    // write to input reg n, update that DAC
  CMD_WRITE_UPDATE_ALL = 0x7,  // write to all input regs, update all DACs
};

// ---- Channel IDs (simple integers) ----
enum : uint8_t { DAC_A = 0,
                 DAC_B = 1,
                 DAC_C = 2,
                 DAC_D = 3,
                 DAC_E = 4,
                 DAC_F = 5,
                 DAC_G = 6,
                 DAC_H = 7 };

// TI DAC7568/8168/8568 command codes (upper nibble of first byte)
enum : uint8_t {
  CMD_WRITE_INPUT_N = 0x00,        // write to input register n (no update)
  CMD_UPDATE_DAC_N = 0x10,         // update DAC register n (power up)
  CMD_WRITE_INPUT_ALL_UPD = 0x20,  // write input regs & update all (SW LDAC)
  CMD_POWERDOWN = 0x30,
  CMD_SET_LDAC_MASK = 0x40,
  CMD_RESET = 0x50,
  CMD_SET_INTERNAL_REF = 0x60,
  CMD_NOOP = 0x70
};

void setup() {
  Serial.begin(115200);
  AudioMemory(700);  // plenty for 8 voices + queues

  SPI.begin();
  setupDisplay();
  setUpSettings();
  setupHardware();
  // --- CS42448 init ---
  while (!Serial)
    ;

  if (cs42448.enable() && cs42448.volume(0.7)) {
    Serial.println("configured CS42448");
  } else {
    Serial.println("failed to config CS42448");
  }

  cardStatus = SD.begin(BUILTIN_SDCARD);
  if (cardStatus) {
    Serial.println("SD card is connected");
    //Get patch numbers and names from SD card
    loadPatches();
    if (patches.size() == 0) {
      //save an initialised patch to SD card
      savePatch("1", INITPATCH);
      loadPatches();
    }
  } else {
    Serial.println("SD card is not connected or unusable");

    //reinitialiseToPanel();
    showPatchPage("No SD", "conn'd / usable");
  }

  Wire.begin();
  Wire.setClock(400000);  // Slow down I2C to 100kHz

  mcp1.begin(0);
  delay(10);
  mcp2.begin(1);
  delay(10);
  mcp3.begin(2);
  delay(10);
  mcp4.begin(3);
  delay(10);
  mcp5.begin(4);
  delay(10);
  mcp6.begin(5);
  delay(10);
  mcp7.begin(6);
  delay(10);
  mcp8.begin(7);
  delay(10);

  //groupEncoders();
  initRotaryEncoders();
  //initButtons();

  mcp1.pinMode(7, OUTPUT);   // pin 7 = GPA7 of MCP2301X
  mcp1.pinMode(15, OUTPUT);  // pin 15 = GPB7 of MCP2301X

  mcp2.pinMode(7, OUTPUT);   // pin 7 = GPA7 of MCP2301X
  mcp2.pinMode(15, OUTPUT);  // pin 15 = GPB7 of MCP2301X

  mcp3.pinMode(7, OUTPUT);   // pin 7 = GPA7 of MCP2301X
  mcp3.pinMode(15, OUTPUT);  // pin 15 = GPB7 of MCP2301X

  mcp4.pinMode(6, OUTPUT);   // pin 6 = GPA6 of MCP2301X
  mcp4.pinMode(7, OUTPUT);   // pin 7 = GPA7 of MCP2301X
  mcp4.pinMode(14, OUTPUT);  // pin 14 = GPB6 of MCP2301X
  mcp4.pinMode(15, OUTPUT);  // pin 15 = GPB7 of MCP2301X

  mcp5.pinMode(6, OUTPUT);   // pin 6 = GPA6 of MCP2301X
  mcp5.pinMode(7, OUTPUT);   // pin 7 = GPA7 of MCP2301X
  mcp5.pinMode(14, OUTPUT);  // pin 14 = GPB6 of MCP2301X
  mcp5.pinMode(15, OUTPUT);  // pin 15 = GPB7 of MCP2301X

  mcp6.pinMode(6, OUTPUT);   // pin 6 = GPA6 of MCP2301X
  mcp6.pinMode(7, OUTPUT);   // pin 7 = GPA7 of MCP2301X
  mcp6.pinMode(14, OUTPUT);  // pin 14 = GPB6 of MCP2301X
  mcp6.pinMode(15, OUTPUT);  // pin 15 = GPB7 of MCP2301X

  mcp7.pinMode(6, OUTPUT);   // pin 6 = GPA6 of MCP2301X
  mcp7.pinMode(7, OUTPUT);   // pin 7 = GPA7 of MCP2301X
  mcp7.pinMode(14, OUTPUT);  // pin 14 = GPB6 of MCP2301X
  mcp7.pinMode(15, OUTPUT);  // pin 15 = GPB7 of MCP2301X

  mcp8.pinMode(6, OUTPUT);   // pin 6 = GPA6 of MCP2301X
  mcp8.pinMode(7, OUTPUT);   // pin 7 = GPA7 of MCP2301X
  mcp8.pinMode(14, OUTPUT);  // pin 14 = GPB6 of MCP2301X
  mcp8.pinMode(15, OUTPUT);  // pin 15 = GPB7 of MCP2301X

  // --- LFOs ---
  LFO1.begin(WAVEFORM_SINE);
  LFO1.amplitude(1.0f);
  LFO1.frequency(2.0f);

  LFO2.begin(WAVEFORM_TRIANGLE);
  LFO2.amplitude(1.0f);
  LFO2.frequency(5.0f);

  // Common DC/LFO/noise
  dc_one.amplitude(1.0f);        // gates envs
  dc_neg1.amplitude(-1.0f);      // for *inverting* envelopes
  dc_pwmAbias.amplitude(0.50f);  // center point for PW A (0..1)
  dc_pwmBbias.amplitude(0.50f);  // center point for PW B
  dc_pwmCbias.amplitude(0.50f);  // center point for PW C

  // --- Noise mix (start muted; raise if you want noise in voices) ---
  noiseMix.gain(0, 0.0f);  // white
  noiseMix.gain(1, 0.0f);  // pink
  noiseMix.gain(2, 0.0f);
  noiseMix.gain(3, 0.0f);

  noiseWhite.amplitude(1.0f); 
  noisePink.amplitude(1.0f); 

  // Init all per-voice bits
  for (uint8_t v = 1; v <= 8; ++v) {
    // Voice mixer: A,B,C on ch0..2, noise on ch3
    vMix[v]->gain(0, 0.8f);
    vMix[v]->gain(1, 0.8f);
    vMix[v]->gain(2, 0.8f);
    vMix[v]->gain(3, 0.0f);  // raise if you want per-voice noise

    // Pitch selector mixers: default to LFO, depth 0 (you’ll set via UI)
    zero4(*pitchA[v]);
    zero4(*pitchB[v]);
    zero4(*pitchC[v]);
    pitchA[v]->gain(0, 0.0f);  // LFO1 depth
    pitchB[v]->gain(0, 0.0f);
    pitchC[v]->gain(0, 0.0f);

    // PWM selector mixers: keep DC bias at 1.0 on ch3; others start at 0
    zero4(*pwmA[v]);
    zero4(*pwmB[v]);
    zero4(*pwmC[v]);
    pwmA[v]->gain(3, 1.0f);
    pwmB[v]->gain(3, 1.0f);
    pwmC[v]->gain(3, 1.0f);

    // DCOs: use pulse for PWM input, set base amplitudes/freqs
    dcoA[v]->begin(WAVEFORM_PULSE);
    dcoB[v]->begin(WAVEFORM_PULSE);
    dcoC[v]->begin(WAVEFORM_PULSE);

    dcoA[v]->amplitude(0.8f);
    dcoB[v]->amplitude(0.8f);
    dcoC[v]->amplitude(0.8f);

    // set each voice’s *carrier* frequency elsewhere when you assign notes
    // but a safe boot value helps sanity:
    dcoA[v]->frequency(220.0f);
    dcoB[v]->frequency(220.0f);
    dcoC[v]->frequency(220.0f);
  }

  // --- Start queues you’ll read (IMPORTANT) ---
  qLFO1.begin();
  qLFO1_amp.begin();
  qLFO2.begin();
  qLFO2_amp.begin();
  qFilterEnv1.begin();
  qAmpEnv1.begin();
  qFilterEnv2.begin();
  qAmpEnv2.begin();
  qFilterEnv3.begin();
  qAmpEnv3.begin();
  qFilterEnv4.begin();
  qAmpEnv4.begin();
  qFilterEnv5.begin();
  qAmpEnv5.begin();
  qFilterEnv6.begin();
  qAmpEnv6.begin();
  qFilterEnv7.begin();
  qAmpEnv7.begin();
  qFilterEnv8.begin();
  qAmpEnv8.begin();


  // USB MIDI
  usbMIDI.setHandleNoteOn(myNoteOn);
  usbMIDI.setHandleNoteOff(myNoteOff);

  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.setHandleNoteOn(myNoteOn);
  MIDI.setHandleNoteOff(myNoteOff);

  octoswitch.begin(PIN_DATA, PIN_LOAD, PIN_CLK);
  octoswitch.setCallback(onButtonPress);

  spiSend32(DAC_FILTER, int_ref_on_flexible_mode);
  delayMicroseconds(50);
  spiSend32(DAC_AMP, int_ref_on_flexible_mode);
  delayMicroseconds(50);
  spiSend32(DAC_GLOBAL, int_ref_on_flexible_mode);
  delayMicroseconds(50);  // give the 2.5 V ref a moment to settle

  recallPatch(patchNo);
}



// Build a 32-bit frame: CMD in [31:28], ADDR in [27:24], 12-bit code in [19:8], low 8 zero
static inline uint32_t dac7568_frame(uint8_t cmd, uint8_t addr, uint16_t code12) {
  return ((uint32_t)(cmd & 0x0F) << 24)
         | ((uint32_t)(addr & 0x0F) << 20)
         | ((uint32_t)(code12 & 0x0FFF) << 8);
}

static inline void spiSend32(uint8_t csPin, uint32_t w) {
  SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE1));
  digitalWrite(csPin, LOW);
  SPI.transfer32(w);
  digitalWrite(csPin, HIGH);
  SPI.endTransaction();
}

// ---- Debug print ----
static inline void printBinaryWithLeadingZeros(uint32_t v, uint8_t bits) {
  for (int i = bits - 1; i >= 0; --i) Serial.print((v >> i) & 1);
  Serial.println();
}

// ---- Buffered write ----
static inline void dacWriteBuffered(uint8_t csPin, uint8_t ch, uint16_t code12) {

  // Serial.print("code12: ");
  // Serial.println(code12);

  const uint32_t w = dac7568_frame(0b0010, ch, code12);

  // Serial.print("frame : ");
  // printBinaryWithLeadingZeros(w, 32);

  spiSend32(csPin, w);
}


static inline void ldacStrobe() {
  // Assumes LDAC idles HIGH
  digitalWrite(DAC_LDAC, LOW);
  // A few tens of ns is enough; this is safe on 600 MHz Teensy
  asm volatile("nop; nop; nop; nop;");
  digitalWrite(DAC_LDAC, HIGH);
}

void onButtonPress(uint16_t btnIndex, uint8_t btnType) {

  if (btnIndex == OSCA_PWM_SW && btnType == ROX_PRESSED) {
    vcoAPWMsource = vcoAPWMsource + 1;
    if (vcoAPWMsource > 3) {
      vcoAPWMsource = 0;
    }
    myControlChange(midiChannel, CCvcoAPWMsource, vcoAPWMsource);
  }

  if (btnIndex == OSCB_PWM_SW && btnType == ROX_PRESSED) {
    vcoBPWMsource = vcoBPWMsource + 1;
    if (vcoBPWMsource > 3) {
      vcoBPWMsource = 0;
    }
    myControlChange(midiChannel, CCvcoBPWMsource, vcoBPWMsource);
  }

  if (btnIndex == OSCC_PWM_SW && btnType == ROX_PRESSED) {
    vcoCPWMsource = vcoCPWMsource + 1;
    if (vcoCPWMsource > 3) {
      vcoCPWMsource = 0;
    }
    myControlChange(midiChannel, CCvcoCPWMsource, vcoCPWMsource);
  }

  if (btnIndex == OSCA_FM_SW && btnType == ROX_PRESSED) {
    vcoAFMsource = vcoAFMsource + 1;
    if (vcoAFMsource > 3) {
      vcoAFMsource = 0;
    }
    myControlChange(midiChannel, CCvcoAFMsource, vcoAFMsource);
  }

  if (btnIndex == OSCB_FM_SW && btnType == ROX_PRESSED) {
    vcoBFMsource = vcoBFMsource + 1;
    if (vcoBFMsource > 3) {
      vcoBFMsource = 0;
    }
    myControlChange(midiChannel, CCvcoBFMsource, vcoBFMsource);
  }

  if (btnIndex == OSCC_FM_SW && btnType == ROX_PRESSED) {
    vcoCFMsource = vcoCFMsource + 1;
    if (vcoCFMsource > 3) {
      vcoCFMsource = 0;
    }
    myControlChange(midiChannel, CCvcoCFMsource, vcoCFMsource);
  }

  if (btnIndex == LFO1_WAVE_SW && btnType == ROX_PRESSED) {
    LFO1Wave = LFO1Wave + 1;
    if (LFO1Wave > 6) {
      LFO1Wave = 0;
    }
    myControlChange(midiChannel, CCLFO1Wave, LFO1Wave);
  }

  if (btnIndex == LFO2_WAVE_SW && btnType == ROX_PRESSED) {
    LFO2Wave = LFO2Wave + 1;
    if (LFO2Wave > 6) {
      LFO2Wave = 0;
    }
    myControlChange(midiChannel, CCLFO2Wave, LFO2Wave);
  }

  if (btnIndex == FILTER_DEPTH_SW && btnType == ROX_PRESSED) {
        if (!filterLFODepthWasToggled) {
          // If it's already 0 and wasn't toggled, do nothing
          if (filterLFODepth == 0) return;

          // Save the current value and set to default
          lastfilterLFODepth = filterLFODepth;
          filterLFODepth = 0;
          filterLFODepthWasToggled = true;
        } else {
          // Toggle back to previous value
          filterLFODepth = lastfilterLFODepth;
          filterLFODepthWasToggled = false;
        }
    myControlChange(midiChannel, CCfilterLFODepthSW, filterLFODepth);
  }

  if (btnIndex == AMP_DEPTH_SW && btnType == ROX_PRESSED) {
        if (!ampLFODepthWasToggled) {
          // If it's already 0 and wasn't toggled, do nothing
          if (ampLFODepth == 0) return;

          // Save the current value and set to default
          lastampLFODepth = ampLFODepth;
          ampLFODepth = 0;
          ampLFODepthWasToggled = true;
        } else {
          // Toggle back to previous value
          ampLFODepth = lastampLFODepth;
          ampLFODepthWasToggled = false;
        }
    myControlChange(midiChannel, CCampLFODepthSW, ampLFODepth);
  }
}

void myControlChange(byte channel, byte control, int value) {
  switch (control) {

    case CCvcoAPWMsource:
      updatevcoAPWMsource();
      break;

    case CCvcoBPWMsource:
      updatevcoBPWMsource();
      break;

    case CCvcoCPWMsource:
      updatevcoCPWMsource();
      break;

    case CCvcoAFMsource:
      updatevcoAFMsource();
      break;

    case CCvcoBFMsource:
      updatevcoBFMsource();
      break;

    case CCvcoCFMsource:
      updatevcoCFMsource();
      break;

    case CCLFO1Wave:
      updateLFO1Wave();
      break;

    case CCLFO2Wave:
      updateLFO2Wave();
      break;

    case CCfilterLFODepthSW:
      updatefilterLFODepth();
      break;

    case CCampLFODepthSW:
      updateampLFODepth();
      break;

  }
}

// Get a pointer to the Nth voice mixer (0..7)
static inline AudioMixer4 *voiceMixer(int v) {
  switch (v) {
    case 0: return &voiceMix1;
    case 1: return &voiceMix2;
    case 2: return &voiceMix3;
    case 3: return &voiceMix4;
    case 4: return &voiceMix5;
    case 5: return &voiceMix6;
    case 6: return &voiceMix7;
    default: return &voiceMix8;
  }
}

void setVoiceAmpFromVelocity(int v, uint8_t vel) {
  float a = 0.33f * velToAmp(vel);  // each DCO amplitude
  VO[v].A->amplitude(a);
  VO[v].B->amplitude(a);
  VO[v].C->amplitude(a);
}

void triggerVoiceEnvelopes(int v, bool on) {
  if (on) {
    VO[v].env->noteOn();
    VO[v].envFilter->noteOn();
    VO[v].envAmp->noteOn();
  } else {
    VO[v].env->noteOff();
    VO[v].envFilter->noteOff();
    VO[v].envAmp->noteOff();
  }
}

// UI -> Hz helper: 0..127 -> ~0.05..20 Hz (expo)
inline float ui127_to_exp_hz(uint8_t val,
                             float fmin = 0.05f,
                             float fmax = 20.0f) {
  float n = val / 127.0f;
  return fmin * powf(fmax / fmin, n);
}

void updateLFO1Rate() {
  float hz = ui127_to_exp_hz(LFO1Rate);  // lfo1Rate: 0..127 encoder value
  LFO1.frequency(hz);                    // Teensy Audio: set LFO speed

  if (!recallPatchFlag) {
    showCurrentParameterPage("LFO1 Rate", String(hz, 2) + " Hz");
    startParameterDisplay();
  }
}

void updateLFO2Rate() {
  float hz = ui127_to_exp_hz(LFO2Rate);  // lfo1Rate: 0..127 encoder value
  LFO2.frequency(hz);                    // Teensy Audio: set LFO speed

  if (!recallPatchFlag) {
    showCurrentParameterPage("LFO2 Rate", String(hz, 2) + " Hz");
    startParameterDisplay();
  }
}

void updatevcoALevel() {
  if (!recallPatchFlag) {
    if (vcoALevel == 0) {
      showCurrentParameterPage("VCO A Level", String("Off"));
    } else {
      showCurrentParameterPage("VCO A Level", String(vcoALevel));
    }
    startParameterDisplay();
  }
  aLevel = vcoALevel / 255.0f;
  for (int v = 0; v < 8; v++) {
    AudioMixer4 *vm = voiceMixer(v);  // helper I gave earlier
    vm->gain(0, aLevel);
  }
}

void updatevcoBLevel() {
  if (!recallPatchFlag) {
    if (vcoBLevel == 0) {
      showCurrentParameterPage("VCO B Level", String("Off"));
    } else {
      showCurrentParameterPage("VCO B Level", String(vcoBLevel));
    }
    startParameterDisplay();
  }
  bLevel = vcoBLevel / 255.0f;
  for (int v = 0; v < 8; v++) {
    AudioMixer4 *vm = voiceMixer(v);  // helper I gave earlier
    vm->gain(1, bLevel);
  }
}

void updatevcoCLevel() {
  if (!recallPatchFlag) {
    if (vcoCLevel == 0) {
      showCurrentParameterPage("VCO C Level", String("Off"));
    } else {
      showCurrentParameterPage("VCO C Level", String(vcoCLevel));
    }
    startParameterDisplay();
  }
  cLevel = vcoCLevel / 255.0f;
  for (int v = 0; v < 8; v++) {
    AudioMixer4 *vm = voiceMixer(v);  // helper I gave earlier
    vm->gain(2, cLevel);
  }
}

inline float ui127_to_time_ms(uint8_t val,
                              float tmin = 1.0f,
                              float tmax = 11880.0f) {
  float n = val / 127.0f;
  return tmin * powf(tmax / tmin, n);
}

inline float ui100_to_sustain(uint8_t val) {
  return constrain(val / 100.0f, 0.0f, 1.0f);
}

void updatepitchAttack() {
  float ms = ui127_to_time_ms(pitchAttack);  // pitchAttack is 0..127
  if (!recallPatchFlag) {
    showCurrentParameterPage("Pitch Attack", String(ms, 0) + " ms");
    startParameterDisplay();
  }
  // Apply to all 8 voices via your VO[] table
  for (int i = 0; i < 8; ++i) {
    VO[i].env->attack(ms);
  }
}

void updatepitchDecay() {
  float ms = ui127_to_time_ms(pitchDecay);  // pitchDecay is 0..127
  if (!recallPatchFlag) {
    showCurrentParameterPage("Pitch Decay", String(ms, 0) + " ms");
    startParameterDisplay();
  }
  // Apply to all 8 voices via your VO[] table
  for (int i = 0; i < 8; ++i) {
    VO[i].env->decay(ms);
  }
}

void updatepitchSustain() {
  if (!recallPatchFlag) {
    showCurrentParameterPage("Pitch Sustain", String(pitchSustain));
    startParameterDisplay();
  }

  float sus = ui100_to_sustain(pitchSustain);
  for (int v = 0; v < 8; v++) {
    VO[v].env->sustain(sus);
  }
}

void updatepitchRelease() {
  float ms = ui127_to_time_ms(pitchRelease);  // pitchRelease is 0..127
  if (!recallPatchFlag) {
    showCurrentParameterPage("Pitch Release", String(ms, 0) + " ms");
    startParameterDisplay();
  }
  // Apply to all 8 voices via your VO[] table
  for (int i = 0; i < 8; ++i) {
    VO[i].env->release(ms);
  }
}

void updatefilterAttack() {
  float ms = ui127_to_time_ms(filterAttack);  // filterAttack is 0..127
  if (!recallPatchFlag) {
    showCurrentParameterPage("Filter Attack", String(ms, 0) + " ms");
    startParameterDisplay();
  }
  // Apply to all 8 voices via your VO[] table
  for (int i = 0; i < 8; ++i) {
    VO[i].envFilter->attack(ms);
  }
}

void updatefilterDecay() {
  float ms = ui127_to_time_ms(filterDecay);  // filterDecay is 0..127
  if (!recallPatchFlag) {
    showCurrentParameterPage("Filter Decay", String(ms, 0) + " ms");
    startParameterDisplay();
  }
  // Apply to all 8 voices via your VO[] table
  for (int i = 0; i < 8; ++i) {
    VO[i].envFilter->decay(ms);
  }
}

void updatefilterSustain() {
  if (!recallPatchFlag) {
    showCurrentParameterPage("Filter Sustain", String(filterSustain));
    startParameterDisplay();
  }

  float sus = ui100_to_sustain(filterSustain);
  for (int v = 0; v < 8; v++) {
    VO[v].envFilter->sustain(sus);
  }
}

void updatefilterRelease() {
  float ms = ui127_to_time_ms(filterRelease);  // filterRelease is 0..127
  if (!recallPatchFlag) {
    showCurrentParameterPage("Filter Release", String(ms, 0) + " ms");
    startParameterDisplay();
  }
  // Apply to all 8 voices via your VO[] table
  for (int i = 0; i < 8; ++i) {
    VO[i].envFilter->release(ms);
  }
}

void updateampAttack() {
  float ms = ui127_to_time_ms(ampAttack);  // ampAttack is 0..127
  if (!recallPatchFlag) {
    showCurrentParameterPage("Amp Attack", String(ms, 0) + " ms");
    startParameterDisplay();
  }
  // Apply to all 8 voices via your VO[] table
  for (int i = 0; i < 8; ++i) {
    VO[i].envAmp->attack(ms);
  }
}

void updateampDecay() {
  float ms = ui127_to_time_ms(ampDecay);  // ampDecay is 0..127
  if (!recallPatchFlag) {
    showCurrentParameterPage("Amp Decay", String(ms, 0) + " ms");
    startParameterDisplay();
  }
  // Apply to all 8 voices via your VO[] table
  for (int i = 0; i < 8; ++i) {
    VO[i].envAmp->decay(ms);
  }
}

void updateampSustain() {
  if (!recallPatchFlag) {
    showCurrentParameterPage("Amp Sustain", String(ampSustain));
    startParameterDisplay();
  }

  float sus = ui100_to_sustain(ampSustain);
  for (int v = 0; v < 8; v++) {
    VO[v].envAmp->sustain(sus);
  }
}

void updateampRelease() {
  float ms = ui127_to_time_ms(ampRelease);  // ampRelease is 0..127
  if (!recallPatchFlag) {
    showCurrentParameterPage("Amp Release", String(ms, 0) + " ms");
    startParameterDisplay();
  }
  // Apply to all 8 voices via your VO[] table
  for (int i = 0; i < 8; ++i) {
    VO[i].envAmp->release(ms);
  }
}

// 0..255 UI -> 0.0..1.0 pulse width
inline float ui255_to_pw(uint8_t v) {
  float pw = v / 255.0f;
  if (pw < 0.0f) pw = 0.0f;
  if (pw > 1.0f) pw = 1.0f;
  return pw;
}

void updatevcoAPW() {
  if (!recallPatchFlag) {
    showCurrentParameterPage("VCO A PW", String(vcoAPW));
    startParameterDisplay();
  }
  aPW = ui255_to_pw(vcoAPW);
  dc_pwmAbias.amplitude(aPW);  // <- baseline PW for all A oscillators
}

void updatevcoBPW() {
  if (!recallPatchFlag) {
    showCurrentParameterPage("VCO B PW", String(vcoBPW));
    startParameterDisplay();
  }
  bPW = ui255_to_pw(vcoBPW);
  dc_pwmBbias.amplitude(bPW);  // <- baseline PW for all B oscillators
}

void updatevcoCPW() {
  if (!recallPatchFlag) {
    showCurrentParameterPage("VCO C PW", String(vcoCPW));
    startParameterDisplay();
  }
  cPW = ui255_to_pw(vcoCPW);
  dc_pwmCbias.amplitude(cPW);  // <- baseline PW for all C oscillators
}

inline float ui255_to_01(uint8_t v) {
  return v / 255.0f;
}
constexpr int VOICES = 8;  // you’re indexing 1..8

void updatevcoAPWM() {
  if (!recallPatchFlag) {
    showCurrentParameterPage("VCO A PWM", vcoAPWM ? String(vcoAPWM) : String("Off"));
    startParameterDisplay();
  }
  aPWM = ui255_to_01(vcoAPWM);
  const float lfoGain = 0.5f * aPWM;
  switch (vcoAPWMsource) {
    case 1:
      for (int v = 1; v <= VOICES; ++v) pwmA[v]->gain(0, lfoGain);  // input 0 = LFO2
      break;

    case 2:
      for (int v = 1; v <= VOICES; ++v) pwmA[v]->gain(1, lfoGain);  // input 1 = env1
      break;

    case 3:
      for (int v = 1; v <= VOICES; ++v) pwmA[v]->gain(2, lfoGain);  // input 2 = inv env1
      break;
  }
}

void updatevcoBPWM() {
  if (!recallPatchFlag) {
    showCurrentParameterPage("VCO B PWM", vcoBPWM ? String(vcoBPWM) : String("Off"));
    startParameterDisplay();
  }
  bPWM = ui255_to_01(vcoBPWM);
  const float lfoGain = 0.5f * bPWM;
  switch (vcoBPWMsource) {
    case 1:
      for (int v = 1; v <= VOICES; ++v) pwmB[v]->gain(0, lfoGain);  // input 0 = LFO2
      break;

    case 2:
      for (int v = 1; v <= VOICES; ++v) pwmB[v]->gain(1, lfoGain);  // input 1 = env1
      break;

    case 3:
      for (int v = 1; v <= VOICES; ++v) pwmB[v]->gain(2, lfoGain);  // input 2 = inv env1
      break;
  }
}

void updatevcoCPWM() {
  if (!recallPatchFlag) {
    showCurrentParameterPage("VCO C PWM", vcoCPWM ? String(vcoCPWM) : String("Off"));
    startParameterDisplay();
  }
  cPWM = ui255_to_01(vcoCPWM);
  const float lfoGain = 0.5f * cPWM;
  switch (vcoCPWMsource) {
    case 1:
      for (int v = 1; v <= VOICES; ++v) pwmC[v]->gain(0, lfoGain);  // input 0 = LFO2
      break;

    case 2:
      for (int v = 1; v <= VOICES; ++v) pwmC[v]->gain(1, lfoGain);  // input 1 = env1
      break;

    case 3:
      for (int v = 1; v <= VOICES; ++v) pwmC[v]->gain(2, lfoGain);  // input 2 = inv env1
      break;
  }
}

void updateXModDepth() {
  if (!recallPatchFlag) {
    showCurrentParameterPage("XMOD Depth", XModDepth ? String(XModDepth) : String("Off"));
    startParameterDisplay();
  }
  bXModDepth = XModDepth / 255.0f;
  for (int v = 1; v <= VOICES; ++v) pitchA[v]->gain(3, bXModDepth);  // input 3 = oscB output
}

void updatevcoAInterval() {
  if (!recallPatchFlag) {
    showCurrentParameterPage("VCO A Int", String(vcoAInterval));
    startParameterDisplay();
  }
  pitchDirty = true;
}

void updatevcoBInterval() {
  if (!recallPatchFlag) {
    showCurrentParameterPage("VCO B Int", String(vcoBInterval));
    startParameterDisplay();
  }
  pitchDirty = true;
}

void updatevcoCInterval() {
  if (!recallPatchFlag) {
    showCurrentParameterPage("VCO C Int", String(vcoCInterval));
    startParameterDisplay();
  }
  pitchDirty = true;
}

void updatevcoAFMDepth() {
  if (!recallPatchFlag) {
    if (vcoAFMDepth == 0) {
      showCurrentParameterPage("A FM Depth", String("Off"));
    } else {
      showCurrentParameterPage("A FM Depth", String(vcoAFMDepth));
    }
    startParameterDisplay();
  }
  aFMDepth = vcoAFMDepth / 511.0f;
  switch (vcoAFMsource) {
    case 1:
      for (int v = 1; v <= VOICES; ++v) pitchA[v]->gain(0, aFMDepth);  // input 0 = LFO1
      break;

    case 2:
      for (int v = 1; v <= VOICES; ++v) pitchA[v]->gain(1, aFMDepth);  // input 1 = env1
      break;

    case 3:
      for (int v = 1; v <= VOICES; ++v) pitchA[v]->gain(2, aFMDepth);  // input 2 = inv env1
      break;
  }
}

void updatevcoBFMDepth() {
  if (!recallPatchFlag) {
    if (vcoBFMDepth == 0) {
      showCurrentParameterPage("B FM Depth", String("Off"));
    } else {
      showCurrentParameterPage("B FM Depth", String(vcoBFMDepth));
    }
    startParameterDisplay();
  }
  bFMDepth = vcoBFMDepth / 511.0f;
  switch (vcoBFMsource) {
    case 1:
      for (int v = 1; v <= VOICES; ++v) pitchB[v]->gain(0, bFMDepth);  // input 0 = LFO1
      break;

    case 2:
      for (int v = 1; v <= VOICES; ++v) pitchB[v]->gain(1, bFMDepth);  // input 1 = env1
      break;

    case 3:
      for (int v = 1; v <= VOICES; ++v) pitchB[v]->gain(2, bFMDepth);  // input 2 = inv env1
      break;
  }
}

void updatevcoCFMDepth() {
  if (!recallPatchFlag) {
    if (vcoCFMDepth == 0) {
      showCurrentParameterPage("C FM Depth", String("Off"));
    } else {
      showCurrentParameterPage("C FM Depth", String(vcoCFMDepth));
    }
    startParameterDisplay();
  }
  cFMDepth = vcoCFMDepth / 511.0f;
  switch (vcoCFMsource) {
    case 1:
      for (int v = 1; v <= VOICES; ++v) pitchC[v]->gain(0, cFMDepth);  // input 0 = LFO1
      break;

    case 2:
      for (int v = 1; v <= VOICES; ++v) pitchC[v]->gain(1, cFMDepth);  // input 1 = env1
      break;

    case 3:
      for (int v = 1; v <= VOICES; ++v) pitchC[v]->gain(2, cFMDepth);  // input 2 = inv env1
      break;
  }
}

void updatevcoBDetune() {
  if (!recallPatchFlag) {
    showCurrentParameterPage("VCO B Detune", String(vcoBDetune));
    startParameterDisplay();
  }
  pitchDirty = true;
  bDetune = 1.0f + ((vcoBDetune - 64) / 64.0f) * 0.05f;
}

void updatevcoCDetune() {
  if (!recallPatchFlag) {
    showCurrentParameterPage("VCO C Detune", String(vcoCDetune));
    startParameterDisplay();
  }
  pitchDirty = true;
  cDetune = 1.0f + ((vcoCDetune - 64) / 64.0f) * 0.05f;
}

void updatevcoAWave() {
  aWave = vcoAWave;
  if (!recallPatchFlag) {
    switch (aWave) {
      case 0:
        showCurrentParameterPage("VCO A Wave", "Sine");
        break;
      case 1:
        showCurrentParameterPage("VCO A Wave", "Saw");
        break;
      case 2:
        showCurrentParameterPage("VCO A Wave", "Reverse Saw");
        break;
      case 3:
        showCurrentParameterPage("VCO A Wave", "Square");
        break;
      case 4:
        showCurrentParameterPage("VCO A Wave", "Triangle");
        break;
      case 5:
        showCurrentParameterPage("VCO A Wave", "Pulse");
        break;
      case 6:
        showCurrentParameterPage("VCO A Wave", "S & H");
        break;
    }
    startParameterDisplay();
  }
  switch (aWave) {
    case 0:
      for (int v = 1; v < 9; v++) {
        dcoA[v]->begin(WAVEFORM_SINE);
      }
      break;
    case 1:
      for (int v = 1; v < 9; v++) {
        dcoA[v]->begin(WAVEFORM_BANDLIMIT_SAWTOOTH);
      }
      break;
    case 2:
      for (int v = 1; v < 9; v++) {
        dcoA[v]->begin(WAVEFORM_BANDLIMIT_SAWTOOTH_REVERSE);
      }
      break;
    case 3:
      for (int v = 1; v < 9; v++) {
        dcoA[v]->begin(WAVEFORM_BANDLIMIT_SQUARE);
      }
      break;
    case 4:
      for (int v = 1; v < 9; v++) {
        dcoA[v]->begin(WAVEFORM_TRIANGLE_VARIABLE);
      }
      break;
    case 5:
      for (int v = 1; v < 9; v++) {
        dcoA[v]->begin(WAVEFORM_BANDLIMIT_PULSE);
      }
      break;
    case 6:
      for (int v = 1; v < 9; v++) {
        dcoA[v]->begin(WAVEFORM_SAMPLE_HOLD);
      }
      break;
  }
  // for (int v = 0; v < 8; v++) {
  //   AudioMixer4 *vm = voiceMixer(v);  // helper I gave earlier
  //   vm->gain(0, aLevel);
  // }
}

void updatevcoBWave() {
  bWave = vcoBWave;
  if (!recallPatchFlag) {
    switch (bWave) {
      case 0:
        showCurrentParameterPage("VCO B Wave", "Sine");
        break;
      case 1:
        showCurrentParameterPage("VCO B Wave", "Saw");
        break;
      case 2:
        showCurrentParameterPage("VCO B Wave", "Reverse Saw");
        break;
      case 3:
        showCurrentParameterPage("VCO B Wave", "Square");
        break;
      case 4:
        showCurrentParameterPage("VCO B Wave", "Triangle");
        break;
      case 5:
        showCurrentParameterPage("VCO B Wave", "Pulse");
        break;
      case 6:
        showCurrentParameterPage("VCO B Wave", "S & H");
        break;
    }
    startParameterDisplay();
  }
  switch (bWave) {
    case 0:
      for (int v = 1; v < 9; v++) {
        dcoB[v]->begin(WAVEFORM_SINE);
      }
      break;
    case 1:
      for (int v = 1; v < 9; v++) {
        dcoB[v]->begin(WAVEFORM_BANDLIMIT_SAWTOOTH);
      }
      break;
    case 2:
      for (int v = 1; v < 9; v++) {
        dcoB[v]->begin(WAVEFORM_BANDLIMIT_SAWTOOTH_REVERSE);
      }
      break;
    case 3:
      for (int v = 1; v < 9; v++) {
        dcoB[v]->begin(WAVEFORM_BANDLIMIT_SQUARE);
      }
      break;
    case 4:
      for (int v = 1; v < 9; v++) {
        dcoB[v]->begin(WAVEFORM_TRIANGLE_VARIABLE);
      }
      break;
    case 5:
      for (int v = 1; v < 9; v++) {
        dcoB[v]->begin(WAVEFORM_BANDLIMIT_PULSE);
      }
      break;
    case 6:
      for (int v = 1; v < 9; v++) {
        dcoB[v]->begin(WAVEFORM_SAMPLE_HOLD);
      }
      break;
  }
  //   for (int v = 0; v < 8; v++) {
  //   AudioMixer4 *vm = voiceMixer(v);  // helper I gave earlier
  //   vm->gain(1, bLevel);
  // }
}

void updatevcoCWave() {
  cWave = vcoCWave;
  if (!recallPatchFlag) {
    switch (cWave) {
      case 0:
        showCurrentParameterPage("VCO C Wave", "Sine");
        break;
      case 1:
        showCurrentParameterPage("VCO C Wave", "Saw");
        break;
      case 2:
        showCurrentParameterPage("VCO C Wave", "Reverse Saw");
        break;
      case 3:
        showCurrentParameterPage("VCO C Wave", "Square");
        break;
      case 4:
        showCurrentParameterPage("VCO C Wave", "Triangle");
        break;
      case 5:
        showCurrentParameterPage("VCO C Wave", "Pulse");
        break;
      case 6:
        showCurrentParameterPage("VCO C Wave", "S & H");
        break;
    }
    startParameterDisplay();
  }
  switch (cWave) {
    case 0:
      for (int v = 1; v < 9; v++) {
        dcoC[v]->begin(WAVEFORM_SINE);
      }
      break;
    case 1:
      for (int v = 1; v < 9; v++) {
        dcoC[v]->begin(WAVEFORM_BANDLIMIT_SAWTOOTH);
      }
      break;
    case 2:
      for (int v = 1; v < 9; v++) {
        dcoC[v]->begin(WAVEFORM_BANDLIMIT_SAWTOOTH_REVERSE);
      }
      break;
    case 3:
      for (int v = 1; v < 9; v++) {
        dcoC[v]->begin(WAVEFORM_BANDLIMIT_SQUARE);
      }
      break;
    case 4:
      for (int v = 1; v < 9; v++) {
        dcoC[v]->begin(WAVEFORM_TRIANGLE_VARIABLE);
      }
      break;
    case 5:
      for (int v = 1; v < 9; v++) {
        dcoC[v]->begin(WAVEFORM_BANDLIMIT_PULSE);
      }
      break;
    case 6:
      for (int v = 1; v < 9; v++) {
        dcoC[v]->begin(WAVEFORM_SAMPLE_HOLD);
      }
      break;
  }
  //   for (int v = 0; v < 8; v++) {
  //   AudioMixer4 *vm = voiceMixer(v);  // helper I gave earlier
  //   vm->gain(2, cLevel);
  // }
}

void updatefilterCutoff() {
  if (!recallPatchFlag) {
    showCurrentParameterPage("VCF Cutoff", String(filterCutoff));
    startParameterDisplay();
  }
}

void updatefilterResonance() {
  if (!recallPatchFlag) {
    showCurrentParameterPage("VCF Res", String(filterResonance));
    startParameterDisplay();
  }
}

void updatefilterEGDepth() {
  if (!recallPatchFlag) {
    showCurrentParameterPage("VCF EG Depth", String(filterEGDepth));
    startParameterDisplay();
  }
}

void updatefilterKeyTrack() {
  if (!recallPatchFlag) {
    showCurrentParameterPage("VCF Keytrack", String(filterKeyTrack));
    startParameterDisplay();
  }
}

void updatefilterLFODepth() {
  if (!recallPatchFlag) {
    if (filterLFODepth == 0) {
      showCurrentParameterPage("LFO Depth", String("Off"));
    } else if (filterLFODepth < 0) {
      float positive_filterLFODepth = abs(filterLFODepth);
      showCurrentParameterPage("LFO1 Depth", String(positive_filterLFODepth));
    } else {
      showCurrentParameterPage("LFO2 Depth", String(filterLFODepth));
    }
    startParameterDisplay();
  }
}

void updatenoiseLevel() {
  if (!recallPatchFlag) {
    if (noiseLevel == 0) {
      showCurrentParameterPage("Noise Level", String("Off"));
    } else if (noiseLevel < 0) {
      float positive_noiseLevel = abs(noiseLevel);
      showCurrentParameterPage("Pink Level", String(positive_noiseLevel));
    } else {
      showCurrentParameterPage("White Level", String(noiseLevel));
    }
    startParameterDisplay();
  }
  // magnitude 0..1 from |−127..+127|
  const float mag = fabsf((float)noiseLevel) / 127.0f;

  // select source into noiseMix (0=white, 1=pink)
  // We keep source at unity and use per-voice gain as the overall level.
  Serial.print("Noise Level ");
  Serial.println(noiseLevel);
  Serial.print("Mix level ");
  Serial.println(mag);
  noiseMix.gain(0, (noiseLevel > 0) ? 1.0f : 0.0f);  // white
  noiseMix.gain(1, (noiseLevel < 0) ? 1.0f : 0.0f);  // pink
  // (If you ever want crossfade instead of either/or, set both to
  // fractions that sum to 1, and reduce per-voice mag accordingly.)

  // push noise amount into each voice’s mixer input 3
  for (int v = 0; v < 8; v++) {
    AudioMixer4 *vm = voiceMixer(v);   // your helper that returns &voiceMixN
    vm->gain(3, mag);                  // input 3 is noise
  }
}

void updateampLFODepth() {
  if (!recallPatchFlag) {
    if (ampLFODepth == 0) {
      showCurrentParameterPage("LFO Depth", String("Off"));
    } else if (ampLFODepth < 0) {
      float positive_ampLFODepth = abs(ampLFODepth);
      showCurrentParameterPage("LFO1 Depth", String(positive_ampLFODepth));
    } else {
      showCurrentParameterPage("LFO2 Depth", String(ampLFODepth));
    }
    startParameterDisplay();
  }
}

// Convert 0..1 → 12-bit (matches helpers shown earlier)
static inline uint16_t to12(float x) {
  x = clamp01(x);
  return (uint16_t)(x * 4095.0f + 0.5f);
}

inline void split_bipolar_depth(int val, float &d1, float &d2) {
  if (val < 0) {
    d1 = (-val) / 127.0f;
    d2 = 0.0f;
  } else {
    d1 = 0.0f;
    d2 = val / 127.0f;
  }
}

void updateFilterDACAll() {
  const float baseCut = filterCutoff / 255.0f;
  const float ktDepth = filterKeyTrack / 255.0f;
  const float envDepth = filterEGDepth / 255.0f;

  // Split the bipolar depth into two unipolar amounts
  float d1 = 0, d2 = 0;  // d1 -> LFO1 depth (0..1), d2 -> LFO2 depth (0..1)
  split_bipolar_depth(filterLFODepth, d1, d2);

  // Unipolar LFOs from the latest samples
  const float lfo1_uni = 0.5f * g_latestLFO + 0.5f;   // your existing LFO1 filter tap
  const float lfo2_uni = 0.5f * g_latestLFO2 + 0.5f;  // new LFO2 filter tap

  for (int v = 1; v <= 8; ++v) {
    float cv = baseCut;

    // Key-track
    const float midi = freq_to_midi(voiceFreq(v));
    const float kt01 = midi_to01(midi);
    cv += ktDepth * kt01;

    // Filter Env (0..1)
    cv += envDepth * g_latestEnv[v];

    // Add LFO contribution (left knob => LFO1, right knob => LFO2)
    cv += d1 * lfo1_uni + d2 * lfo2_uni;

    // Clamp -> 12-bit -> DAC
    cv = clamp01(cv);
    const uint16_t code12 = (uint16_t)lroundf(cv * 4095.0f);
    const uint8_t ch = (uint8_t)(DAC_A + (v - 1));
    dacWriteBuffered(DAC_FILTER, ch, code12);
  }
  ldacStrobe();
}


void updateAmpDACAll() {
  float d1 = 0, d2 = 0;  // d1 -> LFO1 trem depth, d2 -> LFO2 trem depth
  split_bipolar_depth(ampLFODepth, d1, d2);

  const float lfo1_uni_amp = 0.5f * g_latestLFO_amp + 0.5f;   // your LFO1 amp tap
  const float lfo2_uni_amp = 0.5f * g_latestLFO2_amp + 0.5f;  // new LFO2 amp tap

  for (int v = 1; v <= 8; ++v) {
    float env = g_latestAmpEnv[v];  // 0..1
    if (env < 0) env = 0;
    if (env > 1) env = 1;

    // Tremolo: keep average level equal to 'env'
    // (1 - d1 - d2) is the dry proportion; then add the two LFO contributions.
    float gain = (1.0f - d1 - d2) + d1 * lfo1_uni_amp + d2 * lfo2_uni_amp;
    if (gain < 0) gain = 0;
    if (gain > 1) gain = 1;

    float cv = env * gain;
    const uint16_t code12 = (uint16_t)lroundf(clamp01(cv) * 4095.0f);

    const uint8_t ch = (uint8_t)(DAC_A + (v - 1));
    dacWriteBuffered(DAC_AMP, ch, code12);
  }
  ldacStrobe();
}


void updatevcoAPWMsource() {
  switch (vcoAPWMsource) {
    case 0:  // no modulation
      if (!recallPatchFlag) {
        showCurrentParameterPage("VCO A PWM", "Off");
      }
      mcp8.digitalWrite(PWM_A_GREEN, LOW);
      mcp8.digitalWrite(PWM_A_RED, LOW);
      for (int v = 1; v <= VOICES; ++v) pwmA[v]->gain(0, 0);  // input 0 = LFO2
      for (int v = 1; v <= VOICES; ++v) pwmA[v]->gain(1, 0);  // input 0 = env1
      for (int v = 1; v <= VOICES; ++v) pwmA[v]->gain(2, 0);  // input 0 = inv env1
      break;

    case 1:  // LFO2 Modulation
      if (!recallPatchFlag) {
        showCurrentParameterPage("VCO A PWM", "LFO2");
      }
      mcp8.digitalWrite(PWM_A_GREEN, LOW);
      mcp8.digitalWrite(PWM_A_RED, HIGH);
      for (int v = 1; v <= VOICES; ++v) pwmA[v]->gain(1, 0);  // input 0 = env1
      for (int v = 1; v <= VOICES; ++v) pwmA[v]->gain(2, 0);  // input 0 = inv env1
      break;

    case 2:  // Env Modulation
      if (!recallPatchFlag) {
        showCurrentParameterPage("VCO A PWM", "Pitch Env");
      }
      mcp8.digitalWrite(PWM_A_GREEN, HIGH);
      mcp8.digitalWrite(PWM_A_RED, LOW);
      for (int v = 1; v <= VOICES; ++v) pwmA[v]->gain(0, 0);  // input 0 = LFO2
      for (int v = 1; v <= VOICES; ++v) pwmA[v]->gain(2, 0);  // input 0 = inv env1
      break;

    case 3:  // Inv Env Modulation
      if (!recallPatchFlag) {
        showCurrentParameterPage("VCO A PWM", "Pitch Env Inv");
      }
      mcp8.digitalWrite(PWM_A_GREEN, HIGH);
      mcp8.digitalWrite(PWM_A_RED, HIGH);
      for (int v = 1; v <= VOICES; ++v) pwmA[v]->gain(0, 0);  // input 0 = LFO2
      for (int v = 1; v <= VOICES; ++v) pwmA[v]->gain(1, 0);  // input 0 = env1
      break;
  }
  startParameterDisplay();
  recallPatchFlag = true;
  updatevcoAPWM();
}

void updatevcoBPWMsource() {
  switch (vcoBPWMsource) {
    case 0:  // no modulation
      if (!recallPatchFlag) {
        showCurrentParameterPage("VCO B PWM", "Off");
      }
      mcp8.digitalWrite(PWM_B_GREEN, LOW);
      mcp8.digitalWrite(PWM_B_RED, LOW);
      for (int v = 1; v <= VOICES; ++v) pwmB[v]->gain(0, 0);  // input 0 = LFO2
      for (int v = 1; v <= VOICES; ++v) pwmB[v]->gain(1, 0);  // input 0 = env1
      for (int v = 1; v <= VOICES; ++v) pwmB[v]->gain(2, 0);  // input 0 = inv env1
      break;

    case 1:  // LFO2 Modulation
      if (!recallPatchFlag) {
        showCurrentParameterPage("VCO B PWM", "LFO2");
      }
      mcp8.digitalWrite(PWM_B_GREEN, LOW);
      mcp8.digitalWrite(PWM_B_RED, HIGH);
      for (int v = 1; v <= VOICES; ++v) pwmB[v]->gain(1, 0);  // input 0 = env1
      for (int v = 1; v <= VOICES; ++v) pwmB[v]->gain(2, 0);  // input 0 = inv env1
      break;

    case 2:  // Env Modulation
      if (!recallPatchFlag) {
        showCurrentParameterPage("VCO B PWM", "Pitch Env");
      }
      mcp8.digitalWrite(PWM_B_GREEN, HIGH);
      mcp8.digitalWrite(PWM_B_RED, LOW);
      for (int v = 1; v <= VOICES; ++v) pwmB[v]->gain(0, 0);  // input 0 = LFO2
      for (int v = 1; v <= VOICES; ++v) pwmB[v]->gain(2, 0);  // input 0 = inv env1
      break;

    case 3:  // Inv Env Modulation
      if (!recallPatchFlag) {
        showCurrentParameterPage("VCO B PWM", "Pitch Env Inv");
      }
      mcp8.digitalWrite(PWM_B_GREEN, HIGH);
      mcp8.digitalWrite(PWM_B_RED, HIGH);
      for (int v = 1; v <= VOICES; ++v) pwmB[v]->gain(0, 0);  // input 0 = LFO2
      for (int v = 1; v <= VOICES; ++v) pwmB[v]->gain(1, 0);  // input 0 = env1
      break;
  }
  startParameterDisplay();
  recallPatchFlag = true;
  updatevcoBPWM();
}

void updatevcoCPWMsource() {
  switch (vcoCPWMsource) {
    case 0:  // no modulation
      if (!recallPatchFlag) {
        showCurrentParameterPage("VCO C PWM", "Off");
      }
      mcp7.digitalWrite(PWM_C_GREEN, LOW);
      mcp7.digitalWrite(PWM_C_RED, LOW);
      for (int v = 1; v <= VOICES; ++v) pwmC[v]->gain(0, 0);  // input 0 = LFO2
      for (int v = 1; v <= VOICES; ++v) pwmC[v]->gain(1, 0);  // input 0 = env1
      for (int v = 1; v <= VOICES; ++v) pwmC[v]->gain(2, 0);  // input 0 = inv env1
      break;

    case 1:  // LFO2 Modulation
      if (!recallPatchFlag) {
        showCurrentParameterPage("VCO C PWM", "LFO2");
      }
      mcp7.digitalWrite(PWM_C_GREEN, LOW);
      mcp7.digitalWrite(PWM_C_RED, HIGH);
      for (int v = 1; v <= VOICES; ++v) pwmC[v]->gain(1, 0);  // input 0 = env1
      for (int v = 1; v <= VOICES; ++v) pwmC[v]->gain(2, 0);  // input 0 = inv env1
      break;

    case 2:  // Env Modulation
      if (!recallPatchFlag) {
        showCurrentParameterPage("VCO C PWM", "Pitch Env");
      }
      mcp7.digitalWrite(PWM_C_GREEN, HIGH);
      mcp7.digitalWrite(PWM_C_RED, LOW);
      for (int v = 1; v <= VOICES; ++v) pwmC[v]->gain(0, 0);  // input 0 = LFO2
      for (int v = 1; v <= VOICES; ++v) pwmC[v]->gain(2, 0);  // input 0 = inv env1
      break;

    case 3:  // Inv Env Modulation
      if (!recallPatchFlag) {
        showCurrentParameterPage("VCO C PWM", "Pitch Env Inv");
      }
      mcp7.digitalWrite(PWM_C_GREEN, HIGH);
      mcp7.digitalWrite(PWM_C_RED, HIGH);
      for (int v = 1; v <= VOICES; ++v) pwmC[v]->gain(0, 0);  // input 0 = LFO2
      for (int v = 1; v <= VOICES; ++v) pwmC[v]->gain(1, 0);  // input 0 = env1
      break;
  }
  startParameterDisplay();
  recallPatchFlag = true;
  updatevcoCPWM();
}

void updatevcoAFMsource() {
  switch (vcoAFMsource) {
    case 0:  // no modulation
      if (!recallPatchFlag) {
        showCurrentParameterPage("VCO A FM", "Off");
      }
      mcp7.digitalWrite(FM_A_GREEN, LOW);
      mcp7.digitalWrite(FM_A_RED, LOW);

      for (int v = 1; v <= VOICES; ++v) pitchA[v]->gain(0, 0);  // input 0 = LFO2
      for (int v = 1; v <= VOICES; ++v) pitchA[v]->gain(1, 0);  // input 0 = env1
      for (int v = 1; v <= VOICES; ++v) pitchA[v]->gain(2, 0);  // input 0 = inv env1
      break;

    case 1:  // LFO2 Modulation
      if (!recallPatchFlag) {
        showCurrentParameterPage("VCO A FM", "LFO1");
      }
      mcp7.digitalWrite(FM_A_GREEN, LOW);
      mcp7.digitalWrite(FM_A_RED, HIGH);
      for (int v = 1; v <= VOICES; ++v) pitchA[v]->gain(1, 0);  // input 0 = env1
      for (int v = 1; v <= VOICES; ++v) pitchA[v]->gain(2, 0);  // input 0 = inv env1
      break;

    case 2:  // Env Modulation
      if (!recallPatchFlag) {
        showCurrentParameterPage("VCO A FM", "Pitch Env");
      }
      mcp7.digitalWrite(FM_A_GREEN, HIGH);
      mcp7.digitalWrite(FM_A_RED, LOW);
      for (int v = 1; v <= VOICES; ++v) pitchA[v]->gain(0, 0);  // input 0 = LFO2
      for (int v = 1; v <= VOICES; ++v) pitchA[v]->gain(2, 0);  // input 0 = inv env1
      break;

    case 3:  // Inv Env Modulation
      if (!recallPatchFlag) {
        showCurrentParameterPage("VCO A FM", "Pitch Env Inv");
      }
      mcp7.digitalWrite(FM_A_GREEN, HIGH);
      mcp7.digitalWrite(FM_A_RED, HIGH);
      for (int v = 1; v <= VOICES; ++v) pitchA[v]->gain(0, 0);  // input 0 = LFO2
      for (int v = 1; v <= VOICES; ++v) pitchA[v]->gain(1, 0);  // input 0 = env1
      break;
  }
  startParameterDisplay();
  recallPatchFlag = true;
  updatevcoAFMDepth();
}

void updatevcoBFMsource() {
  switch (vcoBFMsource) {
    case 0:  // no modulation
      if (!recallPatchFlag) {
        showCurrentParameterPage("VCO B FM", "Off");
      }
      mcp6.digitalWrite(FM_B_GREEN, LOW);
      mcp6.digitalWrite(FM_B_RED, LOW);
      for (int v = 1; v <= VOICES; ++v) pitchB[v]->gain(0, 0);  // input 0 = LFO2
      for (int v = 1; v <= VOICES; ++v) pitchB[v]->gain(1, 0);  // input 0 = env1
      for (int v = 1; v <= VOICES; ++v) pitchB[v]->gain(2, 0);  // input 0 = inv env1
      break;

    case 1:  // LFO2 Modulation
      if (!recallPatchFlag) {
        showCurrentParameterPage("VCO B FM", "LFO1");
      }
      mcp6.digitalWrite(FM_B_GREEN, LOW);
      mcp6.digitalWrite(FM_B_RED, HIGH);
      for (int v = 1; v <= VOICES; ++v) pitchB[v]->gain(1, 0);  // input 0 = env1
      for (int v = 1; v <= VOICES; ++v) pitchB[v]->gain(2, 0);  // input 0 = inv env1
      break;

    case 2:  // Env Modulation
      if (!recallPatchFlag) {
        showCurrentParameterPage("VCO B FM", "Pitch Env");
      }
      mcp6.digitalWrite(FM_B_GREEN, HIGH);
      mcp6.digitalWrite(FM_B_RED, LOW);
      for (int v = 1; v <= VOICES; ++v) pitchB[v]->gain(0, 0);  // input 0 = LFO2
      for (int v = 1; v <= VOICES; ++v) pitchB[v]->gain(2, 0);  // input 0 = inv env1
      break;

    case 3:  // Inv Env Modulation
      if (!recallPatchFlag) {
        showCurrentParameterPage("VCO B FM", "Pitch Env Inv");
      }
      mcp6.digitalWrite(FM_B_GREEN, HIGH);
      mcp6.digitalWrite(FM_B_RED, HIGH);
      for (int v = 1; v <= VOICES; ++v) pitchB[v]->gain(0, 0);  // input 0 = LFO2
      for (int v = 1; v <= VOICES; ++v) pitchB[v]->gain(1, 0);  // input 0 = env1
      break;
  }
  startParameterDisplay();
  recallPatchFlag = true;
  updatevcoBFMDepth();
}

void updatevcoCFMsource() {
  switch (vcoCFMsource) {
    case 0:  // no modulation
      if (!recallPatchFlag) {
        showCurrentParameterPage("VCO C FM", "Off");
      }
      mcp6.digitalWrite(FM_C_GREEN, LOW);
      mcp6.digitalWrite(FM_C_RED, LOW);
      for (int v = 1; v <= VOICES; ++v) pitchC[v]->gain(0, 0);  // input 0 = LFO2
      for (int v = 1; v <= VOICES; ++v) pitchC[v]->gain(1, 0);  // input 0 = env1
      for (int v = 1; v <= VOICES; ++v) pitchC[v]->gain(2, 0);  // input 0 = inv env1
      break;

    case 1:  // LFO2 Modulation
      if (!recallPatchFlag) {
        showCurrentParameterPage("VCO C FM", "LFO1");
      }
      mcp6.digitalWrite(FM_C_GREEN, LOW);
      mcp6.digitalWrite(FM_C_RED, HIGH);
      for (int v = 1; v <= VOICES; ++v) pitchC[v]->gain(1, 0);  // input 0 = env1
      for (int v = 1; v <= VOICES; ++v) pitchC[v]->gain(2, 0);  // input 0 = inv env1
      break;

    case 2:  // Env Modulation
      if (!recallPatchFlag) {
        showCurrentParameterPage("VCO C FM", "Pitch Env");
      }
      mcp6.digitalWrite(FM_C_GREEN, HIGH);
      mcp6.digitalWrite(FM_C_RED, LOW);
      for (int v = 1; v <= VOICES; ++v) pitchC[v]->gain(0, 0);  // input 0 = LFO2
      for (int v = 1; v <= VOICES; ++v) pitchC[v]->gain(2, 0);  // input 0 = inv env1
      break;

    case 3:  // Inv Env Modulation
      if (!recallPatchFlag) {
        showCurrentParameterPage("VCO C FM", "Pitch Env Inv");
      }
      mcp6.digitalWrite(FM_C_GREEN, HIGH);
      mcp6.digitalWrite(FM_C_RED, HIGH);
      for (int v = 1; v <= VOICES; ++v) pitchC[v]->gain(0, 0);  // input 0 = LFO2
      for (int v = 1; v <= VOICES; ++v) pitchC[v]->gain(1, 0);  // input 0 = env1
      break;
  }
  startParameterDisplay();
  recallPatchFlag = true;
  updatevcoCFMDepth();
}

void updateLFO1Wave() {
  if (!recallPatchFlag) {
    switch (LFO1Wave) {
      case 0:
        showCurrentParameterPage("LFO1 Wave", "Sine");
        break;
      case 1:
        showCurrentParameterPage("LFO1 Wave", "Saw");
        break;
      case 2:
        showCurrentParameterPage("LFO1 Wave", "Reverse Saw");
        break;
      case 3:
        showCurrentParameterPage("LFO1 Wave", "Square");
        break;
      case 4:
        showCurrentParameterPage("LFO1 Wave", "Triangle");
        break;
      case 5:
        showCurrentParameterPage("LFO1 Wave", "Pulse");
        break;
      case 6:
        showCurrentParameterPage("LFO1 Wave", "S & H");
        break;
    }
    startParameterDisplay();
  }
  switch (LFO1Wave) {
    case 0:
      LFO1.begin(WAVEFORM_SINE);
      break;
    case 1:
      LFO1.begin(WAVEFORM_BANDLIMIT_SAWTOOTH);
      break;
    case 2:
      LFO1.begin(WAVEFORM_BANDLIMIT_SAWTOOTH_REVERSE);
      break;
    case 3:
      LFO1.begin(WAVEFORM_BANDLIMIT_SQUARE);
      break;
    case 4:
      LFO1.begin(WAVEFORM_TRIANGLE_VARIABLE);
      break;
    case 5:
      LFO1.begin(WAVEFORM_BANDLIMIT_PULSE);
      break;
    case 6:
      LFO1.begin(WAVEFORM_SAMPLE_HOLD);
      break;
  }
}

void updateLFO2Wave() {
  if (!recallPatchFlag) {
    switch (LFO2Wave) {
      case 0:
        showCurrentParameterPage("LFO2 Wave", "Sine");
        break;
      case 1:
        showCurrentParameterPage("LFO2 Wave", "Saw");
        break;
      case 2:
        showCurrentParameterPage("LFO2 Wave", "Reverse Saw");
        break;
      case 3:
        showCurrentParameterPage("LFO2 Wave", "Square");
        break;
      case 4:
        showCurrentParameterPage("LFO2 Wave", "Triangle");
        break;
      case 5:
        showCurrentParameterPage("LFO2 Wave", "Pulse");
        break;
      case 6:
        showCurrentParameterPage("LFO2 Wave", "S & H");
        break;
    }
    startParameterDisplay();
  }
  switch (LFO1Wave) {
    case 0:
      LFO2.begin(WAVEFORM_SINE);
      break;
    case 1:
      LFO2.begin(WAVEFORM_BANDLIMIT_SAWTOOTH);
      break;
    case 2:
      LFO2.begin(WAVEFORM_BANDLIMIT_SAWTOOTH_REVERSE);
      break;
    case 3:
      LFO2.begin(WAVEFORM_BANDLIMIT_SQUARE);
      break;
    case 4:
      LFO2.begin(WAVEFORM_TRIANGLE_VARIABLE);
      break;
    case 5:
      LFO2.begin(WAVEFORM_BANDLIMIT_PULSE);
      break;
    case 6:
      LFO2.begin(WAVEFORM_SAMPLE_HOLD);
      break;
  }
}

void startParameterDisplay() {
  updateScreen();

  lastDisplayTriggerTime = millis();
  waitingToUpdate = true;
}

void RotaryEncoderChanged(bool clockwise, int id) {

  Serial.print("Encoder Num ");
  Serial.println(id);

  if (!accelerate) {
    speed = 1;
  } else {
    speed = getEncoderSpeed(id);
  }

  if (!clockwise) {
    speed = -speed;
  }

  switch (id) {

    case 1:
      ampLFODepth = (ampLFODepth + speed);
      ampLFODepth = constrain(ampLFODepth, -127, 127);
      updateampLFODepth();
      break;

    case 8:
      LFO1Rate = (LFO1Rate + speed);
      LFO1Rate = constrain(LFO1Rate, 0, 127);
      updateLFO1Rate();
      break;

    case 9:
      LFO2Rate = (LFO2Rate + speed);
      LFO2Rate = constrain(LFO2Rate, 0, 127);
      updateLFO2Rate();
      break;

    case 11:
      ampAttack = (ampAttack + speed);
      ampAttack = constrain(ampAttack, 0, 127);
      updateampAttack();
      break;

    case 12:
      ampDecay = (ampDecay + speed);
      ampDecay = constrain(ampDecay, 0, 127);
      updateampDecay();
      break;

    case 13:
      ampSustain = (ampSustain + speed);
      ampSustain = constrain(ampSustain, 0, 100);
      updateampSustain();
      break;

    case 15:
      ampRelease = (ampRelease + speed);
      ampRelease = constrain(ampRelease, 0, 127);
      updateampRelease();
      break;

    case 16:
      filterAttack = (filterAttack + speed);
      filterAttack = constrain(filterAttack, 0, 127);
      updatefilterAttack();
      break;

    case 17:
      filterDecay = (filterDecay + speed);
      filterDecay = constrain(filterDecay, 0, 127);
      updatefilterDecay();
      break;

    case 18:
      filterSustain = (filterSustain + speed);
      filterSustain = constrain(filterSustain, 0, 100);
      updatefilterSustain();
      break;

    case 19:
      filterRelease = (filterRelease + speed);
      filterRelease = constrain(filterRelease, 0, 127);
      updatefilterRelease();
      break;

    case 20:
      pitchAttack = (pitchAttack + speed);
      pitchAttack = constrain(pitchAttack, 0, 127);
      updatepitchAttack();
      break;

    case 21:
      filterResonance = (filterResonance + speed);
      filterResonance = constrain(filterResonance, 0, 255);
      updatefilterResonance();
      break;

    case 22:
      filterKeyTrack = (filterKeyTrack + speed);
      filterKeyTrack = constrain(filterKeyTrack, 0, 255);
      updatefilterKeyTrack();
      break;

    case 23:
      noiseLevel = (noiseLevel + speed);
      noiseLevel = constrain(noiseLevel, -127, 127);
      updatenoiseLevel();
      break;

    case 24:
      pitchDecay = (pitchDecay + speed);
      pitchDecay = constrain(pitchDecay, 0, 127);
      updatepitchDecay();
      break;

    case 25:
      pitchSustain = (pitchSustain + speed);
      pitchSustain = constrain(pitchSustain, 0, 100);
      updatepitchSustain();
      break;

    case 26:
      pitchRelease = (pitchRelease + speed);
      pitchRelease = constrain(pitchRelease, 0, 127);
      updatepitchRelease();
      break;

    case 27:
      filterCutoff = (filterCutoff + speed);
      filterCutoff = constrain(filterCutoff, 0, 255);
      updatefilterCutoff();
      break;

    case 28:
      filterEGDepth = (filterEGDepth + speed);
      filterEGDepth = constrain(filterEGDepth, 0, 255);
      updatefilterEGDepth();
      break;

    case 29:
      vcoCFMDepth = (vcoCFMDepth + speed);
      vcoCFMDepth = constrain(vcoCFMDepth, 0, 255);
      updatevcoCFMDepth();
      break;

    case 30:
      vcoBDetune = (vcoBDetune + speed);
      vcoBDetune = constrain(vcoBDetune, 0, 127);
      updatevcoBDetune();
      break;

    case 31:
      vcoCDetune = (vcoCDetune + speed);
      vcoCDetune = constrain(vcoCDetune, 0, 127);
      updatevcoCDetune();
      break;

    case 32:
      filterLFODepth = (filterLFODepth + speed);
      filterLFODepth = constrain(filterLFODepth, -127, 127);
      updatefilterLFODepth();
      break;

    case 33:
      vcoAFMDepth = (vcoAFMDepth + speed);
      vcoAFMDepth = constrain(vcoAFMDepth, 0, 255);
      updatevcoAFMDepth();
      break;

    case 34:
      vcoBFMDepth = (vcoBFMDepth + speed);
      vcoBFMDepth = constrain(vcoBFMDepth, 0, 255);
      updatevcoBFMDepth();
      break;

    case 36:
      vcoALevel = (vcoALevel + speed);
      vcoALevel = constrain(vcoALevel, 0, 255);
      updatevcoALevel();
      break;

    case 37:
      vcoBLevel = (vcoBLevel + speed);
      vcoBLevel = constrain(vcoBLevel, 0, 255);
      updatevcoBLevel();
      break;

    case 38:
      vcoCLevel = (vcoCLevel + speed);
      vcoCLevel = constrain(vcoCLevel, 0, 255);
      updatevcoCLevel();
      break;

    case 39:
      vcoAPW = (vcoAPW + speed);
      vcoAPW = constrain(vcoAPW, 0, 255);
      updatevcoAPW();
      break;

    case 40:
      vcoBPW = (vcoBPW + speed);
      vcoBPW = constrain(vcoBPW, 0, 255);
      updatevcoBPW();
      break;

    case 41:
      vcoCPW = (vcoCPW + speed);
      vcoCPW = constrain(vcoCPW, 0, 255);
      updatevcoCPW();
      break;

    case 42:
      vcoAPWM = (vcoAPWM + speed);
      vcoAPWM = constrain(vcoAPWM, 0, 255);
      updatevcoAPWM();
      break;

    case 43:
      vcoBPWM = (vcoBPWM + speed);
      vcoBPWM = constrain(vcoBPWM, 0, 255);
      updatevcoBPWM();
      break;

    case 44:
      vcoCPWM = (vcoCPWM + speed);
      vcoCPWM = constrain(vcoCPWM, 0, 255);
      updatevcoCPWM();
      break;

    case 45:
      if (!clockwise) {
        vcoAWave--;
      } else {
        vcoAWave++;
      }
      vcoAWave = constrain(vcoAWave, 0, 6);
      updatevcoAWave();
      break;

    case 46:
      if (!clockwise) {
        vcoBWave--;
      } else {
        vcoBWave++;
      }
      vcoBWave = constrain(vcoBWave, 0, 6);
      updatevcoBWave();
      break;

    case 47:
      if (!clockwise) {
        vcoCWave--;
      } else {
        vcoCWave++;
      }
      vcoCWave = (vcoCWave + speed);
      vcoCWave = constrain(vcoCWave, 0, 6);
      updatevcoCWave();
      break;

    case 48:
      if (!clockwise) {
        vcoAInterval--;
      } else {
        vcoAInterval++;
      }
      vcoAInterval = constrain(vcoAInterval, -12, 12);
      updatevcoAInterval();
      break;

    case 49:
      if (!clockwise) {
        vcoBInterval--;
      } else {
        vcoBInterval++;
      }
      vcoBInterval = constrain(vcoBInterval, -12, 12);
      updatevcoBInterval();
      break;

    case 50:
      if (!clockwise) {
        vcoCInterval--;
      } else {
        vcoCInterval++;
      }
      vcoCInterval = constrain(vcoCInterval, -12, 12);
      updatevcoCInterval();
      break;

    case 51:
      XModDepth = (XModDepth + speed);
      XModDepth = constrain(XModDepth, 0, 255);
      updateXModDepth();
      break;

    default:
      break;
  }


  //rotaryEncoderChanged(id, clockwise, speed);
}

int getEncoderSpeed(int id) {
  if (id < 1 || id > numEncoders) return 1;

  unsigned long now = millis();
  unsigned long revolutionTime = now - lastTransition[id];

  int speed = 1;
  if (revolutionTime < 50) {
    speed = 10;
  } else if (revolutionTime < 125) {
    speed = 5;
  } else if (revolutionTime < 250) {
    speed = 2;
  }

  lastTransition[id] = now;
  return speed;
}

void initRotaryEncoders() {
  for (auto &rotaryEncoder : rotaryEncoders) {
    rotaryEncoder.init();
  }
}


void pollAllMCPs() {

  for (int j = 0; j < numMCPs; j++) {
    uint16_t gpioAB = allMCPs[j]->readGPIOAB();
    for (int i = 0; i < numEncoders; i++) {
      if (rotaryEncoders[i].getMCP() == allMCPs[j])
        rotaryEncoders[i].feedInput(gpioAB);
    }

    // for (auto &button : allButtons) {
    //   if (button->getMcp() == allMCPs[j]) {
    //     button->feedInput(gpioAB);
    //   }
    // }
  }
}

int getVoiceNo(int note) {
  voiceToReturn = -1;       //Initialise to 'null'
  earliestTime = millis();  //Initialise to now
  if (note == -1) {
    //NoteOn() - Get the oldest free voice (recent voices may be still on release stage)
    for (int i = 0; i < NO_OF_VOICES; i++) {
      if (voices[i].note == -1) {
        if (voices[i].timeOn < earliestTime) {
          earliestTime = voices[i].timeOn;
          voiceToReturn = i;
        }
      }
    }
    if (voiceToReturn == -1) {
      //No free voices, need to steal oldest sounding voice
      earliestTime = millis();  //Reinitialise
      for (int i = 0; i < NO_OF_VOICES; i++) {
        if (voices[i].timeOn < earliestTime) {
          earliestTime = voices[i].timeOn;
          voiceToReturn = i;
        }
      }
    }
    return voiceToReturn + 1;
  } else {
    //NoteOff() - Get voice number from note
    for (int i = 0; i < NO_OF_VOICES; i++) {
      if (voices[i].note == note) {
        return i + 1;
      }
    }
  }
  //Shouldn't get here, return voice 1
  return 1;
}

void myNoteOn(byte channel, byte note, byte velocity) {

  if (MONO_POLY_1 < 511 && MONO_POLY_2 < 511) {
    detune = 1.000;  //POLYPHONIC mode
    if (note < 0 || note > 127) return;
    switch (getVoiceNo(-1)) {
      case 1:
        voices[0].note = note;
        voices[0].velocity = velocity;
        voices[0].timeOn = millis();
        note1freq = note;
        updateVoice1();
        env1.noteOn();
        filterEnv1.noteOn();
        ampEnv1.noteOn();
        env1on = true;
        voiceOn[0] = true;
        Serial.println("Voice 1 On");
        break;

      case 2:
        voices[1].note = note;
        voices[1].velocity = velocity;
        voices[1].timeOn = millis();
        note2freq = note;
        updateVoice2();
        env2.noteOn();
        filterEnv2.noteOn();
        ampEnv2.noteOn();
        env2on = true;
        voiceOn[1] = true;
        Serial.println("Voice 2 On");
        break;

      case 3:
        voices[2].note = note;
        voices[2].velocity = velocity;
        voices[2].timeOn = millis();
        note3freq = note;
        updateVoice3();
        env3.noteOn();
        filterEnv3.noteOn();
        ampEnv3.noteOn();
        env3on = true;
        voiceOn[2] = true;
        Serial.println("Voice 3 On");
        break;

      case 4:
        voices[3].note = note;
        voices[3].velocity = velocity;
        voices[3].timeOn = millis();
        note4freq = note;
        updateVoice4();
        env4.noteOn();
        filterEnv4.noteOn();
        ampEnv4.noteOn();
        env4on = true;
        voiceOn[3] = true;
        Serial.println("Voice 4 On");
        break;

      case 5:
        voices[4].note = note;
        voices[4].velocity = velocity;
        voices[4].timeOn = millis();
        note5freq = note;
        updateVoice5();
        env5.noteOn();
        filterEnv5.noteOn();
        ampEnv5.noteOn();
        env5on = true;
        voiceOn[4] = true;
        Serial.println("Voice 5 On");
        break;

      case 6:
        voices[5].note = note;
        voices[5].velocity = velocity;
        voices[5].timeOn = millis();
        note6freq = note;
        updateVoice6();
        env6.noteOn();
        filterEnv6.noteOn();
        ampEnv6.noteOn();
        env6on = true;
        voiceOn[5] = true;
        Serial.println("Voice 6 On");
        break;

      case 7:
        voices[6].note = note;
        voices[6].velocity = velocity;
        voices[6].timeOn = millis();
        note7freq = note;
        updateVoice7();
        env7.noteOn();
        filterEnv7.noteOn();
        ampEnv7.noteOn();
        env7on = true;
        voiceOn[6] = true;
        Serial.println("Voice 7 On");
        break;

      case 8:
        voices[7].note = note;
        voices[7].velocity = velocity;
        voices[7].timeOn = millis();
        note8freq = note;
        updateVoice8();
        env8.noteOn();
        filterEnv8.noteOn();
        ampEnv8.noteOn();
        env8on = true;
        voiceOn[7] = true;
        Serial.println("Voice 8 On");
        break;
    }
  }

  if (MONO_POLY_1 > 511 && MONO_POLY_2 < 511) {  //UNISON mode
    detune = olddetune;
    noteMsg = note;

    if (velocity == 0) {
      notes[noteMsg] = false;
    } else {
      notes[noteMsg] = true;
    }

    switch (NP) {
      case 0:
        commandTopNoteUnison();
        break;

      case 1:
        commandBottomNoteUnison();
        break;

      case 2:
        if (notes[noteMsg]) {  // If note is on and using last note priority, add to ordered list
          orderIndx = (orderIndx + 1) % 40;
          noteOrder[orderIndx] = noteMsg;
        }
        commandLastNoteUnison();
        break;
    }
  }

  if (MONO_POLY_1 < 511 && MONO_POLY_2 > 511) {
    detune = 1.000;
    noteMsg = note;

    if (velocity == 0) {
      notes[noteMsg] = false;
    } else {
      notes[noteMsg] = true;
    }

    switch (NP) {
      case 0:
        commandTopNote();
        break;

      case 1:
        commandBottomNote();
        break;

      case 2:
        if (notes[noteMsg]) {  // If note is on and using last note priority, add to ordered list
          orderIndx = (orderIndx + 1) % 40;
          noteOrder[orderIndx] = noteMsg;
        }
        commandLastNote();
        break;
    }
  }
}

void myNoteOff(byte channel, byte note, byte velocity) {

  if (MONO_POLY_1 < 511 && MONO_POLY_2 < 511) {  //POLYPHONIC mode
    detune = 1.000;
    switch (getVoiceNo(note)) {
      case 1:
        env1.noteOff();
        filterEnv1.noteOff();
        ampEnv1.noteOff();
        env1on = false;
        voices[0].note = -1;
        voiceOn[0] = false;
        Serial.println("Voice 1 Off");
        break;

      case 2:
        env2.noteOff();
        filterEnv2.noteOff();
        ampEnv2.noteOff();
        env2on = false;
        voices[1].note = -1;
        voiceOn[1] = false;
        Serial.println("Voice 2 Off");
        break;

      case 3:
        env3.noteOff();
        filterEnv3.noteOff();
        ampEnv3.noteOff();
        env3on = false;
        voices[2].note = -1;
        voiceOn[2] = false;
        Serial.println("Voice 3 Off");
        break;

      case 4:
        env4.noteOff();
        filterEnv4.noteOff();
        ampEnv4.noteOff();
        env4on = false;
        voices[3].note = -1;
        voiceOn[3] = false;
        Serial.println("Voice 4 Off");
        break;

      case 5:
        env5.noteOff();
        filterEnv5.noteOff();
        ampEnv5.noteOff();
        env5on = false;
        voices[4].note = -1;
        voiceOn[4] = false;
        Serial.println("Voice 5 Off");
        break;

      case 6:
        env6.noteOff();
        filterEnv6.noteOff();
        ampEnv6.noteOff();
        env6on = false;
        voices[5].note = -1;
        voiceOn[5] = false;
        Serial.println("Voice 6 Off");
        break;

      case 7:
        env7.noteOff();
        filterEnv7.noteOff();
        ampEnv7.noteOff();
        env7on = false;
        voices[6].note = -1;
        voiceOn[6] = false;
        Serial.println("Voice 7 Off");
        break;

      case 8:
        env8.noteOff();
        filterEnv8.noteOff();
        ampEnv8.noteOff();
        env8on = false;
        voices[7].note = -1;
        voiceOn[7] = false;
        Serial.println("Voice 8 Off");
        break;
    }
  }

  if (MONO_POLY_1 > 511 && MONO_POLY_2 < 511) {  //UNISON
    detune = olddetune;
    noteMsg = note;
    notes[noteMsg] = false;

    switch (NP) {
      case 0:
        commandTopNoteUnison();
        break;

      case 1:
        commandBottomNoteUnison();
        break;

      case 2:
        if (notes[noteMsg]) {  // If note is on and using last note priority, add to ordered list
          orderIndx = (orderIndx + 1) % 40;
          noteOrder[orderIndx] = noteMsg;
        }
        commandLastNoteUnison();
        break;
    }
  }

  if (MONO_POLY_1 < 511 && MONO_POLY_2 > 511) {
    detune = 1.000;
    noteMsg = note;
    notes[noteMsg] = false;

    switch (NP) {
      case 0:
        commandTopNote();
        break;

      case 1:
        commandBottomNote();
        break;

      case 2:
        if (notes[noteMsg]) {  // If note is on and using last note priority, add to ordered list
          orderIndx = (orderIndx + 1) % 40;
          noteOrder[orderIndx] = noteMsg;
        }
        commandLastNote();
        break;
    }
  }
}

int mod(int a, int b) {
  int r = a % b;
  return r < 0 ? r + b : r;
}

void commandTopNote() {
  int topNote = 0;
  bool noteActive = false;

  for (int i = 0; i < 88; i++) {
    if (notes[i]) {
      topNote = i;
      noteActive = true;
    }
  }

  if (noteActive)
    commandNote(topNote);
  else  // All notes are off, turn off gate

    env1.noteOff();
  filterEnv1.noteOff();
  ampEnv1.noteOff();
  env1on = false;
}

void commandBottomNote() {

  int bottomNote = 0;
  bool noteActive = false;

  for (int i = 87; i >= 0; i--) {
    if (notes[i]) {
      bottomNote = i;
      noteActive = true;
    }
  }

  if (noteActive)
    commandNote(bottomNote);
  else  // All notes are off, turn off gate
    env1.noteOff();
  filterEnv1.noteOff();
  ampEnv1.noteOff();
  env1on = false;
}

void commandLastNote() {

  int8_t noteIndx;

  for (int i = 0; i < 40; i++) {
    noteIndx = noteOrder[mod(orderIndx - i, 40)];
    if (notes[noteIndx]) {
      commandNote(noteIndx);
      return;
    }
  }
  env1.noteOff();
  filterEnv1.noteOff();
  ampEnv1.noteOff();
  env1on = false;
}

void commandNote(int note) {

  note1freq = note;
  env1.noteOn();
  filterEnv1.noteOn();
  ampEnv1.noteOn();
  env1on = true;
}

void commandTopNoteUnison() {
  int topNote = 0;
  bool noteActive = false;

  for (int i = 0; i < 88; i++) {
    if (notes[i]) {
      topNote = i;
      noteActive = true;
    }
  }

  if (noteActive)
    commandNoteUnison(topNote);
  else  // All notes are off, turn off gate

    env1.noteOff();
  filterEnv1.noteOff();
  ampEnv1.noteOff();
  env1on = false;

  env2.noteOff();
  filterEnv2.noteOff();
  ampEnv2.noteOff();
  env2on = false;
}

void commandBottomNoteUnison() {

  int bottomNote = 0;
  bool noteActive = false;

  for (int i = 87; i >= 0; i--) {
    if (notes[i]) {
      bottomNote = i;
      noteActive = true;
    }
  }

  if (noteActive)
    commandNoteUnison(bottomNote);
  else  // All notes are off, turn off gate
    env1.noteOff();
  filterEnv1.noteOff();
  ampEnv1.noteOff();
  env1on = false;

  env2.noteOff();
  filterEnv2.noteOff();
  ampEnv2.noteOff();
  env2on = false;
}

void commandLastNoteUnison() {

  int8_t noteIndx;

  for (int i = 0; i < 40; i++) {
    noteIndx = noteOrder[mod(orderIndx - i, 40)];
    if (notes[noteIndx]) {
      commandNoteUnison(noteIndx);
      return;
    }
  }
  env1.noteOff();
  filterEnv1.noteOff();
  ampEnv1.noteOff();
  env1on = false;

  env2.noteOff();
  filterEnv2.noteOff();
  ampEnv2.noteOff();
  env2on = false;
}

void commandNoteUnison(int note) {

  note1freq = note;
  env1.noteOn();
  filterEnv1.noteOn();
  ampEnv1.noteOn();
  env1on = true;

  note2freq = note;
  env2.noteOn();
  filterEnv2.noteOn();
  ampEnv2.noteOn();
  env2on = true;
}

void updateVoice1() {
  //voice 1 frequencies
  dco1A.frequency(noteFreqs[note1freq + vcoAInterval] * octave * bend);
  dco1B.frequency(noteFreqs[note1freq + vcoBInterval] * octave * octaveB * tuneB * bend * bDetune);
  dco1C.frequency(noteFreqs[note1freq + vcoCInterval] * octave * octaveC * tuneC * bend * cDetune);
}

void updateVoice2() {
  dco2A.frequency(noteFreqs[note2freq + vcoAInterval] * octave * bend * detune);
  dco2B.frequency(noteFreqs[note2freq + vcoBInterval] * octave * octaveB * tuneB * bend * bDetune);
  dco2C.frequency(noteFreqs[note2freq + vcoCInterval] * octave * octaveC * tuneC * bend * cDetune);
}

void updateVoice3() {
  dco3A.frequency(noteFreqs[note3freq + vcoAInterval] * octave * bend);
  dco3B.frequency(noteFreqs[note3freq + vcoBInterval] * octave * octaveB * tuneB * bend * bDetune);
  dco3C.frequency(noteFreqs[note3freq + vcoCInterval] * octave * octaveC * tuneC * bend * cDetune);
}

void updateVoice4() {
  dco4A.frequency(noteFreqs[note4freq + vcoAInterval] * octave * bend);
  dco4B.frequency(noteFreqs[note4freq + vcoBInterval] * octave * octaveB * tuneB * bend * bDetune);
  dco4C.frequency(noteFreqs[note4freq + vcoCInterval] * octave * octaveC * tuneC * bend * cDetune);
}

void updateVoice5() {
  dco5A.frequency(noteFreqs[note5freq + vcoAInterval] * octave * bend);
  dco5B.frequency(noteFreqs[note5freq + vcoBInterval] * octave * octaveB * tuneB * bend * bDetune);
  dco5C.frequency(noteFreqs[note5freq + vcoCInterval] * octave * octaveC * tuneC * bend * cDetune);
}

void updateVoice6() {
  dco6A.frequency(noteFreqs[note6freq + vcoAInterval] * octave * bend);
  dco6B.frequency(noteFreqs[note6freq + vcoBInterval] * octave * octaveB * tuneB * bend * bDetune);
  dco6C.frequency(noteFreqs[note6freq + vcoCInterval] * octave * octaveC * tuneC * bend * cDetune);
}

void updateVoice7() {
  dco7A.frequency(noteFreqs[note7freq + vcoAInterval] * octave * bend);
  dco7B.frequency(noteFreqs[note7freq + vcoBInterval] * octave * octaveB * tuneB * bend * bDetune);
  dco7C.frequency(noteFreqs[note7freq + vcoCInterval] * octave * octaveC * tuneC * bend * cDetune);
}

void updateVoice8() {
  dco8A.frequency(noteFreqs[note8freq + vcoAInterval] * octave * bend);
  dco8B.frequency(noteFreqs[note8freq + vcoBInterval] * octave * octaveB * tuneB * bend * bDetune);
  dco8C.frequency(noteFreqs[note8freq] * octave * octaveC * tuneC * bend * cDetune);
}

void recallPatch(int patchNo) {
  allNotesOff();
  if (!updateParams) {
    MIDI.sendProgramChange(patchNo - 1, midiOutCh);
  }
  delay(50);
  recallPatchFlag = true;
  File patchFile = SD.open(String(patchNo).c_str());
  if (!patchFile) {
    Serial.println("File not found");
  } else {
    String data[NO_OF_PARAMS];  //Array of data read in
    recallPatchData(patchFile, data);
    setCurrentPatchData(data);
    patchFile.close();
  }
  recallPatchFlag = false;
}

void allNotesOff() {
}

String getCurrentPatchData() {
  return patchName + "," + String(vcoAWave) + "," + String(vcoBWave) + "," + String(vcoCWave) + "," + String(vcoAPW) + "," + String(vcoBPW) + "," + String(vcoCPW)
         + "," + String(vcoAPWM) + "," + String(vcoBPWM) + "," + String(vcoCPWM) + "," + String(vcoBDetune) + "," + String(vcoCDetune)
         + "," + String(vcoAFMDepth) + "," + String(vcoBFMDepth) + "," + String(vcoCFMDepth) + "," + String(vcoALevel) + "," + String(vcoBLevel) + "," + String(vcoCLevel)
         + "," + String(filterCutoff) + "," + String(filterResonance) + "," + String(filterEGDepth) + "," + String(filterKeyTrack) + "," + String(filterLFODepth)
         + "," + String(pitchAttack) + "," + String(pitchDecay) + "," + String(pitchSustain) + "," + String(pitchRelease)
         + "," + String(filterAttack) + "," + String(filterDecay) + "," + String(filterSustain) + "," + String(filterRelease)
         + "," + String(ampAttack) + "," + String(ampDecay) + "," + String(ampSustain) + "," + String(ampRelease)
         + "," + String(LFO1Rate) + "," + String(LFO1Delay) + "," + String(LFO1Wave) + "," + String(LFO2Rate)
         + "," + String(vcoAInterval) + "," + String(vcoBInterval) + "," + String(vcoCInterval)
         + "," + String(vcoAPWMsource) + "," + String(vcoBPWMsource) + "," + String(vcoCPWMsource) + "," + String(vcoAFMsource) + "," + String(vcoBFMsource) + "," + String(vcoCFMsource)
         + "," + String(ampLFODepth) + "," + String(XModDepth) + "," + String(LFO2Wave) + "," + String(noiseLevel);
}

void setCurrentPatchData(String data[]) {
  patchName = data[0];
  vcoAWave = data[1].toInt();
  vcoBWave = data[2].toInt();
  vcoCWave = data[3].toInt();
  vcoAPW = data[4].toFloat();
  vcoBPW = data[5].toFloat();
  vcoCPW = data[6].toFloat();
  vcoAPWM = data[7].toFloat();
  vcoBPWM = data[8].toFloat();
  vcoCPWM = data[9].toFloat();
  vcoBDetune = data[10].toFloat();
  vcoCDetune = data[11].toFloat();
  vcoAFMDepth = data[12].toFloat();
  vcoBFMDepth = data[13].toFloat();
  vcoCFMDepth = data[14].toFloat();
  vcoALevel = data[15].toFloat();
  vcoBLevel = data[16].toFloat();
  vcoCLevel = data[17].toFloat();
  filterCutoff = data[18].toFloat();
  filterResonance = data[19].toFloat();
  filterEGDepth = data[20].toFloat();
  filterKeyTrack = data[21].toFloat();
  filterLFODepth = data[22].toFloat();
  pitchAttack = data[23].toFloat();
  pitchDecay = data[24].toFloat();
  pitchSustain = data[25].toFloat();
  pitchRelease = data[26].toFloat();
  filterAttack = data[27].toFloat();
  filterDecay = data[28].toFloat();
  filterSustain = data[29].toFloat();
  filterRelease = data[30].toFloat();
  ampAttack = data[31].toFloat();
  ampDecay = data[32].toFloat();
  ampSustain = data[33].toFloat();
  ampRelease = data[34].toFloat();
  LFO1Rate = data[35].toFloat();
  LFO1Delay = data[36].toFloat();
  LFO1Wave = data[37].toInt();
  LFO2Rate = data[38].toFloat();
  vcoAInterval = data[39].toInt();
  vcoBInterval = data[40].toInt();
  vcoCInterval = data[41].toInt();
  vcoAPWMsource = data[42].toInt();
  vcoBPWMsource = data[43].toInt();
  vcoCPWMsource = data[44].toInt();
  vcoAFMsource = data[42].toInt();
  vcoBFMsource = data[43].toInt();
  vcoCFMsource = data[44].toInt();
  ampLFODepth = data[45].toFloat();
  XModDepth = data[46].toFloat();
  LFO2Wave = data[47].toInt();
  noiseLevel = data[48].toFloat();

  //Patchname
  updatePatchname();

  updatevcoAWave();
  updatevcoBWave();
  updatevcoCWave();
  updatevcoAPW();
  updatevcoBPW();
  updatevcoCPW();
  // updatevcoAPWM();
  // updatevcoBPWM();
  // updatevcoCPWM();
  updatevcoBDetune();
  updatevcoCDetune();
  // updatevcoAFMDepth();
  // updatevcoBFMDepth();
  // updatevcoCFMDepth();
  updatevcoALevel();
  updatevcoBLevel();
  updatevcoCLevel();
  updatevcoAInterval();
  updatevcoBInterval();
  updatevcoCInterval();
  updatefilterCutoff();
  updatefilterResonance();
  updatefilterEGDepth();
  updatefilterKeyTrack();
  updatefilterLFODepth();
  updateampLFODepth();
  updatepitchAttack();
  updatepitchDecay();
  updatepitchSustain();
  updatepitchRelease();
  updatefilterAttack();
  updatefilterDecay();
  updatefilterSustain();
  updatefilterRelease();
  updateampAttack();
  updateampDecay();
  updateampSustain();
  updateampRelease();
  updateLFO1Wave();
  updateLFO2Wave();
  updateLFO1Rate();
  //updateLFO1Delay();
  updateLFO2Rate();
  updateXModDepth();
  updatenoiseLevel();

  updatevcoAPWMsource();
  updatevcoBPWMsource();
  updatevcoCPWMsource();
  updatevcoAFMsource();
  updatevcoBFMsource();
  updatevcoCFMsource();

  Serial.print("Set Patch: ");
  Serial.println(patchName);
}

void showSettingsPage() {
  showSettingsPage(settings::current_setting(), settings::current_setting_value(), state);
}

void updatePatchname() {
  showPatchPage(String(patchNo), patchName);
}

void checkSwitches() {

  saveButton.update();
  if (saveButton.held()) {
    switch (state) {
      case PARAMETER:
      case PATCH:
        state = DELETE;
        break;
    }
  } else if (saveButton.numClicks() == 1) {
    switch (state) {
      case PARAMETER:
        if (patches.size() < PATCHES_LIMIT) {
          resetPatchesOrdering();  //Reset order of patches from first patch
          patches.push({ patches.size() + 1, INITPATCHNAME });
          state = SAVE;
        }
        updateScreen();
        break;
      case SAVE:
        //Save as new patch with INITIALPATCH name or overwrite existing keeping name - bypassing patch renaming
        patchName = patches.last().patchName;
        state = PATCH;
        savePatch(String(patches.last().patchNo).c_str(), getCurrentPatchData());
        showPatchPage(patches.last().patchNo, patches.last().patchName);
        patchNo = patches.last().patchNo;
        loadPatches();  //Get rid of pushed patch if it wasn't saved
        setPatchesOrdering(patchNo);
        renamedPatch = "";
        state = PARAMETER;
        updateScreen();
        break;
      case PATCHNAMING:
        if (renamedPatch.length() > 0) patchName = renamedPatch;  //Prevent empty strings
        state = PATCH;
        savePatch(String(patches.last().patchNo).c_str(), getCurrentPatchData());
        showPatchPage(patches.last().patchNo, patchName);
        patchNo = patches.last().patchNo;
        loadPatches();  //Get rid of pushed patch if it wasn't saved
        setPatchesOrdering(patchNo);
        renamedPatch = "";
        state = PARAMETER;
        updateScreen();
        break;
    }
  }

  settingsButton.update();
  if (settingsButton.held()) {
    //If recall held, set current patch to match current hardware state
    //Reinitialise all hardware values to force them to be re-read if different
    state = REINITIALISE;
    reinitialiseToPanel();
    updateScreen();
  } else if (settingsButton.numClicks() == 1) {
    switch (state) {
      case PARAMETER:
        state = SETTINGS;
        showSettingsPage();
        updateScreen();
        break;
      case SETTINGS:
        showSettingsPage();
        updateScreen();
      case SETTINGSVALUE:
        settings::save_current_value();
        state = SETTINGS;
        showSettingsPage();
        updateScreen();
        break;
    }
  }

  backButton.update();
  if (backButton.held()) {
    //If Back button held, Panic - all notes off
    allNotesOff();
    updateScreen();  //Hack
  } else if (backButton.numClicks() == 1) {
    switch (state) {
      case RECALL:
        setPatchesOrdering(patchNo);
        state = PARAMETER;
        updateScreen();
        break;
      case SAVE:
        renamedPatch = "";
        state = PARAMETER;
        loadPatches();  //Remove patch that was to be saved
        setPatchesOrdering(patchNo);
        updateScreen();
        break;
      case PATCHNAMING:
        charIndex = 0;
        renamedPatch = "";
        state = SAVE;
        updateScreen();
        break;
      case DELETE:
        setPatchesOrdering(patchNo);
        state = PARAMETER;
        updateScreen();
        break;
      case SETTINGS:
        state = PARAMETER;
        updateScreen();
        break;
      case SETTINGSVALUE:
        state = SETTINGS;
        showSettingsPage();
        updateScreen();
        break;
    }
  }

  //Encoder switch
  recallButton.update();
  if (recallButton.held()) {
    //If Recall button held, return to current patch setting
    //which clears any changes made
    state = PATCH;
    //Recall the current patch
    patchNo = patches.first().patchNo;
    recallPatch(patchNo);
    state = PARAMETER;
    updateScreen();
  } else if (recallButton.numClicks() == 1) {
    switch (state) {
      case PARAMETER:
        state = RECALL;  //show patch list
        updateScreen();
        break;
      case RECALL:
        state = PATCH;
        //Recall the current patch
        patchNo = patches.first().patchNo;
        recallPatch(patchNo);
        state = PARAMETER;
        updateScreen();
        break;
      case SAVE:
        showRenamingPage(patches.last().patchName);
        patchName = patches.last().patchName;
        state = PATCHNAMING;
        updateScreen();
        break;
      case PATCHNAMING:
        if (renamedPatch.length() < 13) {
          renamedPatch.concat(String(currentCharacter));
          charIndex = 0;
          currentCharacter = CHARACTERS[charIndex];
          showRenamingPage(renamedPatch);
        }
        updateScreen();
        break;
      case DELETE:
        //Don't delete final patch
        if (patches.size() > 1) {
          state = DELETEMSG;
          patchNo = patches.first().patchNo;     //PatchNo to delete from SD card
          patches.shift();                       //Remove patch from circular buffer
          deletePatch(String(patchNo).c_str());  //Delete from SD card
          loadPatches();                         //Repopulate circular buffer to start from lowest Patch No
          renumberPatchesOnSD();
          loadPatches();                      //Repopulate circular buffer again after delete
          patchNo = patches.first().patchNo;  //Go back to 1
          recallPatch(patchNo);               //Load first patch
        }
        state = PARAMETER;
        updateScreen();
        break;
      case SETTINGS:
        state = SETTINGSVALUE;
        showSettingsPage();
        updateScreen();
        break;
      case SETTINGSVALUE:
        settings::save_current_value();
        state = SETTINGS;
        showSettingsPage();
        updateScreen();
        break;
    }
  }
}

void reinitialiseToPanel() {
  // //This sets the current patch to be the same as the current hardware panel state - all the pots
  // //The four button controls stay the same state
  // //This reinialises the previous hardware values to force a re-read
  // muxInput = 0;
  // for (int i = 0; i < MUXCHANNELS; i++) {
  // }
  // patchName = INITPATCHNAME;
  // showPatchPage("Initial", "Panel Settings");
}

void checkEncoder() {
  //Encoder works with relative inc and dec values
  //Detent encoder goes up in 4 steps, hence +/-3

  long encRead = encoder.read();
  if ((encCW && encRead > encPrevious + 3) || (!encCW && encRead < encPrevious - 3)) {
    switch (state) {
      case PARAMETER:
        state = PATCH;
        patches.push(patches.shift());
        patchNo = patches.first().patchNo;
        recallPatch(patchNo);
        state = PARAMETER;
        updateScreen();
        break;
      case RECALL:
        patches.push(patches.shift());
        updateScreen();
        break;
      case SAVE:
        patches.push(patches.shift());
        updateScreen();
        break;
      case PATCHNAMING:
        if (charIndex == TOTALCHARS) charIndex = 0;  //Wrap around
        currentCharacter = CHARACTERS[charIndex++];
        showRenamingPage(renamedPatch + currentCharacter);
        updateScreen();
        break;
      case DELETE:
        patches.push(patches.shift());
        updateScreen();
        break;
      case SETTINGS:
        settings::increment_setting();
        showSettingsPage();
        updateScreen();
        break;
      case SETTINGSVALUE:
        settings::increment_setting_value();
        showSettingsPage();
        updateScreen();
        break;
    }
    encPrevious = encRead;
  } else if ((encCW && encRead < encPrevious - 3) || (!encCW && encRead > encPrevious + 3)) {
    switch (state) {
      case PARAMETER:
        state = PATCH;
        patches.unshift(patches.pop());
        patchNo = patches.first().patchNo;
        recallPatch(patchNo);
        state = PARAMETER;
        updateScreen();
        break;
      case RECALL:
        patches.unshift(patches.pop());
        updateScreen();
        break;
      case SAVE:
        //if (patchNo < 57 ) patchNo = 57;
        patches.unshift(patches.pop());
        updateScreen();
        break;
      case PATCHNAMING:
        if (charIndex == -1)
          charIndex = TOTALCHARS - 1;
        currentCharacter = CHARACTERS[charIndex--];
        showRenamingPage(renamedPatch + currentCharacter);
        updateScreen();
        break;
      case DELETE:
        patches.unshift(patches.pop());
        updateScreen();
        break;
      case SETTINGS:
        settings::decrement_setting();
        showSettingsPage();
        updateScreen();
        break;
      case SETTINGSVALUE:
        settings::decrement_setting_value();
        showSettingsPage();
        updateScreen();
        break;
    }
    encPrevious = encRead;
  }
}

void loop() {

  usbMIDI.read();
  MIDI.read();

  checkEncoder();
  checkSwitches();
  pollAllMCPs();
  octoswitch.update();

  if (pitchDirty && msSincePitchUpdate > 2) {  // ~500 Hz max updates; tweak as you like
    updateVoice1();
    updateVoice2();
    updateVoice3();
    updateVoice4();
    updateVoice5();
    updateVoice6();
    updateVoice7();
    updateVoice8();
    pitchDirty = false;
    msSincePitchUpdate = 0;
  }

  // --- LFO1 (bipolar) ---
  if (qLFO1.available()) {
    int16_t *b = qLFO1.readBuffer();
    g_latestLFO = s16_to_float(b[127]);  // last sample of the block
    qLFO1.freeBuffer();
  }

  // --- LFO1_amp (bipolar) ---
  if (qLFO1_amp.available()) {
    int16_t *b = qLFO1_amp.readBuffer();
    g_latestLFO_amp = s16_to_float(b[127]);  // last sample of the block
    qLFO1_amp.freeBuffer();
  }

  if (qLFO2.available()) {
    int16_t *b = qLFO2.readBuffer();
    g_latestLFO2 = s16_to_float(b[127]);  // last sample of the block
    qLFO2.freeBuffer();
  }

  if (qLFO2_amp.available()) {  // only if you added the amp tap
    int16_t *b = qLFO2_amp.readBuffer();
    g_latestLFO2_amp = s16_to_float(b[127]);  // last sample of the block
    qLFO2_amp.freeBuffer();
  }

  // --- Filter envelopes (unipolar 0..+1) ---
  if (qFilterEnv1.available()) {
    int16_t *b = qFilterEnv1.readBuffer();
    g_latestEnv[1] = max(0.0f, s16_to_float(b[127]));
    qFilterEnv1.freeBuffer();
  }
  if (qFilterEnv2.available()) {
    int16_t *b = qFilterEnv2.readBuffer();
    g_latestEnv[2] = max(0.0f, s16_to_float(b[127]));
    qFilterEnv2.freeBuffer();
  }
  if (qFilterEnv3.available()) {
    int16_t *b = qFilterEnv3.readBuffer();
    g_latestEnv[3] = max(0.0f, s16_to_float(b[127]));
    qFilterEnv3.freeBuffer();
  }
  if (qFilterEnv4.available()) {
    int16_t *b = qFilterEnv4.readBuffer();
    g_latestEnv[4] = max(0.0f, s16_to_float(b[127]));
    qFilterEnv4.freeBuffer();
  }
  if (qFilterEnv5.available()) {
    int16_t *b = qFilterEnv5.readBuffer();
    g_latestEnv[5] = max(0.0f, s16_to_float(b[127]));
    qFilterEnv5.freeBuffer();
  }
  if (qFilterEnv6.available()) {
    int16_t *b = qFilterEnv6.readBuffer();
    g_latestEnv[6] = max(0.0f, s16_to_float(b[127]));
    qFilterEnv6.freeBuffer();
  }
  if (qFilterEnv7.available()) {
    int16_t *b = qFilterEnv7.readBuffer();
    g_latestEnv[7] = max(0.0f, s16_to_float(b[127]));
    qFilterEnv7.freeBuffer();
  }
  if (qFilterEnv8.available()) {
    int16_t *b = qFilterEnv8.readBuffer();
    g_latestEnv[8] = max(0.0f, s16_to_float(b[127]));
    qFilterEnv8.freeBuffer();
  }

  updateFilterDACAll();

  // (You can keep draining qAmpEnvN the same way if you need them elsewhere)
  if (qAmpEnv1.available()) {
    int16_t *b = qAmpEnv1.readBuffer(); /* optional use */
    g_latestAmpEnv[1] = max(0.0f, s16_to_float(b[127]));
    qAmpEnv1.freeBuffer();
  }
  if (qAmpEnv2.available()) {
    int16_t *b = qAmpEnv2.readBuffer(); /* optional use */
    g_latestAmpEnv[2] = max(0.0f, s16_to_float(b[127]));
    qAmpEnv2.freeBuffer();
  }
  if (qAmpEnv3.available()) {
    int16_t *b = qAmpEnv3.readBuffer(); /* optional use */
    g_latestAmpEnv[3] = max(0.0f, s16_to_float(b[127]));
    qAmpEnv3.freeBuffer();
  }
  if (qAmpEnv4.available()) {
    int16_t *b = qAmpEnv4.readBuffer(); /* optional use */
    g_latestAmpEnv[4] = max(0.0f, s16_to_float(b[127]));
    qAmpEnv4.freeBuffer();
  }
  if (qAmpEnv5.available()) {
    int16_t *b = qAmpEnv5.readBuffer(); /* optional use */
    g_latestAmpEnv[5] = max(0.0f, s16_to_float(b[127]));
    qAmpEnv5.freeBuffer();
  }
  if (qAmpEnv6.available()) {
    int16_t *b = qAmpEnv6.readBuffer(); /* optional use */
    g_latestAmpEnv[6] = max(0.0f, s16_to_float(b[127]));
    qAmpEnv6.freeBuffer();
  }
  if (qAmpEnv7.available()) {
    int16_t *b = qAmpEnv7.readBuffer(); /* optional use */
    g_latestAmpEnv[7] = max(0.0f, s16_to_float(b[127]));
    qAmpEnv7.freeBuffer();
  }
  if (qAmpEnv8.available()) {
    int16_t *b = qAmpEnv8.readBuffer(); /* optional use */
    g_latestAmpEnv[8] = max(0.0f, s16_to_float(b[127]));
    qAmpEnv8.freeBuffer();
  }

  if (waitingToUpdate && (millis() - lastDisplayTriggerTime >= displayTimeout)) {
    updateScreen();  // retrigger
    waitingToUpdate = false;
  }

  updateAmpDACAll();
}



/* ============================================================
   NOTES
   - Set per-voice base pitch with dcoX.frequency() per your allocator.
   - Adjust mod depths via modPitchN / modPWNN gains.
   - Raise noiseMix gains if you want shared noise into any voice.
   - Queues: you’re tapping LFO1 and every filter/amp envelope; read and free regularly.
   - TDM mapping: voices 1..8 → channels 0,2,4,6,8,10,12,14 on CS42448.
   ============================================================ */
