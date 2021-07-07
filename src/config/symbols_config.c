
#include <sdk/types.h>

#if _IS_BOOT
u32 const symbols_table[] = {0};
#else
#include <sos/symbols.h>
#include "config.h"
#include <sos/symbols/table.h>
#endif
