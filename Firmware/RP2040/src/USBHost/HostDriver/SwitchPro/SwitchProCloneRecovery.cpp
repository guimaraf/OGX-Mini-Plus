#include <hardware/structs/watchdog.h>

#include "USBHost/HostDriver/SwitchPro/SwitchProCloneRecovery.h"

namespace
{
    constexpr uint32_t MAGIC = 0x53504352; // SPCR
    constexpr uint32_t FLAG_PROFILE_DETECTED = 0x01;
    constexpr uint32_t FLAG_ATTACH_RECOVERY_ARMED = 0x02;
    constexpr uint32_t FLAG_REBOOT_REQUESTED = 0x04;

    uint32_t read_flags()
    {
        if (watchdog_hw->scratch[0] != MAGIC)
        {
            return 0;
        }
        return watchdog_hw->scratch[1];
    }

    void write_flags(uint32_t flags)
    {
        watchdog_hw->scratch[0] = MAGIC;
        watchdog_hw->scratch[1] = flags;
    }
}

namespace SwitchProCloneRecovery
{
    void mark_clone_profile_detected()
    {
        write_flags(read_flags() | FLAG_PROFILE_DETECTED);
    }

    bool clone_profile_detected()
    {
        return (read_flags() & FLAG_PROFILE_DETECTED) != 0;
    }

    void arm_next_attach_recovery()
    {
        if (clone_profile_detected())
        {
            write_flags(read_flags() | FLAG_ATTACH_RECOVERY_ARMED);
        }
    }

    bool next_attach_recovery_armed()
    {
        return (read_flags() & FLAG_ATTACH_RECOVERY_ARMED) != 0;
    }

    void clear_next_attach_recovery()
    {
        const uint32_t flags = read_flags();
        if (flags != 0)
        {
            write_flags(flags & ~(FLAG_ATTACH_RECOVERY_ARMED | FLAG_REBOOT_REQUESTED));
        }
    }

    void begin_attach_attempt()
    {
        const uint32_t flags = read_flags();
        if ((flags & FLAG_ATTACH_RECOVERY_ARMED) != 0)
        {
            write_flags(flags & ~FLAG_REBOOT_REQUESTED);
        }
    }

    void end_attach_attempt()
    {
        clear_reboot_request();
    }

    bool request_reboot_once()
    {
        const uint32_t flags = read_flags();
        if ((flags & FLAG_REBOOT_REQUESTED) != 0)
        {
            return false;
        }

        write_flags(flags | FLAG_REBOOT_REQUESTED);
        return true;
    }

    void clear_reboot_request()
    {
        const uint32_t flags = read_flags();
        if (flags != 0)
        {
            write_flags(flags & ~FLAG_REBOOT_REQUESTED);
        }
    }
}
