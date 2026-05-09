#ifndef _SWITCH_PRO_CLONE_RECOVERY_H_
#define _SWITCH_PRO_CLONE_RECOVERY_H_

#include <cstdint>

namespace SwitchProCloneRecovery
{
    uint32_t debug_flags();
    void reset_all();

    void mark_clone_profile_detected();
    bool clone_profile_detected();

    void arm_next_attach_recovery();
    bool next_attach_recovery_armed();
    void clear_next_attach_recovery();

    void begin_attach_attempt();
    void end_attach_attempt();

    bool request_reboot_once();
    void clear_reboot_request();
}

#endif // _SWITCH_PRO_CLONE_RECOVERY_H_
