#pragma once

#include <cstdint>

#include "USBHost/HostDriver/HostDriverTypes.h"

namespace OGXMini {
    void initialize();
    void run();
    void host_mounted(bool mounted, HostDriverType host_type);
    void host_mounted(bool mounted);
    void wireless_connected(bool connected, uint8_t idx);
} // namespace OGXMini