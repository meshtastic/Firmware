#if QUIRK_RTTTL
#define ledcSetup(ch, freq, res)                                                                                                 \
    uint32_t __freq = freq;                                                                                                      \
    uint8_t __res = res;                                                                                                         \
    do {                                                                                                                         \
    } while (0)
#define ledcAttachPin(pin, ch) ledcAttachChannel(pin, __freq, __res, ch)
#endif

#if QUIRK_LOVYAN // for chatter2 and m5stack-core
#include "rom/ets_sys.h"
#include <stdbool.h>
#undef bool
#undef true
#undef false
#endif
