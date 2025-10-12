// bank53.h

// Includes
#include "Wavetables/AKWF_pluckalgo/AKWF_pluckalgo_0001_256.h"
#include "Wavetables/AKWF_pluckalgo/AKWF_pluckalgo_0002_256.h"
#include "Wavetables/AKWF_pluckalgo/AKWF_pluckalgo_0003_256.h"
#include "Wavetables/AKWF_pluckalgo/AKWF_pluckalgo_0004_256.h"
#include "Wavetables/AKWF_pluckalgo/AKWF_pluckalgo_0005_256.h"
#include "Wavetables/AKWF_pluckalgo/AKWF_pluckalgo_0006_256.h"
#include "Wavetables/AKWF_pluckalgo/AKWF_pluckalgo_0007_256.h"
#include "Wavetables/AKWF_pluckalgo/AKWF_pluckalgo_0008_256.h"
#include "Wavetables/AKWF_pluckalgo/AKWF_pluckalgo_0009_256.h"

// Pointer table
const int16_t* bank53_tables[] = {
  AKWF_pluckalgo_0001_256_DATA,
  AKWF_pluckalgo_0002_256_DATA,
  AKWF_pluckalgo_0003_256_DATA,
  AKWF_pluckalgo_0004_256_DATA,
  AKWF_pluckalgo_0005_256_DATA,
  AKWF_pluckalgo_0006_256_DATA,
  AKWF_pluckalgo_0007_256_DATA,
  AKWF_pluckalgo_0008_256_DATA,
  AKWF_pluckalgo_0009_256_DATA,
};

const uint16_t bank53_count = sizeof(bank53_tables) / sizeof(bank53_tables[0]);
