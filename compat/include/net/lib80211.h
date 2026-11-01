/* lib80211.h saiu do kernel; o driver so usa esta constante dele. */
#ifndef SSV_COMPAT_LIB80211_H
#define SSV_COMPAT_LIB80211_H
#include <linux/ieee80211.h>
#define IEEE80211_CRYPTO_TKIP_COUNTERMEASURES (1 << 0)
#endif
