// bank57.h

// Includes
#include "Wavetables/AKWF_stringbox/AKWF_cheeze_0001_256.h"
#include "Wavetables/AKWF_stringbox/AKWF_cheeze_0002_256.h"
#include "Wavetables/AKWF_stringbox/AKWF_cheeze_0003_256.h"
#include "Wavetables/AKWF_stringbox/AKWF_cheeze_0004_256.h"
#include "Wavetables/AKWF_stringbox/AKWF_cheeze_0005_256.h"
#include "Wavetables/AKWF_stringbox/AKWF_cheeze_0006_256.h"

// Pointer table
const int16_t* bank57_tables[] = {
  AKWF_cheeze_0001_256_DATA,
  AKWF_cheeze_0002_256_DATA,
  AKWF_cheeze_0003_256_DATA,
  AKWF_cheeze_0004_256_DATA,
  AKWF_cheeze_0005_256_DATA,
  AKWF_cheeze_0006_256_DATA,
};

const uint16_t bank57_count = sizeof(bank57_tables) / sizeof(bank57_tables[0]);
