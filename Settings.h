#include "SettingsService.h"

void settingsMIDICh();
void settingsMIDIOutCh();
void settingsEncoderDir();
void settingsEncoderAccelerate();
void settingsUpdateParams();
void settingsROMType();
void settingsSetBank();
void settingsLoadFactory();
void settingsLoadRAM();
void settingsAfterTouch();
void settingsSaveAll();

int currentIndexMIDICh();
int currentIndexMIDIOutCh();
int currentIndexEncoderDir();
int currentIndexEncoderAccelerate();
int currentIndexUpdateParams();
int currentIndexROMType();
int currentIndexSetBank();
int currentIndexLoadFactory();
int currentIndexLoadRAM();
int currentIndexAfterTouch();
int currentIndexSaveAll();

void settingsMIDICh(int index, const char *value) {
  if (strcmp(value, "ALL") == 0) {
    midiChannel = MIDI_CHANNEL_OMNI;
  } else {
    midiChannel = atoi(value);
  }
  storeMidiChannel(midiChannel);
}

void settingsMIDIOutCh(int index, const char *value) {
  if (strcmp(value, "Off") == 0) {
    midiOutCh = 0;
  } else {
    midiOutCh = atoi(value);
  }
  storeMidiOutCh(midiOutCh);
}

void settingsEncoderDir(int index, const char *value) {
  if (strcmp(value, "Type 1") == 0) {
    encCW = true;
  } else {
    encCW = false;
  }
  storeEncoderDir(encCW ? 1 : 0);
}

void settingsEncoderAccelerate(int index, const char *value) {
  if (strcmp(value, "Yes") == 0) {
    accelerate = true;
  } else {
    accelerate = false;
  }
  storeEncoderAccelerate(accelerate ? 1 : 0);
}

void settingsUpdateParams(int index, const char *value) {
  if (strcmp(value, "Send Params") == 0) {
    updateParams = true;
  } else {
    updateParams = false;
  }
  storeUpdateParams(updateParams ? 1 : 0);
}

void settingsSetBank(int index, const char *value) {
  if (strcmp(value, "RAM") == 0) {
    bankselect = 0;
  } else {
    bankselect = atoi(value);
  }
  storeSetBank(bankselect);
}

void settingsLoadFactory(int index, const char *value) {
  if (strcmp(value, "Yes") == 0) {
    loadFactory = true;
  } else {
    loadFactory = false;
  }
  storeLoadFactory(loadFactory);
}

void settingsLoadRAM(int index, const char *value) {
  if (strcmp(value, "Yes") == 0) {
    loadRAM = true;
  } else {
    loadRAM = false;
  }
  storeLoadRAM(loadRAM);
}

void settingsROMType(int index, const char *value) {
  if (strcmp(value, "ROM I") == 0) {
    ROMType = true;
  } else {
    ROMType = false;
  }
  storeROMType(ROMType);
}

void settingsSaveCurrent(int index, const char *value) {
  if (strcmp(value, "Yes") == 0) {
    saveCurrent = true;
  } else {
    saveCurrent = false;
  }
  storeSaveCurrent(saveCurrent);
}

void settingsAfterTouch(int index, const char *value) {
  if (strcmp(value, "Off") == 0) {
    afterTouch = false;
  } else {
    afterTouch = true;
  }
  storeAfterTouch(afterTouch);
}

void settingsSaveAll(int index, const char *value) {
  if (strcmp(value, "Yes") == 0) {
    saveAll = true;
  } else {
    saveAll = false;
  }
  storeSaveAll(saveAll);
}

int currentIndexMIDICh() {
  return getMIDIChannel();
}

int currentIndexMIDIOutCh() {
  return getMIDIOutCh();
}

int currentIndexEncoderDir() {
  return getEncoderDir() ? 0 : 1;
}

int currentIndexEncoderAccelerate() {
  return getEncoderAccelerate() ? 1 : 0;
}

int currentIndexUpdateParams() {
  return getUpdateParams() ? 1 : 0;
}

int currentIndexSetBank() {
  return getSetBank();
}

int currentIndexLoadFactory() {
  return getLoadFactory();
}

int currentIndexLoadRAM() {
  return getLoadRAM();
}

int currentIndexROMType() {
  return getROMType() ? 0 : 1;
}

int currentIndexSaveCurrent() {
  return getSaveCurrent();
}

int currentIndexSaveAll() {
  return getSaveAll();
}

int currentIndexAfterTouch() {
  return getAfterTouch() ? 1 : 0;
}

// add settings to the circular buffer
void setUpSettings() {
  settings::append(settings::SettingsOption{ "MIDI Ch.", { "All", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "\0" }, settingsMIDICh, currentIndexMIDICh });
  settings::append(settings::SettingsOption{ "MIDI Out Ch.", { "Off", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "\0" }, settingsMIDIOutCh, currentIndexMIDIOutCh });
  settings::append(settings::SettingsOption{ "Encoder", { "Type 1", "Type 2", "\0" }, settingsEncoderDir, currentIndexEncoderDir });
  settings::append(settings::SettingsOption{ "Enc Speed", { "No", "Yes", "\0" }, settingsEncoderAccelerate, currentIndexEncoderAccelerate });
  settings::append(settings::SettingsOption{ "MIDI Params", { "Off", "Send Params", "\0" }, settingsUpdateParams, currentIndexUpdateParams });
  settings::append(settings::SettingsOption{ "Set Bank", { "RAM", "1", "2", "3", "4", "\0" }, settingsSetBank, currentIndexSetBank });
  settings::append(settings::SettingsOption{ "Load Factory", { "No", "Yes", "\0" }, settingsLoadFactory, currentIndexLoadFactory });
  settings::append(settings::SettingsOption{ "Load RAM", { "No", "Yes", "\0" }, settingsLoadRAM, currentIndexLoadRAM });
  settings::append(settings::SettingsOption{ "Aftertouch", { "Off", "On", "\0" }, settingsAfterTouch, currentIndexAfterTouch });
  settings::append(settings::SettingsOption{ "Send RAM", { "No", "Yes", "\0" }, settingsSaveAll, currentIndexSaveAll });
  settings::append(settings::SettingsOption{ "ROM Type", { "ROM I", "ROM K or L", "\0" }, settingsROMType, currentIndexROMType });
}
