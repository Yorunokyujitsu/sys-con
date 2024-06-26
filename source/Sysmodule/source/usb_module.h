#pragma once

#include "vapours/results/results_common.hpp"
#include "config_handler.h"
namespace syscon::usb
{
    void Initialize(syscon::config::DiscoveryMode discovery_mode, std::vector<syscon::config::ControllerVidPid> &discovery_vidpid, bool auto_add_controller);
    void Exit();
} // namespace syscon::usb