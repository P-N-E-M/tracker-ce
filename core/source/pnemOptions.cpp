// PNEM CE: options in Config > General (see pnemOptions.h).

#include "pnemOptions.h"

#include "mtStructs.h"

namespace pnem
{

uint8_t optionMenuValue[kOptionCount] = { 1, 1, 1 };

bool option(Option which)
{
    return which < kOptionCount && (mtConfig.general.pnemOptionsOff & (1u << which)) == 0;
}

void optionsFromConfig()
{
    mtConfig.general.pnemOptionsOff &= (1u << kOptionCount) - 1; // unknown bits off
    for (uint8_t i = 0; i < kOptionCount; i++)
    {
        optionMenuValue[i] = option(static_cast<Option>(i)) ? 1 : 0;
    }
}

void optionChanged()
{
    uint8_t off = 0;
    for (uint8_t i = 0; i < kOptionCount; i++)
    {
        if (optionMenuValue[i] == 0)
        {
            off |= (uint8_t)(1u << i);
        }
    }
    mtConfig.general.pnemOptionsOff = off;
}

} // namespace pnem
