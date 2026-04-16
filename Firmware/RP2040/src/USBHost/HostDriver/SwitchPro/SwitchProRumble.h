#ifndef _SWITCH_PRO_RUMBLE_H_
#define _SWITCH_PRO_RUMBLE_H_

#include <cstdint>

namespace SwitchProRumble
{
    void fill_neutral(uint8_t (&data)[4]);
    void encode(uint8_t (&data)[4], uint8_t magnitude);
}

#endif // _SWITCH_PRO_RUMBLE_H_
