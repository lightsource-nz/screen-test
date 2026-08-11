#ifndef _SCREENTEST_CONFIG_H
#define _SCREENTEST_CONFIG_H

// board config for the two-display Pico-OLED-1.3 rig.
//
// deliberately almost empty: screentest_common's defaults were written for exactly this
// panel (64x128, 1bpp, REND_ROTATE_90) and this rig's two displays, so there is nothing to
// override. the file still exists rather than being optional, because every app providing
// one makes the set of them greppable -- and an app that needs no overrides is worth being
// able to see at a glance, rather than inferring from an absence

#endif
