//Values below are just for initialising and will be changed when synth is initialised to current panel controls & EEPROM settings
byte midiChannel = MIDI_CHANNEL_OMNI;  //(EEPROM)
byte midiOutCh = 1; 

int patchNo = 1;               //Current patch no
int voiceToReturn = -1;        //Initialise
long earliestTime = millis();  //For voice allocation - initialise to now

uint32_t int_ref_on_flexible_mode = 0b00001001000010100000000000000000;
uint32_t set_a_to_0     = 0b00000010000000000000000000000000;
uint32_t set_a_to_2_5   = 0b00000010000011111111111100000000;               // { 0000 , 0010 , 0000 , 0111111111111111 , 0000 }
uint32_t set_a_to_1_25  = 0b00000010000001111111111100000000; 
uint32_t w = 0;

//Delayed LFO
int numberOfNotes = 0;
int oldnumberOfNotes = 0;
unsigned long previousMillis = 0;
unsigned long interval = 1; //10 seconds
long delaytime  = 0;
int LFODelayGo = 0;
bool LFODelayGoA = false;
bool LFODelayGoB = false;
bool LFODelayGoC = false;
static float g_latestLFO = 0.0f;         // -1..+1
static float g_latestLFO_amp = 0.0f;     // -1..+1
volatile float g_latestLFO2 = 0.0f;      // -1..+1
volatile float g_latestLFO2_amp = 0.0f;  // -1..+1

uint32_t noteAgeCounter = 1;
int nextVoiceRR = 0;
uint8_t voiceNote[9];  // per-voice note index (0..127 or into noteFreqs)

//Unison Detune
byte unidetune = 0;
byte oldunidetune = 0;
byte uniNotes = 0;

float voiceDetune[8] = { 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000 };

// adding encoders
bool rotaryEncoderChanged(int id, bool clockwise, int speed);
#define NUM_ENCODERS 51
unsigned long lastTransition[NUM_ENCODERS + 1];
unsigned long lastDisplayTriggerTime = 0;
bool waitingToUpdate = false;
const unsigned long displayTimeout = 5000;  // e.g. 5 seconds

int MIDIThru = midi::Thru::Off;  //(EEPROM)
String patchName = INITPATCHNAME;
bool encCW = true;  //This is to set the encoder to increment when turned CW - Settings Option
bool announce = true;
byte accelerate = 1;
int speed = 1;
bool updateParams = false;  //(EEPROM)
int old_value = 0;
int old_param_offset = 0;

// ---- UI controls (0..255 unless noted) ----
uint8_t uiCutoff      = 128;  // base cutoff
uint8_t uiKeytrackAmt = 128;  // keytrack depth
uint8_t uiEnvAmt      = 128;  // filter env depth
uint8_t uiLfoAmt      = 0;    // LFO depth to filter

// Dirty flag + lightweight throttle
volatile bool pitchDirty = true;
elapsedMillis msSincePitchUpdate;

float atFMDepth = 0;
float mwFMDepth = 0;
float wheel = 0;
float depth = 0;
float detune = 1.00f;
float olddetune = 1.00f;
float bend = 1.00;
float octave = 1;
float octaveB = 1;
float octaveC = 1;
float tuneB = 1;
float tuneC = 1;

int vcoAWave = 0;
int vcoBWave = 0;
int vcoCWave = 0;

bool vcoATable = false;
bool vcoBTable = false;
bool vcoCTable = false;

int vcoAWaveNumber = 1;
int vcoBWaveNumber = 1;
int vcoCWaveNumber = 1;

int vcoAWaveBank = 1;
int vcoBWaveBank = 1;
int vcoCWaveBank = 1;

int vcoAInterval = 0;
int vcoBInterval = 0;
int vcoCInterval = 0;
int aInterval = 0;
int bInterval = 0;
int cInterval = 0;

float vcoAPW = 0;
float vcoBPW = 0;
float vcoCPW = 0;
float aPW = 0;
float bPW = 0;
float cPW = 0;

float vcoAPWM = 0;
float vcoBPWM = 0;
float vcoCPWM = 0;
float aPWM = 0;
float bPWM = 0;
float cPWM = 0;

float vcoBDetune = 0;
float vcoCDetune = 0;
float bDetune = 1.00;
float cDetune = 1.00;

int vcoAOctave = 1;
int vcoBOctave = 1;
int vcoCOctave = 1;

float vcoAFMDepth = 0;
float vcoBFMDepth = 0;
float vcoCFMDepth = 0;
float aFMDepth = 0;
float bFMDepth = 0;
float cFMDepth = 0;

float vcoALevel = 0;
float vcoBLevel = 0;
float vcoCLevel = 0;
float aLevel = 0;
float bLevel = 0;
float cLevel = 0;

float filterCutoff = 0;
float filterResonance = 0;
float filterEGDepth = 0;
float filterKeyTrack = 0;
float filterLFODepth;
bool filterKeyTrackSW = 0;
int egInvertSW = 0;

float ampLFODepth;
float XModDepth = 0;
float bXModDepth = 0;
float noiseLevel = 0;

float pitchAttack = 0;
float pitchDecay = 0;
float pitchSustain = 0;
float pitchRelease = 0;

float filterAttack = 0;
float filterDecay = 0;
float filterSustain = 0;
float filterRelease = 0;

float ampAttack = 0;
float ampDecay = 0;
float ampSustain = 0;
float ampRelease = 0;

float LFO1Rate = 0;
float LFO1Delay = 0;
float LFO2Rate = 0;
int LFO1Wave = 0;
int LFO2Wave = 0;

float effectPot1 = 0;
float effectPot2 = 0;
float effectPot3 = 0;
float effectsMix = 0;
float volumeLevel = 0;

int MWDepth = 0;
int PBDepth = 0;
int ATDepth = 0;

int filterType = 0;
bool filterPoleSW = 0;

bool filterVelocitySW = 0;
bool ampVelocitySW = 0;

int vcoAPWMsource = 0;
int vcoBPWMsource = 0;
int vcoCPWMsource = 0;

int vcoAFMsource = 0;
int vcoBFMsource = 0;
int vcoCFMsource = 0;
bool multiSW = 0;

int effectNumberSW = 0;
int effectBankSW = 0;

int playModeSW = 0;
int notePrioritySW = 0;

// Not stored

int FMSyncSW = 0;
int PWSyncSW = 0;
int PWMSyncSW = 0;
bool effectsPot3SW = false;
bool pot3ToggleState = false;  // false = go to fast, true = return to stored
int slowpot3 = 5;
int fastpot3 = 250;
bool fast = false;
bool slow = false;
int oldeffectPot3 = -99;

static int lastfilterLFODepth = 0;
static bool filterLFODepthWasToggled = false;

static int lastampLFODepth = 0;
static bool ampLFODepthWasToggled = false;

static int lastnoiseLevel = 0;
static bool noiseLevelWasToggled = false;

static int lastfilterKeyTrack = 0;
static bool filterKeyTrackWasToggled = false;

static int lasteffectsMix = 0;
static bool effectsMixWasToggled = false;

uint16_t filterattackout = 0;
uint16_t filterdecayout = 0;
uint16_t filtersustainout = 0;
uint16_t filterreleaseout = 0;
uint16_t ampattackout = 0;
uint16_t ampreleaseout = 0;
uint16_t ampsustainout = 0;
uint16_t ampdecayout = 0;


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


// ---------- Pointers to your existing per-voice objects ----------
struct VoiceIO {
  AudioSynthWaveformModulated *A;
  AudioSynthWaveformModulated *B;
  AudioSynthWaveformModulated *C;
  AudioEffectEnvelope *env;  // pitch/PWM env (digital)
};

// These MUST match the names from the big graph you pasted earlier
VoiceIO VO[8] = {
  {
    &dco1A,
    &dco1B,
    &dco1C,
    &env1,
  },
  {
    &dco2A,
    &dco2B,
    &dco2C,
    &env2,
  },
  {
    &dco3A,
    &dco3B,
    &dco3C,
    &env3,
  },
  {
    &dco4A,
    &dco4B,
    &dco4C,
    &env4,
  },
  {
    &dco5A,
    &dco5B,
    &dco5C,
    &env5,
  },
  {
    &dco6A,
    &dco6B,
    &dco6C,
    &env6,
  },
  {
    &dco7A,
    &dco7B,
    &dco7C,
    &env7,
  },
  {
    &dco8A,
    &dco8B,
    &dco8C,
    &env8,
  },
};

struct VoiceState {
  int8_t note = -1;  // -1 = free
  uint8_t vel = 0;
  uint32_t age = 0;  // increments each noteOn for simple stealing
} VS[8];

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

///// notes, frequencies, voices /////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////
const float noteFreqs[128] = { 8.176, 8.662, 9.177, 9.723, 10.301, 10.913, 11.562, 12.25, 12.978, 13.75, 14.568, 15.434, 16.352, 17.324, 18.354, 19.445, 20.602, 21.827, 23.125, 24.5, 25.957, 27.5, 29.135, 30.868, 32.703, 34.648, 36.708, 38.891, 41.203, 43.654, 46.249, 48.999, 51.913, 55, 58.27, 61.735, 65.406, 69.296, 73.416, 77.782, 82.407, 87.307, 92.499, 97.999, 103.826, 110, 116.541, 123.471, 130.813, 138.591, 146.832, 155.563, 164.814, 174.614, 184.997, 195.998, 207.652, 220, 233.082, 246.942, 261.626, 277.183, 293.665, 311.127, 329.628, 349.228, 369.994, 391.995, 415.305, 440, 466.164, 493.883, 523.251, 554.365, 587.33, 622.254, 659.255, 698.456, 739.989, 783.991, 830.609, 880, 932.328, 987.767, 1046.502, 1108.731, 1174.659, 1244.508, 1318.51, 1396.913, 1479.978, 1567.982, 1661.219, 1760, 1864.655, 1975.533, 2093.005, 2217.461, 2349.318, 2489.016, 2637.02, 2793.826, 2959.955, 3135.963, 3322.438, 3520, 3729.31, 3951.066, 4186.009, 4434.922, 4698.636, 4978.032, 5274.041, 5587.652, 5919.911, 6271.927, 6644.875, 7040, 7458.62, 7902.133, 8372.018, 8869.844, 9397.273, 9956.063, 10548.08, 11175.3, 11839.82, 12543.85 };

int note1freq;
int note2freq;
int note3freq;
int note4freq;
int note5freq;
int note6freq;
int note7freq;
int note8freq;
//int voices;


//checks if notes are on or not
///////////////////////////////////////////////////////////////////////////////////////////////////////////
bool env1on = false;
bool env2on = false;
bool env3on = false;
bool env4on = false;
bool env5on = false;
bool env6on = false;
bool env7on = false;
bool env8on = false;

struct VoiceAndNote {
  int note;
  int velocity;
  long timeOn;
};

struct VoiceAndNote voices[NO_OF_VOICES] = {
  { -1, -1, 0 },
  { -1, -1, 0 },
  { -1, -1, 0 },
  { -1, -1, 0 },
  { -1, -1, 0 },
  { -1, -1, 0 },
  { -1, -1, 0 },
  { -1, -1, 0 }
};

bool voiceOn[NO_OF_VOICES] = { false, false, false, false, false, false, false, false };

int prevNote = 0;  //Initialised to middle value
bool notes[88] = { 0 }, initial_loop = 1;
int8_t noteOrder[80] = { 0 }, orderIndx = { 0 };
int noteMsg;

const char* str_ptr = nullptr;

//OLED display data table
//PRESET 1st line
const char str01[8][13] PROGMEM = {//str1 is effect name of 1st line
 "Chorus", "Flange", "Tremolo", "Pitch", "Pitch", "No Effect", "Reverb", "Reverb"
};
const char* const name01[] PROGMEM = {
 str01[0], str01[1], str01[2], str01[3], str01[4], str01[5], str01[6], str01[7],
};

//PRESET 2nd line
const char str02[8][14] PROGMEM = {//str1 is effect name of 1st line
 "Reverb    ", "Reverb    ", "Reverb    ", "Shift        ", "Echo       ", "-           ", "Medium  ", "Strong   "
};
const char* const name02[] PROGMEM = {
 str02[0], str02[1], str02[2], str02[3], str02[4], str02[5], str02[6], str02[7],
};

//PRESET param1
const char str03[8][13] PROGMEM = {//str1 is effect name of 1st line
 "Reverb Mix", "Reverb Mix", "Reverb Mix", "Pitch", "Pitch", "-", "Reverb Time", "Reverb Time"
};
const char* const name03[] PROGMEM = {
 str03[0], str03[1], str03[2], str03[3], str03[4], str03[5], str03[6], str03[7],
};

//PRESET param2
const char str04[8][13] PROGMEM = {//str1 is effect name of 1st line
 "Chorus Rate", "Flange Rate", "Trem Rate", "-", "Echo Delay", "-", "HF Filter", "HF Filter"
};
const char* const name04[] PROGMEM = {
 str04[0], str04[1], str04[2], str04[3], str04[4], str04[5], str04[6], str04[7],
};

//PRESET param3
const char str05[8][13] PROGMEM = {//str1 is effect name of 1st line
 "Chorus Mix", "Flange Mix", "Trem Mix", "-", "Echo Mix", "-", "LF Filter", "LF Filter"
};
const char* const name05[] PROGMEM = {
 str05[0], str05[1], str05[2], str05[3], str05[4], str05[5], str05[6], str05[7],
};

//ROM1 1st line
const char str11[8][13] PROGMEM = {//str1 is effect name of 1st line
"Qubit","SuperNova","Modulated","Echo","Shimmer","Sample &","Modulated","Glitch Bit"
};
const char* const name11[] PROGMEM = {
 str11[0], str11[1], str11[2], str11[3], str11[4], str11[5], str11[6], str11[7],
};

//ROM1 2nd line
const char str12[8][14] PROGMEM = {//str1 is effect name of 1st line
"Delay       ","Delay       ","Delay       ","Reverb    ","Reverb    ","Hold        ","Reverb    ","Delay       "
};
const char* const name12[] PROGMEM = {
 str12[0], str12[1], str12[2], str12[3], str12[4], str12[5], str12[6], str12[7],
};

//ROM1 param1
const char str13[8][13] PROGMEM = {//str1 is effect name of 1st line
"Delay Time","Delay Time","Delay Time","Reverb Level","Reverb Dwell","Delay Time","Reverb Dwell","Delay Time 1"

};
const char* const name13[] PROGMEM = {
 str13[0], str13[1], str13[2], str13[3], str13[4], str13[5], str13[6], str13[7],
};

//ROM1 param2
const char str14[8][13] PROGMEM = {//str1 is effect name of 1st line
"Modulation","Filter","Feedback","Delay Time","Pitch","Feedback","Reverb Depth","Delay Time 2"

};
const char* const name14[] PROGMEM = {
 str14[0], str14[1], str14[2], str14[3], str14[4], str14[5], str14[6], str14[7],
};

//ROM1 param3
const char str15[8][13] PROGMEM = {//str1 is effect name of 1st line
"Feedback","Feedback","Modulation","Echo Level","Blend","Modulation","Rate","Feedback"

};
const char* const name15[] PROGMEM = {
 str15[0], str15[1], str15[2], str15[3], str15[4], str15[5], str15[6], str15[7],
};

//ROM2 1st line
const char str21[8][13] PROGMEM = {//str1 is effect name of 1st line
"Daydream","Starfield","Dual Pitch","Triple","Reverse","Wah","Vibrato","Phaser"
};
const char* const name21[] PROGMEM = {
 str21[0], str21[1], str21[2], str21[3], str21[4], str21[5], str21[6], str21[7],
};

//ROM2 2nd line
const char str22[8][14] PROGMEM = {//str1 is effect name of 1st line
"Delay       ","Delay       ","Shift        ","Delay       ","Delay       ","Reverb    ","Reverb    ","Reverb    "
};
const char* const name22[] PROGMEM = {
 str22[0], str22[1], str22[2], str22[3], str22[4], str22[5], str22[6], str22[7],
};

//ROM2 param1
const char str23[8][13] PROGMEM = {//str1 is effect name of 1st line
"Delay Time","Delay Time","Pitch 1","Delay Time 1","Sample Size","Reverb Size","Reverb Size ","Reverb Size"
};
const char* const name23[] PROGMEM = {
 str23[0], str23[1], str23[2], str23[3], str23[4], str23[5], str23[6], str23[7],
};

//ROM2 param2
const char str24[8][13] PROGMEM = {//str1 is effect name of 1st line
"Feedback","Feedback","1-mix-2","Delay Time 2","Feedback","Resonance","Vib Rate","Phase Rate"
};
const char* const name24[] PROGMEM = {
 str24[0], str24[1], str24[2], str24[3], str24[4], str24[5], str24[6], str24[7],
};

//ROM2 param3
const char str25[8][13] PROGMEM = {//str1 is effect name of 1st line
"Filter","Phaser","Pitch 2","Delay Time 3","Delay","Wah","Vibrato","Phaser"
};
const char* const name25[] PROGMEM = {
 str25[0], str25[1], str25[2], str25[3], str25[4], str25[5], str25[6], str25[7],
};

//ROM3 1st line
const char str31[8][13] PROGMEM = {//str1 is effect name of 1st line
"Phaser","Flanger","VP330","Cathedral","Rotor","Ensemble","Leslie","Wah Wah"
};
const char* const name31[] PROGMEM = {
 str31[0], str31[1], str31[2], str31[3], str31[4], str31[5], str31[6], str31[7],
};

//ROM3 2nd line
const char str32[8][14] PROGMEM = {//str1 is effect name of 1st line
"Bass       ","Bass       ","Ensemble","Sound      ","Effect      ","Effect      ","Effect      ","Funk       "
};
const char* const name32[] PROGMEM = {
 str32[0], str32[1], str32[2], str32[3], str32[4], str32[5], str32[6], str32[7],
};

//ROM3 param1
const char str33[8][13] PROGMEM = {//str1 is effect name of 1st line
"Phaser Rate","Flange Rate","Reverb Level","Reverb Level","Reverb Level","Reverb Level","Reverb Level","Reverb Level"
};
const char* const name33[] PROGMEM = {
 str33[0], str33[1], str33[2], str33[3], str33[4], str33[5], str33[6], str33[7],
};

//ROM3 param2
const char str34[8][13] PROGMEM = {//str1 is effect name of 1st line
"Phase Depth","Flange Depth","Cho/Ens Mix","Feedback","Depth","Ens mix","Filter Freq","Filter Q"
};
const char* const name34[] PROGMEM = {
 str34[0], str34[1], str34[2], str34[3], str34[4], str34[5], str34[6], str34[7],
};

//ROM3 param3
const char str35[8][13] PROGMEM = {//str1 is effect name of 1st line
"Feedback","Feedback","Ensemble","Speed","Speed","Treble","Speed","Sensitivity"
};
const char* const name35[] PROGMEM = {
 str35[0], str35[1], str35[2], str35[3], str35[4], str35[5], str35[6], str35[7],
};



