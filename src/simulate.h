#ifndef __SIMULATE_H__
#define __SIMULATE_H__
#define NUM_PRED_SIZES 4

#include "memory.h"
#include "read_elf.h"
#include <stdio.h>

// Simulerer RISC-V programmet i givet lager og fra given start adresse

struct PredictorStat {
    unsigned long long predictions;
    unsigned long long mispredictions;
};

struct Stat {
    long int insns;

    // Always Not Taken
    struct PredictorStat nt;

    // Backward Taken, Forward Not Taken
    struct PredictorStat btfnt;

    // Bimodal og gShare – én entry per tabelstørrelse
    struct PredictorStat bimodal[NUM_PRED_SIZES];
    struct PredictorStat gshare[NUM_PRED_SIZES];
};


// NOTE: Use of symbols provide for nicer disassembly, but is not required for A4.
// Feel free to remove this parameter or pass in a NULL pointer and ignore it.

struct Stat simulate(struct memory *mem, int start_addr, FILE *log_file, struct symbols* symbols);

#endif