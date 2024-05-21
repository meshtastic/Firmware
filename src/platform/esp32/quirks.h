#if QUIRK_RTTTL || QUIRK_LOVYAN
#define ledcSetup(ch, freq, res)                                                                                                 \
    uint32_t __freq = freq;                                                                                                      \
    uint8_t __res = res;                                                                                                         \
    do {                                                                                                                         \
    } while (0)
#define ledcAttachPin(pin, ch) ledcAttachChannel(pin, __freq, __res, ch)
#endif

#if QUIRK_LOVYAN
#include <stdbool.h>

#include "esp_heap_caps.h"
#include "multi_heap.h"

#include "rom/ets_sys.h"
#include "rom/gpio.h"
#include "soc/gpio_struct.h"
#include "soc/spi_periph.h"
#include "soc/spi_reg.h"

#undef bool
#undef true
#undef false
#endif

#if QUIRK_ESP8266_AUDIO
#ifdef __cplusplus
#include "WiFi.h"
#endif
#include "esp_chip_info.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#define rtc_clk_apll_enable(...)
#define I2S_MCLK_MULTIPLE_DEFAULT I2S_MCLK_MULTIPLE_128
#endif