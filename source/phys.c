#include <3ds.h>
#include <stdlib.h>
#include <time.h>

#include "globals.h"
#include "phys.h"

float bounciness = BOUNCINESS_BASE;
float globalGX = 0;
float globalGY = GRAVITY;
star *gStar = NULL;

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
        ball *b1 = &(*balls)[j];
        if (b1->life <= 0) continue;

        for (int i = 0; i < MAX_BALLS; i++) {
            ball *b2 = &(*balls)[i];
            // don't collide with self & dead balls
            if (b1 == b2 || b2->life <= 0) continue;

            float dist = distance2(b1->x, b1->y, b2->x, b2->y);
            if (dist < TWORADIUS2) {
                // this ball is worth it, so run sqrt
                dist = sqrtf(dist);

                // calculate & normalise vector between the balls
                float dx = b2->x - b1->x;
                float dy = b2->y - b1->y;
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

void tickBalls(ball (*balls)[MAX_BALLS]) {

    collideBalls(balls);
    for (int i = 0; i < MAX_BALLS; i++) {
        // collideBalls(balls); // this makes everything extremely slow but makes the simulation less jittery
        if ((*balls)[i].life == 0) continue;
        // get gravity for ball
        float gX, gY;
        if (gStar != NULL) {
            float dx = gStar->x - (*balls)[i].x;
            float dy = gStar->y - (*balls)[i].y;
            // magnitude of gravity
            float m = gStar->strength / magnitude2(dx, dy);

            norm(&dx, &dy);
            gX = dx*m;
            gY = dy*m;
        } else {
            gX = globalGX;
            gY = globalGY;
        }
        updateBall(&(*balls)[i], gX, gY);
    }
}