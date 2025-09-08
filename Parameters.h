//Values below are just for initialising and will be changed when synth is initialised to current panel controls & EEPROM settings
byte midiChannel = MIDI_CHANNEL_OMNI;  //(EEPROM)
byte midiOutCh = 1; 

uint32_t int_ref_on_flexible_mode = 0b00001001000010100000000000000000;
uint32_t set_a_to_0     = 0b00000010000000000000000000000000;
uint32_t set_a_to_2_5   = 0b00000010000011111111111100000000;               // { 0000 , 0010 , 0000 , 0111111111111111 , 0000 }
uint32_t set_a_to_1_25  = 0b00000010000001111111111100000000; 
uint32_t w = 0;

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
bool recallPatchFlag = true;
boolean accelerate = true;
int speed = 1;
boolean updateParams = false;  //(EEPROM)
int bankselect = 0;
int old_value = 0;
int old_param_offset = 0;

// ---- UI controls (0..255 unless noted) ----
uint8_t uiCutoff      = 128;  // base cutoff
uint8_t uiKeytrackAmt = 128;  // keytrack depth
uint8_t uiEnvAmt      = 128;  // filter env depth
uint8_t uiLfoAmt      = 0;    // LFO depth to filter


// Not needed - remove from settings
boolean loadFactory = false;
boolean loadRAM = false;
boolean loadFromDW = false;
boolean ROMType = false;
boolean dataInProgress = false;
int currentSendPatch = 0;
boolean saveCurrent = false;
boolean afterTouch = false;
boolean saveAll = false;

// Dirty flag + lightweight throttle
volatile bool pitchDirty = true;
elapsedMillis msSincePitchUpdate;

int MONO_POLY_1 = 1;
int MONO_POLY_2 = 1;
float detune = 1.00f;
float olddetune = 1.00f;
float bend = 1.00;
float octave = 1;
float octaveB = 1;
float octaveC = 1;
float tuneB = 1;
float tuneC = 1;
int NP = 0;

int vcoAWave = 0;
int vcoBWave = 0;
int vcoCWave = 0;
int aWave = 0;
int bWave = 0;
int cWave = 0;

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
float filterLFODepth = 0;

float ampLFODepth = 0;
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

float MWDepth = 0;
float PBDepth = 0;
float ATDepth = 0;

static int lastfilterLFODepth = 0;
static bool filterLFODepthWasToggled = false;

static int lastampLFODepth = 0;
static bool ampLFODepthWasToggled = false;

int vcoAPWMsource = 0;
int vcoBPWMsource = 0;
int vcoCPWMsource = 0;

int vcoAFMsource = 0;
int vcoBFMsource = 0;
int vcoCFMsource = 0;

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

boolean voiceOn[NO_OF_VOICES] = { false, false, false, false, false, false, false, false };

int prevNote = 0;  //Initialised to middle value
bool notes[88] = { 0 }, initial_loop = 1;
int8_t noteOrder[80] = { 0 }, orderIndx = { 0 };
int noteMsg;