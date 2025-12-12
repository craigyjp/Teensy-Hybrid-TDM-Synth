#define  NO_OF_VOICES 8
constexpr int VOICES = 8;  // you’re indexing 1..8

static const float DETUNE_FINE_RANGE   = 10.0f;  // +/- 10 cents
const char* VERSION = "V1.8";

#define BANKS 63
#define MAX_TABLES_PER_BANK 160
#define TABLE_SIZE 256      // samples per wavetable
#define SAMPLE_BYTES 2      // int16_t

#define NO_OF_PARAMS 92
const char* INITPATCHNAME = "Initial Patch";
#define HOLD_DURATION 1000
#define PATCHES_LIMIT 999
const uint32_t CLICK_DURATION = 250;
const String INITPATCH = "Init Patch,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1";