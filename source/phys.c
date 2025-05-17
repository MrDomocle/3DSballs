#include <3ds.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#include "globals.h"
#include "phys.h"

float delta = DELTA;
float bounciness = BOUNCINESS_BASE;
float globalGX = 0;
float globalGY = GRAVITY;
star gStar = {0,0,0,0};

// vectors
void norm(float *x, float *y) {
    if (x != NULL && y != NULL) {
        float m = magnitude(*x, *y);
        *x /= m;
        *y /= m;
    }
}

// physics
void collideBalls(ball (*balls)[MAX_BALLS]) {
    for (int j = 0; j < MAX_BALLS; j++) {
        if ((*balls)[j].life <= 0) continue;
        ball *b1 = &(*balls)[j];

        for (int i = j+1; i < MAX_BALLS; i++) {
            if (!(*balls)[i].life) continue;

            ball *b2 = &(*balls)[i];
            // calculate vector
            float dx = b2->x - b1->x;
            float dy = b2->y - b1->y;

            float dist = magnitude2(dx, dy);
            if (dist < TWORADIUS2) {
                // this ball is worth it, so run sqrt
                dist = sqrtf(dist);

                // normalise vector between the balls
                norm(&dx, &dy);

                b1->vx += -dx*bounciness*(2*RADIUS - dist)/DELTA;
                b1->vy += -dy*bounciness*(2*RADIUS - dist)/DELTA;
                b2->vx += dx*bounciness*(2*RADIUS - dist)/DELTA;
                b2->vy += dy*bounciness*(2*RADIUS - dist)/DELTA;

                // un-stuck
                b1->x -= dx*(2*RADIUS - dist);
                b1->y -= dy*(2*RADIUS - dist);
                b2->x += dx*(2*RADIUS - dist);
                b2->y += dy*(2*RADIUS - dist);
            }
        }
    }
}

void updateBall(ball* b, float gX, float gY) {
    if (b->life == 0) return;

    // wall collisions
    if (b->x < RADIUS) {
        b->x = RADIUS;
        b->vx *= -bounciness;
    }
    else if (b->x > (SCREEN_W - RADIUS)) {
        b->x = SCREEN_W - RADIUS;
        b->vx *= -bounciness;
    }

    if (b->y < RADIUS) {
        b->y = RADIUS;
        b->vy *= -bounciness;
    }
    else if (b->y > (SCREEN_H - RADIUS)) {
        b->y = SCREEN_H - RADIUS;
        b->vy *= -bounciness;
    }

    b->vx += DELTA*gX;
    b->vy += DELTA*gY;

    b->x += DELTA*b->vx;
    b->y += DELTA*b->vy;
}

void tickBalls(void *v) {
    u64 framenumber = 0;
    u64 t_start = 0;
    u64 delta_start = svcGetSystemTick();
    while (!shutting_down) {
        delta_start = svcGetSystemTick();
        if (!(framenumber % PHYS_STEPS_PER_FRAME)) t_start = osGetTime();
        collideBalls(&balls);
        for (int i = 0; i < MAX_BALLS; i++) {
            if (balls[i].life == 0) continue;
            // get gravity for ball
            float gX, gY;
            gX = globalGX;
            gY = globalGY;
            if (gStar.enabled) {
                float dx = gStar.x - balls[i].x;
                float dy = gStar.y - balls[i].y;

                // magnitude of gravity
                float m = magnitude2(dx, dy) / gStar.strength;

                norm(&dx, &dy);
                gX += dx*m;
                gY += dy*m;
            }
            updateBall(&balls[i], gX, gY);
        }

        if (!((framenumber - PHYS_STEPS_PER_FRAME) % PHYS_STEPS_PER_FRAME)) {
            consoleClear();
            u64 frametime = osGetTime() - t_start;
            printf("\x1b[2;0H %lld; DELTA: %f", frametime, DELTA);
        }
        framenumber++;
        svcSleepThread(FRAME_NS/PHYS_STEPS_PER_FRAME);
    }
}