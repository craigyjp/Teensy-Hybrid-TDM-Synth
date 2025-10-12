// bank26.h

// Includes
#include "Wavetables/AKWF_bw_sawbright/AKWF_bsaw_0001_256.h"
#include "Wavetables/AKWF_bw_sawbright/AKWF_bsaw_0002_256.h"
#include "Wavetables/AKWF_bw_sawbright/AKWF_bsaw_0003_256.h"
#include "Wavetables/AKWF_bw_sawbright/AKWF_bsaw_0004_256.h"
#include "Wavetables/AKWF_bw_sawbright/AKWF_bsaw_0005_256.h"
#include "Wavetables/AKWF_bw_sawbright/AKWF_bsaw_0006_256.h"
#include "Wavetables/AKWF_bw_sawbright/AKWF_bsaw_0007_256.h"
#include "Wavetables/AKWF_bw_sawbright/AKWF_bsaw_0008_256.h"
#include "Wavetables/AKWF_bw_sawbright/AKWF_bsaw_0009_256.h"
#include "Wavetables/AKWF_bw_sawbright/AKWF_bsaw_0010_256.h"

// Pointer table
const int16_t* bank26_tables[] = {
  AKWF_bsaw_0001_256_DATA,
  AKWF_bsaw_0002_256_DATA,
  AKWF_bsaw_0003_256_DATA,
  AKWF_bsaw_0004_256_DATA,
  AKWF_bsaw_0005_256_DATA,
  AKWF_bsaw_0006_256_DATA,
  AKWF_bsaw_0007_256_DATA,
  AKWF_bsaw_0008_256_DATA,
  AKWF_bsaw_0009_256_DATA,
  AKWF_bsaw_0010_256_DATA,
};

const uint16_t bank26_count = sizeof(bank26_tables) / sizeof(bank26_tables[0]);
