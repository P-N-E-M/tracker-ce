
#ifndef INTERFACE_PERFORMANCEMODE_PERFORMANCEFX_H_
#define INTERFACE_PERFORMANCEMODE_PERFORMANCEFX_H_

#include <stdint.h>

struct fxSetup
{
    int16_t min;
    int16_t max;

    const char* alternativeValuesLabels = nullptr;
    uint8_t alternativeValuesLabelsLength = 0;

    // id fxow zerowanych przez ten fx
    uint8_t blockedFx1 = 0;
    uint8_t blockedFx2 = 0;

    bool isGlobalFx = false;

    // PNEM: the top row holds a value of its own (stored, editable in Rec) instead of always being
    // off; selecting it still turns the effect off (clearFunct), which uses that value (MIDI CC A-F)
    bool topRowIsValue = false;
};

class cPerformFx
{
  public:
    cPerformFx(uint8_t fxId,
               const char* fxName,
               void (*fxSetFunct)(uint8_t trackIdx, int16_t value),
               void (*fxClearFunct)(uint8_t trackIdx),
               const fxSetup* fxSetup);
    ~cPerformFx() {}

    uint8_t id;
    const char* name;
    void (*setFunct)(uint8_t trackIdx, int16_t value);
    void (*clearFunct)(uint8_t trackIdx);
    const fxSetup* setup;
};

#endif /* INTERFACE_PERFORMANCEMODE_PERFORMANCEFX_H_ */
