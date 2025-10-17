// This optional setting causes Encoder to use more optimized code,
// It must be defined before Encoder.h is included.
#define ENCODER_OPTIMIZE_INTERRUPTS
#include <Encoder.h>
#include <Bounce.h>
#include "TButton.h"

#include "Rotary.h"
#include "RotaryEncOverMCP.h"


// Pins for MCP23017
#define GPA0 0
#define GPA1 1
#define GPA2 2
#define GPA3 3
#define GPA4 4
#define GPA5 5
#define GPA6 6
#define GPA7 7
#define GPB0 8
#define GPB1 9
#define GPB2 10
#define GPB3 11
#define GPB4 12
#define GPB5 13
#define GPB6 14
#define GPB7 15

void RotaryEncoderChanged (bool clockwise, int id);

//void mainButtonChanged(Button *btn, bool released);

// I2C MCP23017 GPIO expanders

Adafruit_MCP23017 mcp1;
Adafruit_MCP23017 mcp2;
Adafruit_MCP23017 mcp3;
Adafruit_MCP23017 mcp4;
Adafruit_MCP23017 mcp5;
Adafruit_MCP23017 mcp6;
Adafruit_MCP23017 mcp7;
Adafruit_MCP23017 mcp8;

//Array of pointers of all MCPs
Adafruit_MCP23017 *allMCPs[] = {&mcp1, &mcp2, &mcp3, &mcp4, &mcp5, &mcp6, &mcp7, &mcp8};

// // My encoders
// /* Array of all rotary encoders and their pins */
RotaryEncOverMCP rotaryEncoders[] = {
        RotaryEncOverMCP(&mcp1, 0, 1, &RotaryEncoderChanged, 1),
        RotaryEncOverMCP(&mcp1, 2, 3, &RotaryEncoderChanged, 2),
        RotaryEncOverMCP(&mcp1, 4, 5, &RotaryEncoderChanged, 3),
        RotaryEncOverMCP(&mcp1, 8, 9, &RotaryEncoderChanged, 4),
        RotaryEncOverMCP(&mcp1, 10, 11, &RotaryEncoderChanged, 5),
        RotaryEncOverMCP(&mcp1, 12, 13, &RotaryEncoderChanged, 6),
        RotaryEncOverMCP(&mcp1, 6, 14, &RotaryEncoderChanged, 7),

        RotaryEncOverMCP(&mcp2, 0, 1, &RotaryEncoderChanged, 8),
        RotaryEncOverMCP(&mcp2, 2, 3, &RotaryEncoderChanged, 9),
        RotaryEncOverMCP(&mcp2, 4, 5, &RotaryEncoderChanged, 10),
        RotaryEncOverMCP(&mcp2, 8, 9, &RotaryEncoderChanged, 11),
        RotaryEncOverMCP(&mcp2, 10, 11, &RotaryEncoderChanged, 12),
        RotaryEncOverMCP(&mcp2, 12, 13, &RotaryEncoderChanged, 13),
        RotaryEncOverMCP(&mcp2, 6, 14, &RotaryEncoderChanged, 14),

        RotaryEncOverMCP(&mcp3, 0, 1, &RotaryEncoderChanged, 15),
        RotaryEncOverMCP(&mcp3, 2, 3, &RotaryEncoderChanged, 16),
        RotaryEncOverMCP(&mcp3, 4, 5, &RotaryEncoderChanged, 17),
        RotaryEncOverMCP(&mcp3, 8, 9, &RotaryEncoderChanged, 18),
        RotaryEncOverMCP(&mcp3, 10, 11, &RotaryEncoderChanged, 19),
        RotaryEncOverMCP(&mcp3, 12, 13, &RotaryEncoderChanged, 20),

        RotaryEncOverMCP(&mcp4, 0, 1, &RotaryEncoderChanged, 21),
        RotaryEncOverMCP(&mcp4, 2, 3, &RotaryEncoderChanged, 22),
        RotaryEncOverMCP(&mcp4, 4, 5, &RotaryEncoderChanged, 23),
        RotaryEncOverMCP(&mcp4, 8, 9, &RotaryEncoderChanged, 24),
        RotaryEncOverMCP(&mcp4, 10, 11, &RotaryEncoderChanged, 25),
        RotaryEncOverMCP(&mcp4, 12, 13, &RotaryEncoderChanged, 26),

        RotaryEncOverMCP(&mcp5, 0, 1, &RotaryEncoderChanged, 27),
        RotaryEncOverMCP(&mcp5, 2, 3, &RotaryEncoderChanged, 28),
        RotaryEncOverMCP(&mcp5, 4, 5, &RotaryEncoderChanged, 29),
        RotaryEncOverMCP(&mcp5, 8, 9, &RotaryEncoderChanged, 30),
        RotaryEncOverMCP(&mcp5, 10, 11, &RotaryEncoderChanged, 31),
        RotaryEncOverMCP(&mcp5, 12, 13, &RotaryEncoderChanged, 32),

        RotaryEncOverMCP(&mcp6, 0, 1, &RotaryEncoderChanged, 33),
        RotaryEncOverMCP(&mcp6, 2, 3, &RotaryEncoderChanged, 34),
        RotaryEncOverMCP(&mcp6, 4, 5, &RotaryEncoderChanged, 35),
        RotaryEncOverMCP(&mcp6, 8, 9, &RotaryEncoderChanged, 36),
        RotaryEncOverMCP(&mcp6, 10, 11, &RotaryEncoderChanged, 37),
        RotaryEncOverMCP(&mcp6, 12, 13, &RotaryEncoderChanged, 38),

        RotaryEncOverMCP(&mcp7, 0, 1, &RotaryEncoderChanged, 39),
        RotaryEncOverMCP(&mcp7, 2, 3, &RotaryEncoderChanged, 40),
        RotaryEncOverMCP(&mcp7, 4, 5, &RotaryEncoderChanged, 41),
        RotaryEncOverMCP(&mcp7, 8, 9, &RotaryEncoderChanged, 42),
        RotaryEncOverMCP(&mcp7, 10, 11, &RotaryEncoderChanged, 43),
        RotaryEncOverMCP(&mcp7, 12, 13, &RotaryEncoderChanged, 44),

        RotaryEncOverMCP(&mcp8, 0, 1, &RotaryEncoderChanged, 45),
        RotaryEncOverMCP(&mcp8, 2, 3, &RotaryEncoderChanged, 46),
        RotaryEncOverMCP(&mcp8, 4, 5, &RotaryEncoderChanged, 47),
        RotaryEncOverMCP(&mcp8, 8, 9, &RotaryEncoderChanged, 48),
        RotaryEncOverMCP(&mcp8, 10, 11, &RotaryEncoderChanged, 49),
        RotaryEncOverMCP(&mcp8, 12, 13, &RotaryEncoderChanged, 50),
        RotaryEncOverMCP(&mcp3, 6, 14, &RotaryEncoderChanged, 51),
};


// after your rotaryEncoders[] definition
constexpr size_t NUM_MCP = sizeof(allMCPs) / sizeof(allMCPs[0]);
constexpr int numMCPs = (int)(sizeof(allMCPs) / sizeof(*allMCPs));
constexpr int numEncoders = (int)(sizeof(rotaryEncoders) / sizeof(*rotaryEncoders));

// an array of vectors to hold pointers to the encoders on each MCP
std::vector<RotaryEncOverMCP*> encByMCP[NUM_MCP];

// // GP1
// #define osc1_PW_A 0
// #define osc1_PW_B 1
// #define osc1_PWM_A 2
// #define osc1_PWM_B 3
// #define osc1_level_A 4
// #define osc1_level_B 5
// #define OSC1_OCTAVE_LED_RED 6
#define NOTE_PRIORITY_GREEN 7
// #define OSC1_OCTAVE 8
// #define OSC1_WAVE 9
// #define OSC1_SUB 10
// #define osc2_detune_A 11
// #define osc2_detune_B 12
// #define OSC1_SUB_LED 13
// #define OSC1_WAVE_LED_RED 14
#define NOTE_PRIORITY_RED 15

// // GP2
// #define osc2_freq_A 0
// #define osc2_freq_B 1
// #define osc2_eg_depth_A 2
// #define osc2_eg_depth_B 3
// #define osc2_level_A 4
// #define osc2_level_B 5
// #define OSC2_WAVE_LED_RED 6
#define PLAY_MODE_GREEN 7
// #define OSC2_WAVE 8
// #define OSC2_XMOD 9
// #define OSC2_EG_SELECT 10
// #define vcf_cutoff_A 11
// #define vcf_cutoff_B 12
// #define OSC2_XMOD_LED_RED 13
// #define OSC2_XMOD_LED_GREEN 14
#define PLAY_MODE_RED 15

// // GP3
// #define vcf_res_A 0
// #define vcf_res_B 1
// #define vcf_eg_depth_A 2
// #define vcf_eg_depth_B 3
// #define vcf_key_follow_A 4
// #define vcf_key_follow_B 5
// // 6 unused
#define FILTER_VELOCITY_RED 7
// #define vcf_hpf_A 8
// #define vcf_hpf_B 9
// #define lfo1_depth_A 10
// #define lfo1_depth_B 11
// #define lfo2_depth_A 12
// #define lfo2_depth_B 13
// // 14 unused
#define EG_INVERT_LED 15

// // GP4
// #define eg1_attack_A 0
// #define eg1_attack_B 1
// #define eg1_decay_A 2
// #define eg1_decay_B 3
// #define LFO1_WAVE 4
// #define LFO2_WAVE 5
#define C_OCTAVE_GREEN 6
#define C_OCTAVE_RED 7
// #define eg2_attack_A 8
// #define eg2_attack_B 9
// #define eg2_decay_A 10
// #define eg2_decay_B 11
// #define LFO3_WAVE 12
// // 13 unused
#define FILTER_POLE_RED 14
#define KEYTRACK_RED 15

// // GP5
// #define vcf_key_velocity_A 0
// #define vcf_key_velocity_B 1
// #define lfo3_depth_A 2
// #define lfo3_depth_B 3
// // 4 unused
// // 5 unused
#define A_OCTAVE_GREEN 6
#define A_OCTAVE_RED 7
// #define eg1_key_follow_A 8
// #define eg1_key_follow_B 9
// #define eg2_key_follow_A 10
// #define eg2_key_follow_B 11
// #define EG_SELECT 12
// #define LFO_SELECT 13
#define B_OCTAVE_GREEN 14
#define B_OCTAVE_RED 15

// // GP6
// #define vca_key_velocity_A 0
// #define vca_key_velocity_B 1
// #define vca_level_A 2
// #define vca_level_B 3
// #define eg1_sus_A 4
// #define eg1_sus_B 5
#define FM_C_GREEN 6
#define FM_C_RED 7
// #define eg1_rel_A 8
// #define eg1_rel_B 9
// #define eg2_sus_A 10
// #define eg2_sus_B 11
// #define eg2_rel_A 12
// #define eg2_rel_B 13
#define FM_B_GREEN 14
#define FM_B_RED 15

// // GP7
// #define lfo1_speed_A 0
// #define lfo1_speed_B 1
// #define lfo2_speed_A 2
// #define lfo2_speed_B 3
// #define lfo3_speed_A 4
// #define lfo3_speed_B 5
#define FM_A_GREEN 6
#define FM_A_RED 7
// #define lfo1_delay_A 8
// #define lfo1_delay_B 9
// #define lfo2_delay_A 10
// #define lfo2_delay_B 11
// #define lfo3_delay_A 12
// #define lfo3_delay_B 13
#define PWM_C_GREEN 14
#define PWM_C_RED 15

// // GP8
// #define lfo1_speed_A 0
// #define lfo1_speed_B 1
// #define lfo2_speed_A 2
// #define lfo2_speed_B 3
// #define lfo3_speed_A 4
// #define lfo3_speed_B 5
#define PWM_B_RED 6
#define PWM_B_GREEN 7
// #define lfo1_delay_A 8
// #define lfo1_delay_B 9
// #define lfo2_delay_A 10
// #define lfo2_delay_B 11
// #define lfo3_delay_A 12
// #define lfo3_delay_B 13
#define PWM_A_RED 14
#define PWM_A_GREEN 15

// 74HC165 buttons

#define LFO1_WAVE_SW 0
#define PLAYMODE_SW 1
#define PRIORITY_SW 2
#define EFFECT_BANK_SW 3
#define EFFECT_SW 4
#define MULTI_SW 5
#define FILTER_POLE_SW 6
#define FILTER_TYPE_SW 7

#define OSCC_OCT_SW 8
#define OSCB_OCT_SW 9
#define OSCA_OCT_SW 10
#define OSCC_FM_SW 11
#define OSCB_FM_SW 12
#define OSCA_FM_SW 13
#define OSCC_PWM_SW 14
#define OSCB_PWM_SW 15

#define OSCA_PWM_SW 16
#define KEYTRACK_SW 17
#define LFO2_WAVE_SW 18
#define FILTER_LFO_DEPTH_SW 19
#define AMP_LFO_DEPTH_SW 20
#define EG_INVERT_SW 21
#define FILTER_VELOCITY_SW 22
#define FM_SYNC_SW 23

#define WAVE_TABLE_A_SW 24
#define PW_SYNC_SW 25
#define PWM_SYNC_SW 26
#define AMP_VELOCITY_SW 27
#define NOISE_ZERO_DEPTH_SW 28
#define KEYTRACK_ZERO_DEPTH_SW 29
#define WAVE_TABLE_B_SW 30
#define WAVE_TABLE_C_SW 31

#define EFFECTS_ZERO_DEPTH_SW 32
#define SPARE_33_SW 33
#define SPARE_34_SW 34
#define SPARE_35_SW 35


// 74HC595 OUTPUTS
// 3.3V outputs
#define EFFECT_0 0
#define EFFECT_1 1
#define EFFECT_2 2
#define EFFECT_INTERNAL 3
#define EFFECT_BANK_1 4
#define EFFECT_BANK_2 5
#define EFFECT_BANK_3 6
#define FILTER_TYPE_OUT 7

// 5V outputs
#define FILTER_A 8
#define FILTER_B 9
#define FILTER_C 10
#define FILTER_POLE 11
#define EG_INVERT 12
#define FILTER_VELOCITY_OUT 13
// 3.3v outputs
#define MULTI_LED_RED 14
#define AMP_VELOCITY_RED 15

// 3.3V outputs
#define GATE_OUT_1 16
#define GATE_OUT_2 17
#define GATE_OUT_3 18
#define GATE_OUT_4 19
#define GATE_OUT_5 20
#define GATE_OUT_6 21
#define GATE_OUT_7 22
#define GATE_OUT_8 23

//Teensy 4.1 Pins

#define RECALL_SW 29
#define SAVE_SW 30
#define SETTINGS_SW 31
#define BACK_SW 32

#define DAC_FILTER 10
#define DAC_ADSR 28
#define DAC_GLOBAL 6
#define DAC_LDAC 5
#define DAC_VELOCITY 9
#define AMP_VELOCITY_OUT 25

#define ENCODER_PINA 3
#define ENCODER_PINB 4

#define DEBOUNCE 30

static long encPrevious = 0;

//These are pushbuttons and require debouncing

TButton saveButton{ SAVE_SW, LOW, HOLD_DURATION, DEBOUNCE, CLICK_DURATION };
TButton settingsButton{ SETTINGS_SW, LOW, HOLD_DURATION, DEBOUNCE, CLICK_DURATION };
TButton backButton{ BACK_SW, LOW, HOLD_DURATION, DEBOUNCE, CLICK_DURATION };
TButton recallButton{ RECALL_SW, LOW, HOLD_DURATION, DEBOUNCE, CLICK_DURATION }; // on encoder

Encoder encoder(ENCODER_PINB, ENCODER_PINA);  //This often needs the pins swapping depending on the encoder

void setupHardware() {

  //Switches
  pinMode(RECALL_SW, INPUT_PULLUP);  //On encoder
  pinMode(SAVE_SW, INPUT_PULLUP);
  pinMode(SETTINGS_SW, INPUT_PULLUP);
  pinMode(BACK_SW, INPUT_PULLUP);

  pinMode(DAC_FILTER, OUTPUT);
  pinMode(DAC_ADSR, OUTPUT);
  pinMode(DAC_GLOBAL, OUTPUT);
  pinMode(DAC_LDAC, OUTPUT);
  pinMode(DAC_VELOCITY, OUTPUT);
  pinMode(AMP_VELOCITY_OUT, OUTPUT);

  digitalWrite(DAC_FILTER, HIGH);
  digitalWrite(DAC_ADSR, HIGH);
  digitalWrite(DAC_GLOBAL, HIGH);
  digitalWrite(DAC_VELOCITY, HIGH);
  delay(100);
  digitalWrite(DAC_LDAC, HIGH);
  digitalWrite(AMP_VELOCITY_OUT, LOW);
}
