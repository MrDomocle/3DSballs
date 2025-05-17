#pragma once
#include <3ds.h>

#include "phys.h"

#define SCREEN_W 320
#define SCREEN_H 240

#define FRAME_S 0.01666666f
#define FRAME_MS FRAME_S*1000.0f
#define FRAME_NS (u64)(FRAME_MS*1000000.0f)

#define CPAD_MAX 160
#define RADIUS 6
#define R 0xFF0000
#define G 0x00FF00
#define B 0x0000FF

extern ball balls[MAX_BALLS];
extern u8 shutting_down;