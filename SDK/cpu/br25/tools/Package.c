#include "app_config.h"

// #define USER_PACKAGE_EN
#ifndef CONFIG_DEBUG_ENABLE
// #ifdef CONFIG_DEBUG_ENABLE

::cd cpu\br25\tools
cd  %~dp0

::start /wait "" "Package.cmd"
start "" "Package.cmd"

#endif