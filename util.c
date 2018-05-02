#include <debug.h>

#include "libdelta.h"
#include "util.h"

void
debug(const char *str)
{
	purple_debug_info(PLUGIN_ID, str);
}

