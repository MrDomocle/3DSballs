#pragma once

#include <3ds.h>
#include <math.h>

#define MAX_BALLS 128
#define PHYS_STEPS_PER_FRAME 10
#define BOUNCINESS_BASE 0.9f
#define GRAVITY 981.0f
#define STAR_STRENGTH 10.0f
#define TWORADIUS2 RADIUS*RADIUS*4
#define DELTA FRAME_S/PHYS_STEPS_PER_FRAME

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    u8 life;
    u32 clr;
} ball;
typedef struct {
    float x;
    float y;
    float strength;
    u8 enabled;
} star;

extern float bounciness;
extern float globalGX;
extern float globalGY;
extern star gStar;

// vector helpers
static inline float distance(float x1, float y1, float x2, float y2) {
    return sqrtf((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));
}
static inline float distance2(float x1, float y1, float x2, float y2) {
    return (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2);
}
static inline float magnitude(float dx, float dy) {
    return sqrtf(dx*dx + dy*dy);
}
static inline float magnitude2(float dx, float dy) {
    return dx*dx + dy*dy;
}
void norm(float *x, float *y);

void collideBalls(ball (*balls)[MAX_BALLS]);
void updateBall(ball* b, float gX, float gY);
void tickBalls(void *v);