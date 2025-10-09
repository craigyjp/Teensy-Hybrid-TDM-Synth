#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioSynthWaveformDc     dc_one;         //xy=72.14279174804688,479.0000219345093
AudioSynthNoisePink      noisePink;      //xy=75.8570785522461,406.42858123779297
AudioSynthWaveformDc     dc_neg1;        //xy=75.28564834594727,558.1428813934326
AudioSynthNoiseWhite     noiseWhite;     //xy=80.14279556274414,343.28573417663574
AudioSynthWaveform       LFO2;           //xy=81.28564453125,227.85714721679688
AudioSynthWaveform       LFO1;           //xy=84.14279174804688,78.42860412597656
AudioSynthWaveformDc     dc_pwmAbias;            //xy=87.5,716
AudioSynthWaveformDc     dc_pwmBbias; //xy=91.5,778
AudioSynthWaveformDc     dc_pwmCbias; //xy=91.5,832
AudioRecordQueue         qLFO1;          //xy=307.28568267822266,75.8571720123291
AudioRecordQueue         qLFO1_amp;   // new: dedicated to AMP path
AudioRecordQueue         qLFO2;          //xy=307.28568267822266,75.8571720123291
AudioRecordQueue         qLFO2_amp;   // new: dedicated to AMP path
AudioMixer4              noiseMix;       //xy=464.1427917480469,406.4285888671875

// --- Voice 1 ---
AudioEffectEnvelope      env1;
AudioEffectMultiply      invEnv1;
AudioMixer4              pitchAmtA1, pitchAmtB1, pitchAmtC1;
AudioMixer4              pwmAmtA1, pwmAmtB1, pwmAmtC1;
AudioSynthWaveformModulated dco1A, dco1B, dco1C;
AudioMixer4              voiceMix1;


// --- Voice 2 ---
AudioEffectEnvelope      env2;
AudioEffectMultiply      invEnv2;
AudioMixer4              pitchAmtA2, pitchAmtB2, pitchAmtC2;
AudioMixer4              pwmAmtA2,   pwmAmtB2,   pwmAmtC2;
AudioSynthWaveformModulated dco2A, dco2B, dco2C;
AudioMixer4              voiceMix2;


// --- Voice 3 ---
AudioEffectEnvelope      env3;
AudioEffectMultiply      invEnv3;
AudioMixer4              pitchAmtA3, pitchAmtB3, pitchAmtC3;
AudioMixer4              pwmAmtA3,   pwmAmtB3,   pwmAmtC3;
AudioSynthWaveformModulated dco3A, dco3B, dco3C;
AudioMixer4              voiceMix3;


// --- Voice 4 ---
AudioEffectEnvelope      env4;
AudioEffectMultiply      invEnv4;
AudioMixer4              pitchAmtA4, pitchAmtB4, pitchAmtC4;
AudioMixer4              pwmAmtA4,   pwmAmtB4,   pwmAmtC4;
AudioSynthWaveformModulated dco4A, dco4B, dco4C;
AudioMixer4              voiceMix4;


// --- Voice 5 ---
AudioEffectEnvelope      env5;
AudioEffectMultiply      invEnv5;
AudioMixer4              pitchAmtA5, pitchAmtB5, pitchAmtC5;
AudioMixer4              pwmAmtA5, pwmAmtB5, pwmAmtC5;
AudioSynthWaveformModulated dco5A, dco5B, dco5C;
AudioMixer4              voiceMix5;


// --- Voice 6 ---
AudioEffectEnvelope      env6;
AudioEffectMultiply      invEnv6;
AudioMixer4              pitchAmtA6, pitchAmtB6, pitchAmtC6;
AudioMixer4              pwmAmtA6,   pwmAmtB6,   pwmAmtC6;
AudioSynthWaveformModulated dco6A, dco6B, dco6C;
AudioMixer4              voiceMix6;


// --- Voice 7 ---
AudioEffectEnvelope      env7;
AudioEffectMultiply      invEnv7;
AudioMixer4              pitchAmtA7, pitchAmtB7, pitchAmtC7;
AudioMixer4              pwmAmtA7,   pwmAmtB7,   pwmAmtC7;
AudioSynthWaveformModulated dco7A, dco7B, dco7C;
AudioMixer4              voiceMix7;


// --- Voice 8 ---
AudioEffectEnvelope      env8;
AudioEffectMultiply      invEnv8;
AudioMixer4              pitchAmtA8, pitchAmtB8, pitchAmtC8;
AudioMixer4              pwmAmtA8,   pwmAmtB8,   pwmAmtC8;
AudioSynthWaveformModulated dco8A, dco8B, dco8C;
AudioMixer4              voiceMix8;


AudioOutputTDM           tdmOut;         //xy=1536,183


AudioConnection          patchCord1(dc_one, env1);

AudioConnection          patchCord4(noisePink, 0, noiseMix, 1);
AudioConnection          patchCord5(dc_neg1, 0, invEnv1, 1);
AudioConnection          patchCord6(noiseWhite, 0, noiseMix, 0);
AudioConnection          patchCord7(LFO2, 0, pwmAmtA1, 0);
AudioConnection          patchCord8(LFO2, 0, pwmAmtB1, 0);
AudioConnection          patchCord9(LFO2, 0, pwmAmtC1, 0);
AudioConnection          pcLFO1(LFO1, qLFO1);
AudioConnection          pcLFO1_amp(LFO1, qLFO1_amp);
AudioConnection          pcLFO2(LFO2, qLFO2);
AudioConnection          pcLFO2_amp(LFO2, qLFO2_amp);
AudioConnection          patchCord11(LFO1, 0, pitchAmtA1, 0);
AudioConnection          patchCord12(LFO1, 0, pitchAmtB1, 0);
AudioConnection          patchCord13(LFO1, 0, pitchAmtC1, 0);
AudioConnection          patchCord14(dc_pwmAbias, 0, pwmAmtA1, 3);
AudioConnection          patchCord15(dc_pwmBbias, 0, pwmAmtB1, 3);
AudioConnection          patchCord16(dc_pwmCbias, 0, pwmAmtC1, 3);
AudioConnection          patchCord17(env1, 0, invEnv1, 0);
AudioConnection          patchCord18(env1, 0, pitchAmtA1, 1);
AudioConnection          patchCord19(env1, 0, pitchAmtB1, 1);
AudioConnection          patchCord20(env1, 0, pitchAmtC1, 1);
AudioConnection          patchCord21(env1, 0, pwmAmtA1, 1);
AudioConnection          patchCord22(env1, 0, pwmAmtB1, 1);
AudioConnection          patchCord23(env1, 0, pwmAmtC1, 1);
AudioConnection          patchCord24(invEnv1, 0, pitchAmtA1, 2);
AudioConnection          patchCord25(invEnv1, 0, pwmAmtA1, 2);
AudioConnection          patchCord26(invEnv1, 0, pwmAmtB1, 2);
AudioConnection          patchCord27(invEnv1, 0, pwmAmtC1, 2);
AudioConnection          patchCord28(invEnv1, 0, pitchAmtB1, 2);
AudioConnection          patchCord29(invEnv1, 0, pitchAmtC1, 2);
AudioConnection          patchCord30(noiseMix, 0, voiceMix1, 3);
AudioConnection          patchCord31(pitchAmtA1, 0, dco1A, 0);
AudioConnection          patchCord32(pitchAmtB1, 0, dco1B, 0);
AudioConnection          patchCord33(pwmAmtB1, 0, dco1B, 1);
AudioConnection          patchCord34(pwmAmtA1, 0, dco1A, 1);
AudioConnection          patchCord35(pitchAmtC1, 0, dco1C, 0);
AudioConnection          patchCord36(pwmAmtC1, 0, dco1C, 1);
AudioConnection          patchCord37(dco1B, 0, voiceMix1, 1);
AudioConnection          patchCord38(dco1A, 0, voiceMix1, 0);
AudioConnection          patchCord39(dco1C, 0, voiceMix1, 2);
AudioConnection          patchCord40(voiceMix1, 0, tdmOut, 0);
AudioConnection          patchCord43(dco1B, 0, pitchAmtA1, 3);

// ========= COMMON SOURCES INTO VOICE 2 =========
// Gate the envelopes
AudioConnection pc200(dc_one, env2);

// Build inverted Env2
AudioConnection pc203(dc_neg1, 0, invEnv2, 1);
AudioConnection pc204(env2,    0, invEnv2, 0);

// Pitch selectors (LFO1 / Env2 / -Env2)
AudioConnection pc205(LFO1, 0, pitchAmtA2, 0);
AudioConnection pc206(env2, 0, pitchAmtA2, 1);
AudioConnection pc207(invEnv2, 0, pitchAmtA2, 2);

AudioConnection pc208(LFO1, 0, pitchAmtB2, 0);
AudioConnection pc209(env2, 0, pitchAmtB2, 1);
AudioConnection pc210(invEnv2, 0, pitchAmtB2, 2);

AudioConnection pc211(LFO1, 0, pitchAmtC2, 0);
AudioConnection pc212(env2, 0, pitchAmtC2, 1);
AudioConnection pc213(invEnv2, 0, pitchAmtC2, 2);

// PWM selectors (LFO2 / Env2 / -Env2 / DC-bias per osc)
AudioConnection pc214(LFO2, 0, pwmAmtA2, 0);
AudioConnection pc215(env2, 0, pwmAmtA2, 1);
AudioConnection pc216(invEnv2, 0, pwmAmtA2, 2);
AudioConnection pc217(dc_pwmAbias, 0, pwmAmtA2, 3);

AudioConnection pc218(LFO2, 0, pwmAmtB2, 0);
AudioConnection pc219(env2, 0, pwmAmtB2, 1);
AudioConnection pc220(invEnv2, 0, pwmAmtB2, 2);
AudioConnection pc221(dc_pwmBbias, 0, pwmAmtB2, 3);

AudioConnection pc222(LFO2, 0, pwmAmtC2, 0);
AudioConnection pc223(env2, 0, pwmAmtC2, 1);
AudioConnection pc224(invEnv2, 0, pwmAmtC2, 2);
AudioConnection pc225(dc_pwmCbias, 0, pwmAmtC2, 3);

// Voice 2 DCO routing
AudioConnection pc226(pitchAmtA2, 0, dco2A, 0);
AudioConnection pc227(pwmAmtA2,   0, dco2A, 1);
AudioConnection pc228(pitchAmtB2, 0, dco2B, 0);
AudioConnection pc229(pwmAmtB2,   0, dco2B, 1);
AudioConnection pc230(pitchAmtC2, 0, dco2C, 0);
AudioConnection pc231(pwmAmtC2,   0, dco2C, 1);

// Mix A/B/C plus noise (noiseMix -> ch3 already exists; connect here)
AudioConnection pc232(dco2A, 0, voiceMix2, 0);
AudioConnection pc233(dco2B, 0, voiceMix2, 1);
AudioConnection pc234(dco2C, 0, voiceMix2, 2);
AudioConnection pc235(noiseMix, 0, voiceMix2, 3);

// Send voice 2 to TDM (usually slot 2)
AudioConnection pc236(voiceMix2, 0, tdmOut, 2);

// (optional) tap envelopes to queues
AudioConnection pc239(dco2B, 0, pitchAmtA2, 3);


// ========= VOICE 3 (same pattern with env3, invEnv3, pitchAmt?3, pwmAmt?3, dco3?, voiceMix3) =========
AudioConnection pc300(dc_one, env3);
AudioConnection pc303(dc_neg1, 0, invEnv3, 1);
AudioConnection pc304(env3,    0, invEnv3, 0);

AudioConnection pc305(LFO1, 0, pitchAmtA3, 0);
AudioConnection pc306(env3, 0, pitchAmtA3, 1);
AudioConnection pc307(invEnv3, 0, pitchAmtA3, 2);
AudioConnection pc308(LFO1, 0, pitchAmtB3, 0);
AudioConnection pc309(env3, 0, pitchAmtB3, 1);
AudioConnection pc310(invEnv3, 0, pitchAmtB3, 2);
AudioConnection pc311(LFO1, 0, pitchAmtC3, 0);
AudioConnection pc312(env3, 0, pitchAmtC3, 1);
AudioConnection pc313(invEnv3, 0, pitchAmtC3, 2);

AudioConnection pc314(LFO2, 0, pwmAmtA3, 0);
AudioConnection pc315(env3, 0, pwmAmtA3, 1);
AudioConnection pc316(invEnv3, 0, pwmAmtA3, 2);
AudioConnection pc317(dc_pwmAbias, 0, pwmAmtA3, 3);
AudioConnection pc318(LFO2, 0, pwmAmtB3, 0);
AudioConnection pc319(env3, 0, pwmAmtB3, 1);
AudioConnection pc320(invEnv3, 0, pwmAmtB3, 2);
AudioConnection pc321(dc_pwmBbias, 0, pwmAmtB3, 3);
AudioConnection pc322(LFO2, 0, pwmAmtC3, 0);
AudioConnection pc323(env3, 0, pwmAmtC3, 1);
AudioConnection pc324(invEnv3, 0, pwmAmtC3, 2);
AudioConnection pc325(dc_pwmCbias, 0, pwmAmtC3, 3);

AudioConnection pc326(pitchAmtA3, 0, dco3A, 0);
AudioConnection pc327(pwmAmtA3,   0, dco3A, 1);
AudioConnection pc328(pitchAmtB3, 0, dco3B, 0);
AudioConnection pc329(pwmAmtB3,   0, dco3B, 1);
AudioConnection pc330(pitchAmtC3, 0, dco3C, 0);
AudioConnection pc331(pwmAmtC3,   0, dco3C, 1);
AudioConnection pc332(dco3A, 0, voiceMix3, 0);
AudioConnection pc333(dco3B, 0, voiceMix3, 1);
AudioConnection pc334(dco3C, 0, voiceMix3, 2);
AudioConnection pc335(noiseMix, 0, voiceMix3, 3);
AudioConnection pc336(voiceMix3, 0, tdmOut, 4);
AudioConnection pc339(dco3B, 0, pitchAmtA3, 3);

// ========= VOICE 4 (same pattern) =========
AudioConnection pc400(dc_one, env4);
AudioConnection pc403(dc_neg1, 0, invEnv4, 1);
AudioConnection pc404(env4,    0, invEnv4, 0);

AudioConnection pc405(LFO1, 0, pitchAmtA4, 0);
AudioConnection pc406(env4, 0, pitchAmtA4, 1);
AudioConnection pc407(invEnv4, 0, pitchAmtA4, 2);
AudioConnection pc408(LFO1, 0, pitchAmtB4, 0);
AudioConnection pc409(env4, 0, pitchAmtB4, 1);
AudioConnection pc410(invEnv4, 0, pitchAmtB4, 2);
AudioConnection pc411(LFO1, 0, pitchAmtC4, 0);
AudioConnection pc412(env4, 0, pitchAmtC4, 1);
AudioConnection pc413(invEnv4, 0, pitchAmtC4, 2);

AudioConnection pc414(LFO2, 0, pwmAmtA4, 0);
AudioConnection pc415(env4, 0, pwmAmtA4, 1);
AudioConnection pc416(invEnv4, 0, pwmAmtA4, 2);
AudioConnection pc417(dc_pwmAbias, 0, pwmAmtA4, 3);
AudioConnection pc418(LFO2, 0, pwmAmtB4, 0);
AudioConnection pc419(env4, 0, pwmAmtB4, 1);
AudioConnection pc420(invEnv4, 0, pwmAmtB4, 2);
AudioConnection pc421(dc_pwmBbias, 0, pwmAmtB4, 3);
AudioConnection pc422(LFO2, 0, pwmAmtC4, 0);
AudioConnection pc423(env4, 0, pwmAmtC4, 1);
AudioConnection pc424(invEnv4, 0, pwmAmtC4, 2);
AudioConnection pc425(dc_pwmCbias, 0, pwmAmtC4, 3);

AudioConnection pc426(pitchAmtA4, 0, dco4A, 0);
AudioConnection pc427(pwmAmtA4,   0, dco4A, 1);
AudioConnection pc428(pitchAmtB4, 0, dco4B, 0);
AudioConnection pc429(pwmAmtB4,   0, dco4B, 1);
AudioConnection pc430(pitchAmtC4, 0, dco4C, 0);
AudioConnection pc431(pwmAmtC4,   0, dco4C, 1);
AudioConnection pc432(dco4A, 0, voiceMix4, 0);
AudioConnection pc433(dco4B, 0, voiceMix4, 1);
AudioConnection pc434(dco4C, 0, voiceMix4, 2);
AudioConnection pc435(noiseMix, 0, voiceMix4, 3);
AudioConnection pc436(voiceMix4, 0, tdmOut, 6);
AudioConnection pc439(dco4B, 0, pitchAmtA4, 3);

// ========= COMMON SOURCES INTO VOICE 5 =========
// Gate the envelopes
AudioConnection pc500(dc_one, env5);

// Build inverted Env5
AudioConnection pc503(dc_neg1, 0, invEnv5, 1);
AudioConnection pc504(env5,    0, invEnv5, 0);

// Pitch selectors (LFO1 / Env5 / -Env5)
AudioConnection pc505(LFO1, 0, pitchAmtA5, 0);
AudioConnection pc506(env5, 0, pitchAmtA5, 1);
AudioConnection pc507(invEnv5, 0, pitchAmtA5, 2);

AudioConnection pc508(LFO1, 0, pitchAmtB5, 0);
AudioConnection pc509(env5, 0, pitchAmtB5, 1);
AudioConnection pc510(invEnv5, 0, pitchAmtB5, 2);

AudioConnection pc511(LFO1, 0, pitchAmtC5, 0);
AudioConnection pc512(env5, 0, pitchAmtC5, 1);
AudioConnection pc513(invEnv5, 0, pitchAmtC5, 2);

// PWM selectors (LFO2 / Env5 / -Env5 / DC-bias per osc)
AudioConnection pc514(LFO2, 0, pwmAmtA5, 0);
AudioConnection pc515(env5, 0, pwmAmtA5, 1);
AudioConnection pc516(invEnv5, 0, pwmAmtA5, 2);
AudioConnection pc517(dc_pwmAbias, 0, pwmAmtA5, 3);

AudioConnection pc518(LFO2, 0, pwmAmtB5, 0);
AudioConnection pc519(env5, 0, pwmAmtB5, 1);
AudioConnection pc520(invEnv5, 0, pwmAmtB5, 2);
AudioConnection pc521(dc_pwmBbias, 0, pwmAmtB5, 3);

AudioConnection pc522(LFO2, 0, pwmAmtC5, 0);
AudioConnection pc523(env5, 0, pwmAmtC5, 1);
AudioConnection pc524(invEnv5, 0, pwmAmtC5, 2);
AudioConnection pc525(dc_pwmCbias, 0, pwmAmtC5, 3);

// Voice 5 DCO routing
AudioConnection pc526(pitchAmtA5, 0, dco5A, 0);
AudioConnection pc527(pwmAmtA5,   0, dco5A, 1);
AudioConnection pc528(pitchAmtB5, 0, dco5B, 0);
AudioConnection pc529(pwmAmtB5,   0, dco5B, 1);
AudioConnection pc530(pitchAmtC5, 0, dco5C, 0);
AudioConnection pc531(pwmAmtC5,   0, dco5C, 1);

// Mix A/B/C plus noise (noiseMix -> ch3 already exists; connect here)
AudioConnection pc532(dco5A, 0, voiceMix5, 0);
AudioConnection pc533(dco5B, 0, voiceMix5, 1);
AudioConnection pc534(dco5C, 0, voiceMix5, 2);
AudioConnection pc535(noiseMix, 0, voiceMix5, 3);

// Send voice 5 to TDM (usually slot 8)
AudioConnection pc536(voiceMix5, 0, tdmOut, 8);

// (optional) tap envelopes to queues
AudioConnection pc539(dco5B, 0, pitchAmtA5, 3);


// ========= VOICE 6 (same pattern with env6, invEn6, pitchAmt?6, pwmAmt?6, dco6?, voiceMix6) =========
AudioConnection pc600(dc_one, env6);
AudioConnection pc603(dc_neg1, 0, invEnv6, 1);
AudioConnection pc604(env6,    0, invEnv6, 0);

AudioConnection pc605(LFO1, 0, pitchAmtA6, 0);
AudioConnection pc606(env6, 0, pitchAmtA6, 1);
AudioConnection pc607(invEnv6, 0, pitchAmtA6, 2);
AudioConnection pc608(LFO1, 0, pitchAmtB6, 0);
AudioConnection pc609(env6, 0, pitchAmtB6, 1);
AudioConnection pc610(invEnv6, 0, pitchAmtB6, 2);
AudioConnection pc611(LFO1, 0, pitchAmtC6, 0);
AudioConnection pc612(env6, 0, pitchAmtC6, 1);
AudioConnection pc613(invEnv6, 0, pitchAmtC6, 2);

AudioConnection pc614(LFO2, 0, pwmAmtA6, 0);
AudioConnection pc615(env6, 0, pwmAmtA6, 1);
AudioConnection pc616(invEnv6, 0, pwmAmtA6, 2);
AudioConnection pc617(dc_pwmAbias, 0, pwmAmtA6, 3);
AudioConnection pc618(LFO2, 0, pwmAmtB6, 0);
AudioConnection pc619(env6, 0, pwmAmtB6, 1);
AudioConnection pc620(invEnv6, 0, pwmAmtB6, 2);
AudioConnection pc621(dc_pwmBbias, 0, pwmAmtB6, 3);
AudioConnection pc622(LFO2, 0, pwmAmtC6, 0);
AudioConnection pc623(env6, 0, pwmAmtC6, 1);
AudioConnection pc624(invEnv6, 0, pwmAmtC6, 2);
AudioConnection pc625(dc_pwmCbias, 0, pwmAmtC6, 3);

AudioConnection pc626(pitchAmtA6, 0, dco6A, 0);
AudioConnection pc627(pwmAmtA6,   0, dco6A, 1);
AudioConnection pc628(pitchAmtB6, 0, dco6B, 0);
AudioConnection pc629(pwmAmtB6,   0, dco6B, 1);
AudioConnection pc630(pitchAmtC6, 0, dco6C, 0);
AudioConnection pc631(pwmAmtC6,   0, dco6C, 1);
AudioConnection pc632(dco6A, 0, voiceMix6, 0);
AudioConnection pc633(dco6B, 0, voiceMix6, 1);
AudioConnection pc634(dco6C, 0, voiceMix6, 2);
AudioConnection pc635(noiseMix, 0, voiceMix6, 3);
AudioConnection pc636(voiceMix6, 0, tdmOut, 10);
AudioConnection pc639(dco6B, 0, pitchAmtA6, 3);


// ========= VOICE 7 (same pattern) =========
AudioConnection pc700(dc_one, env7);
AudioConnection pc703(dc_neg1, 0, invEnv7, 1);
AudioConnection pc704(env7,    0, invEnv7, 0);

AudioConnection pc705(LFO1, 0, pitchAmtA7, 0);
AudioConnection pc706(env7, 0, pitchAmtA7, 1);
AudioConnection pc707(invEnv7, 0, pitchAmtA7, 2);
AudioConnection pc708(LFO1, 0, pitchAmtB7, 0);
AudioConnection pc709(env7, 0, pitchAmtB7, 1);
AudioConnection pc710(invEnv7, 0, pitchAmtB7, 2);
AudioConnection pc711(LFO1, 0, pitchAmtC7, 0);
AudioConnection pc712(env7, 0, pitchAmtC7, 1);
AudioConnection pc713(invEnv7, 0, pitchAmtC7, 2);

AudioConnection pc714(LFO2, 0, pwmAmtA7, 0);
AudioConnection pc715(env7, 0, pwmAmtA7, 1);
AudioConnection pc716(invEnv7, 0, pwmAmtA7, 2);
AudioConnection pc717(dc_pwmAbias, 0, pwmAmtA7, 3);
AudioConnection pc718(LFO2, 0, pwmAmtB7, 0);
AudioConnection pc719(env7, 0, pwmAmtB7, 1);
AudioConnection pc720(invEnv7, 0, pwmAmtB7, 2);
AudioConnection pc721(dc_pwmBbias, 0, pwmAmtB7, 3);
AudioConnection pc722(LFO2, 0, pwmAmtC7, 0);
AudioConnection pc723(env7, 0, pwmAmtC7, 1);
AudioConnection pc724(invEnv7, 0, pwmAmtC7, 2);
AudioConnection pc725(dc_pwmCbias, 0, pwmAmtC7, 3);

AudioConnection pc726(pitchAmtA7, 0, dco7A, 0);
AudioConnection pc727(pwmAmtA7,   0, dco7A, 1);
AudioConnection pc728(pitchAmtB7, 0, dco7B, 0);
AudioConnection pc729(pwmAmtB7,   0, dco7B, 1);
AudioConnection pc730(pitchAmtC7, 0, dco7C, 0);
AudioConnection pc731(pwmAmtC7,   0, dco7C, 1);
AudioConnection pc732(dco7A, 0, voiceMix7, 0);
AudioConnection pc733(dco7B, 0, voiceMix7, 1);
AudioConnection pc734(dco7C, 0, voiceMix7, 2);
AudioConnection pc735(noiseMix, 0, voiceMix7, 3);
AudioConnection pc736(voiceMix7, 0, tdmOut, 12);
AudioConnection pc739(dco7B, 0, pitchAmtA7, 3);

// ========= VOICE 8 (same pattern) =========
AudioConnection pc800(dc_one, env8);
AudioConnection pc803(dc_neg1, 0, invEnv8, 1);
AudioConnection pc804(env8,    0, invEnv8, 0);

AudioConnection pc805(LFO1, 0, pitchAmtA8, 0);
AudioConnection pc806(env8, 0, pitchAmtA8, 1);
AudioConnection pc807(invEnv8, 0, pitchAmtA8, 2);
AudioConnection pc808(LFO1, 0, pitchAmtB8, 0);
AudioConnection pc809(env8, 0, pitchAmtB8, 1);
AudioConnection pc810(invEnv8, 0, pitchAmtB8, 2);
AudioConnection pc811(LFO1, 0, pitchAmtC8, 0);
AudioConnection pc812(env8, 0, pitchAmtC8, 1);
AudioConnection pc813(invEnv8, 0, pitchAmtC8, 2);

AudioConnection pc814(LFO2, 0, pwmAmtA8, 0);
AudioConnection pc815(env8, 0, pwmAmtA8, 1);
AudioConnection pc816(invEnv8, 0, pwmAmtA8, 2);
AudioConnection pc817(dc_pwmAbias, 0, pwmAmtA8, 3);
AudioConnection pc818(LFO2, 0, pwmAmtB8, 0);
AudioConnection pc819(env8, 0, pwmAmtB8, 1);
AudioConnection pc820(invEnv8, 0, pwmAmtB8, 2);
AudioConnection pc821(dc_pwmBbias, 0, pwmAmtB8, 3);
AudioConnection pc822(LFO2, 0, pwmAmtC8, 0);
AudioConnection pc823(env8, 0, pwmAmtC8, 1);
AudioConnection pc824(invEnv8, 0, pwmAmtC8, 2);
AudioConnection pc825(dc_pwmCbias, 0, pwmAmtC8, 3);

AudioConnection pc826(pitchAmtA8, 0, dco8A, 0);
AudioConnection pc827(pwmAmtA8,   0, dco8A, 1);
AudioConnection pc828(pitchAmtB8, 0, dco8B, 0);
AudioConnection pc829(pwmAmtB8,   0, dco8B, 1);
AudioConnection pc830(pitchAmtC8, 0, dco8C, 0);
AudioConnection pc831(pwmAmtC8,   0, dco8C, 1);
AudioConnection pc832(dco8A, 0, voiceMix8, 0);
AudioConnection pc833(dco8B, 0, voiceMix8, 1);
AudioConnection pc834(dco8C, 0, voiceMix8, 2);
AudioConnection pc835(noiseMix, 0, voiceMix8, 3);
AudioConnection pc836(voiceMix8, 0, tdmOut, 14);
AudioConnection pc839(dco8B, 0, pitchAmtA8, 3);

AudioControlCS42448      cs42448;        //xy=1534.8571166992188,29.142852783203125
// GUItool: end automatically generated code
