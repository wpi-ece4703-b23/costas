#include "xlaudio.h"
#include "xlaudio_armdsp.h"
#include <stdlib.h>

#define SYMBOLS 3
#define UPSAMPLE 16

#define NUMCOEF (SYMBOLS * UPSAMPLE + 1)

// This array contains the filter coefficients
// you have computed for the root raised cosine filter
// implementation (3 symbols, 16 samples per symbol, roll-off 0.3)

float32_t coef[NUMCOEF] = {
  -0.042923315, -0.047711509,   -0.050726122, -0.05161785,
  -0.050086476, -0.045895194,   -0.038883134, -0.02897551,
  -0.016190898, -0.00064525557,  0.017447588,  0.037779089,
   0.05995279,   0.083494586,    0.10786616,   0.13248111,
   0.1567233,    0.17996659,     0.20159548,   0.22102564,
   0.23772369,   0.2512255,      0.26115217,   0.26722319,
   0.26926623,   0.26722319,     0.26115217,   0.2512255,
   0.23772369,   0.22102564,     0.20159548,   0.17996659,
   0.1567233,    0.13248111,     0.10786616,   0.083494586,
   0.05995279,   0.037779089,    0.017447588, -0.00064525557,
  -0.016190898, -0.02897551,    -0.038883134, -0.045895194,
  -0.050086476, -0.05161785,    -0.050726122, -0.047711509,
  -0.042923315
};

float32_t coefcos[NUMCOEF];
float32_t coefsin[NUMCOEF];

void modulatefs4_cos() {
    int i;
    float32_t carrier[4] = {1.0f, 0.0f, -1.0f, 0.0f};
    for (i = 0; i<NUMCOEF; i++)
        coefcos[i] = coef[i] * carrier[i % 4];
}

void modulatefs4_sin() {
    int i;
    float32_t carrier[4] = {0.0f, 1.0f, 0.0f, -1.0f};
    for (i = 0; i<NUMCOEF; i++)
        coefsin[i] = coef[i] * carrier[i % 4];
}

#define NUMTAPS (SYMBOLS+1)
int symboltaps[NUMTAPS];

float32_t rrcphase(int phase, float32_t symbol) {
    uint32_t i;
    if (phase == 0) {
        for (i=NUMTAPS-1; i>0; i--)
            symboltaps[i] = symboltaps[i-1];
        symboltaps[0] =symbol;
        xlaudio_debugpinhigh();
    } else {
        xlaudio_debugpinlow();
    }

    float32_t *localcoef = coefcos;
    if (xlaudio_pushButtonLeftDown())
        localcoef = coefsin;

    float32_t q = 0.0;
    uint16_t limit = (phase == 0) ? NUMTAPS : NUMTAPS - 1;
    for (i=0; i<limit; i++)
        q += symboltaps[i] * localcoef[i * UPSAMPLE + phase + 1];

    return q;
}

int bpsksymbolgenerator() {
    return 1 - (rand() % 2)*2;
}

uint16_t processSample(uint16_t x) {
    static int filterphase = 0;
    float32_t result = rrcphase(filterphase, bpsksymbolgenerator());
    filterphase = (filterphase + 1) % UPSAMPLE;
    return xlaudio_f32_to_dac14(result * 0.25);
}

#include <stdio.h>

int main(void) {
    WDT_A_hold(WDT_A_BASE);

    modulatefs4_cos();
    modulatefs4_sin();

    xlaudio_clocksystem(XLAUDIO_XTAL);
    xlaudio_init_intr(FS_8000_HZ, XLAUDIO_J1_2_IN, processSample);

    int c = xlaudio_measurePerfSample(processSample);
    printf("Cycles %d\n", c);

    xlaudio_run();

    return 1;
}
