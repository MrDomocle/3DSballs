#include <3ds.h>
#include <citro2d.h>

#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#define MAX_BALLS 350
#define SCREEN_W 320
#define SCREEN_H 240
#define DELTA 0.2f
#define GRAVITY 9.81f
#define RADIUS 9
#define BOUNCINESS 0.9f
#define COOLDOWN 32
#define LIFETIME 1200
#define SWING_MULT 2.5f

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    u32 life;
} ball;

void updateBall(ball* b, float gX, float gY) {
    if (b->life == 0) return;
    b->life--;

    if (b->x < RADIUS) {
        b->x = RADIUS;
        b->vx *= -BOUNCINESS;
    }
    else if (b->x > (SCREEN_W - RADIUS)) {
        b->x = SCREEN_W - RADIUS;
        b->vx *= -BOUNCINESS;
    }

    if (b->y < RADIUS) {
        b->y = RADIUS;
        b->vy *= -BOUNCINESS;
    }
    else if (b->y > (SCREEN_H - RADIUS)) {
        b->y = SCREEN_H - RADIUS;
        b->vy *= -BOUNCINESS;
    }

    b->vx += DELTA*gX;
    b->vy += DELTA*gY;

    b->x += DELTA*b->vx;
    b->y += DELTA*b->vy;
}

ball* spawnBall(ball (*balls)[MAX_BALLS]) {
    ball *ptr = NULL;
    for (int i = 0; i < MAX_BALLS; i++) {
        if ((*balls)[i].life == 0) {
            ptr = &(*balls)[i];
            (*balls)[i].vx = 0;
            (*balls)[i].vy = 0;
            break;
        }
    }
    return ptr;
}
void letGoBall(ball *b, float x, float y, float px, float py) {
    b->life = LIFETIME+(rand()%40);
    b->vx = (x-px)/DELTA;
    b->vy = (y-py)/DELTA;
}

int main() {
    ball balls[MAX_BALLS];
    srand(time(NULL));
    
    for (int i = 0; i < 8; i++) {
        balls[i].x=rand()%SCREEN_W;
        balls[i].y=rand()%SCREEN_H;
        balls[i].vx=0;
        balls[i].vy=0;
        balls[i].life = LIFETIME+(rand()%40);
    }

    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();
	consoleInit(GFX_TOP, NULL);

    HIDUSER_EnableAccelerometer();

    C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    u32 clr = C2D_Color32(0xFF, 0x80, 0x80, 0xFF);
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

        for (int i = 0; i < MAX_BALLS; i++) {
            updateBall(&balls[i], gX, gY);
            if (balls[i].life != 0 || &balls[i] == dragging) C2D_DrawCircleSolid(balls[i].x, balls[i].y, 0, 16, clr);
        }

        //Scan all the inputs. This should be done once for each frame
        hidScanInput();

        //hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
        u32 kDown = hidKeysDown();
        u32 kUp = hidKeysUp();

        if (kDown & KEY_B) {
            errorConf conf;
            errorInit(&conf, ERROR_TEXT, CFG_LANGUAGE_DEFAULT);
            errorCode(&conf, 69);
            errorText(&conf, "420");
            errorDisp(&conf);
        }
        
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

        printf("\x1b[2;0H %04d;%04d;%04d", v.x, v.y, v.z);

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