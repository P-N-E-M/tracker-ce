// PNEM CE: options in Config > General (see pnemOptions.h).

#include "pnemOptions.h"

#include "mtStructs.h"

namespace pnem
{

uint8_t optionMenuValue[kOptionCount] = {};

bool option(Option which)
{
    return which < kOptionCount && (mtConfig.general.pnemOptionsOn & (1u << which)) != 0;
}

void optionsFromConfig()
{
    mtConfig.general.pnemOptionsOn &= (1u << kOptionCount) - 1; // unknown bits off
    for (uint8_t i = 0; i < kOptionCount; i++)
    {
        optionMenuValue[i] = option(static_cast<Option>(i)) ? 1 : 0;
    }
}

void optionChanged()
{
    uint8_t on = 0;
    for (uint8_t i = 0; i < kOptionCount; i++)
    {
        if (optionMenuValue[i] != 0)
        {
            on |= (uint8_t)(1u << i);
        }
    }
    mtConfig.general.pnemOptionsOn = on;
}

} // namespace pnem
