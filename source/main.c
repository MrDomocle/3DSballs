#include <3ds.h>
#include <citro2d.h>

#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#define MAX_BALLS 32
#define SCREEN_W 320
#define SCREEN_H 240
#define CIRCLE_MAX 160
#define DELTA 0.2f
#define GRAVITY 9.81f
#define GSCALE 10.0f
#define RADIUS 9
#define BOUNCINESS_BASE 0.9f
#define COOLDOWN 32
#define LIFETIME 1200
#define R 0xFF0000
#define G 0x00FF00
#define B 0x0000FF

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    u32 life;
    u32 clr;
} ball;

float bounciness = 0.9f;

// vector helpers
float distance(float x1, float y1, float x2, float y2) {
    return sqrtf((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));
}
float magnitude(float dx, float dy) {
    return sqrtf(dx*dx + dy*dy);
}
void norm(float *x, float *y) {
    if (x != NULL && y != NULL) {
        float m = magnitude(*x, *y);
        *x /= m;
        *y /= m;
    }
}

u32 randomCol() {
    u32 clr;
    switch (rand()%3) {
        case 0:
            clr = C2D_Color32(rand()%255, rand()%128, rand()%128, 255);
        break;
        case 1:
            clr = C2D_Color32(rand()%128, rand()%255, rand()%128, 255);
        break;
        default:
            clr = C2D_Color32(rand()%128, rand()%128, rand()%255, 255);
        break;
    }
    return clr;
}

void collideBalls(ball (*balls)[MAX_BALLS]) {
    for (int j = 0; j < MAX_BALLS; j++) {
        ball *b1 = &(*balls)[j];
        if (b1->life <= 0) continue;

        for (int i = 0; i < MAX_BALLS; i++) {
            ball *b2 = &(*balls)[i];
            // don't collide with self & dead balls
            if (b1 == b2 || b2->life <= 0) continue;

            float dist = distance(b1->x, b1->y, b2->x, b2->y);
            if (dist < 2*RADIUS) {
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
    b->life--;

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

ball* spawnBall(ball (*balls)[MAX_BALLS]) {
    ball *ptr = NULL;
    for (int i = 0; i < MAX_BALLS; i++) {
        if ((*balls)[i].life <= 0) {
            ptr = &(*balls)[i];
            (*ptr).vx = 0;
            (*ptr).vy = 0;
            (*ptr).clr = randomCol();
            break;
        }
    }
    return ptr;
}
void letGoBall(ball *b, float x, float y, float px, float py) {
    if (b == NULL) return;
    b->life = LIFETIME+(rand()%40);
    b->vx = (x-px)/DELTA;
    b->vy = (y-py)/DELTA;
}

void deleteBalls(ball (*balls)[MAX_BALLS], int a) {
    for (int i = 0; a > 0 && i < MAX_BALLS; i++) {
        if ((*balls)[i].life <= 0) continue;

        (*balls)[i].life = 0;
        a--;
    }
}
void populateBalls(ball (*balls)[MAX_BALLS], int a) {
    for (int i = 0; a > 0 && i < MAX_BALLS; i++) {
        if ((*balls)[i].life > 0) continue;

        (*balls)[i].x=rand()%SCREEN_W;
        (*balls)[i].y=rand()%SCREEN_H;
        (*balls)[i].vx=rand()%25;
        (*balls)[i].vy=rand()%25;
        (*balls)[i].life = LIFETIME+(rand()%40);
        
        (*balls)[i].clr = randomCol();
        a--;
    }
}

int main() {
    ball balls[MAX_BALLS];
    srand(time(NULL));
    
    populateBalls(&balls, 128);

    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();
	consoleInit(GFX_TOP, NULL);

    HIDUSER_EnableAccelerometer();

    C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    u32 clrClear = C2D_Color32(0xEE, 0xEE, 0xEE, 0xFF);
    
    int spawnCd = 0;

    int flickFrames = 0;

    ball *dragging = NULL;

    touchPosition lastTouch;
    lastTouch.px = 0;
    lastTouch.py = 0;

    touchPosition last2ndTouch;
    last2ndTouch.px = 0;
    last2ndTouch.py = 0;

    while (aptMainLoop())
    {
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(top, clrClear);
		C2D_SceneBegin(top);

        // calculate gyroscope gravity
        accelVector v;
        hidAccelRead(&v);
        float gX = GRAVITY*(float)v.x/(-500.0f);
        float gY = GRAVITY*(float)v.z/(-500.0f);

        circlePosition c;
        circlePosition cs;
        hidCircleRead(&c);
        hidCstickRead(&cs);

        bounciness = BOUNCINESS_BASE + ((float)cs.dy/CIRCLE_MAX)*0.5f;
        gX += GSCALE*(float)c.dx/CIRCLE_MAX;
        gY -= GSCALE*(float)c.dy/CIRCLE_MAX;

        for (int i = 0; i < MAX_BALLS; i++) {
            collideBalls(&balls);
            updateBall(&balls[i], gX, gY);
            if (balls[i].life != 0 || &balls[i] == dragging) C2D_DrawCircleSolid(balls[i].x, balls[i].y, 0, RADIUS, balls[i].clr);
        }

        //Scan all the inputs. This should be done once for each frame
        hidScanInput();

        //hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
        u32 kDown = hidKeysDown();
        u32 kUp = hidKeysUp();
        u32 kHeld = hidKeysHeld();

        if ((kDown & KEY_A || kDown & KEY_B) && kHeld & KEY_A && kHeld & KEY_B) deleteBalls(&balls, MAX_BALLS);
        else if (kDown & KEY_B) deleteBalls(&balls, 1);
        // {
            // errorConf conf;
            // errorInit(&conf, ERROR_TEXT, CFG_LANGUAGE_DEFAULT);
            // errorCode(&conf, 69);
            // errorText(&conf, "420");
            // errorDisp(&conf);
        // }
        if (kDown & KEY_X) populateBalls(&balls, 4);
        
        spawnCd--;
        touchPosition touch;
        hidTouchRead(&touch);
        // Start of drag
        if (kDown & KEY_TOUCH) {
            lastTouch.px = touch.px;
            lastTouch.py = touch.py;
            last2ndTouch.px = touch.px;
            last2ndTouch.py = touch.py;
            dragging = spawnBall(&balls);
            flickFrames++;
        }
        else if (kUp & KEY_TOUCH) { // End of drag
            if (spawnCd <= 0) {
                spawnCd = COOLDOWN;
                letGoBall(dragging, lastTouch.px, lastTouch.py, last2ndTouch.px, last2ndTouch.py);
                dragging = NULL;
            }

            flickFrames = 0;
        }
        else { // During drag
            if (dragging != NULL) {
                (*dragging).x = touch.px;
                (*dragging).y = touch.py;
            }
            last2ndTouch.px = lastTouch.px;
            last2ndTouch.py = lastTouch.py;
            lastTouch.px = touch.px;
            lastTouch.py = touch.py;
        }

        if (kDown & KEY_START) break; // break in order to return to hbmenu

        printf("\x1b[2;0H %04d;%04d", c.dx, c.dy);

        // Flush and swap framebuffers
        gfxFlushBuffers();
        gfxSwapBuffers();

        //Wait for VBlank
        C3D_FrameEnd(0);
    }

    C2D_Fini();
	C3D_Fini();
	gfxExit();

    return 0;
}