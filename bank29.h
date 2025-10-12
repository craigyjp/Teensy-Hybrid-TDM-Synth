// bank29.h

// Includes
#include "Wavetables/AKWF_bw_sin/AKWF_sin_0001_256.h"
#include "Wavetables/AKWF_bw_sin/AKWF_sin_0002_256.h"
#include "Wavetables/AKWF_bw_sin/AKWF_sin_0003_256.h"
#include "Wavetables/AKWF_bw_sin/AKWF_sin_0004_256.h"
#include "Wavetables/AKWF_bw_sin/AKWF_sin_0005_256.h"
#include "Wavetables/AKWF_bw_sin/AKWF_sin_0006_256.h"
#include "Wavetables/AKWF_bw_sin/AKWF_sin_0007_256.h"
#include "Wavetables/AKWF_bw_sin/AKWF_sin_0008_256.h"
#include "Wavetables/AKWF_bw_sin/AKWF_sin_0009_256.h"
#include "Wavetables/AKWF_bw_sin/AKWF_sin_0010_256.h"
#include "Wavetables/AKWF_bw_sin/AKWF_sin_0011_256.h"
#include "Wavetables/AKWF_bw_sin/AKWF_sin_0012_256.h"

// Pointer table
const int16_t* bank29_tables[] = {
  AKWF_sin_0001_256_DATA,
  AKWF_sin_0002_256_DATA,
  AKWF_sin_0003_256_DATA,
  AKWF_sin_0004_256_DATA,
  AKWF_sin_0005_256_DATA,
  AKWF_sin_0006_256_DATA,
  AKWF_sin_0007_256_DATA,
  AKWF_sin_0008_256_DATA,
  AKWF_sin_0009_256_DATA,
  AKWF_sin_0010_256_DATA,
  AKWF_sin_0011_256_DATA,
  AKWF_sin_0012_256_DATA,
};

const uint16_t bank29_count = sizeof(bank29_tables) / sizeof(bank29_tables[0]);
