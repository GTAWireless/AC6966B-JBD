#include "app_config.h"

#define USER_PACKAGE_EN
#ifdef USER_PACKAGE_EN

::cd cpu\br25\tools
cd  %~dp0

::start /wait "" "Package.cmd"
start "" "Package.cmd"

#endif