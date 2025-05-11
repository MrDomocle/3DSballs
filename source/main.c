#include <3ds.h>
#include <citro2d.h>

#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include "globals.h"
#include "phys.h"

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

ball* spawnBall(ball (*balls)[MAX_BALLS], float x, float y) {
    ball *ptr = NULL;
    for (int i = 0; i < MAX_BALLS; i++) {
        if ((*balls)[i].life <= 0) {
            ptr = &(*balls)[i];
            (*ptr).x = x;
            (*ptr).y = y;
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

void drawBalls(ball (*balls)[MAX_BALLS], ball *dragging) {
    for (int i = 0; i < MAX_BALLS; i++) {
        if ((*balls)[i].life != 0 || &(*balls)[i] == dragging)
          C2D_DrawCircleSolid((*balls)[i].x, (*balls)[i].y, 0, RADIUS, (*balls)[i].clr);
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
        globalGX = GRAVITY*(float)v.x/(-500.0f);
        globalGY = GRAVITY*(float)v.z/(-500.0f);

        circlePosition c;
        circlePosition cs;
        hidCircleRead(&c);
        hidCstickRead(&cs);

        bounciness = BOUNCINESS_BASE + ((float)cs.dy/CIRCLE_MAX)*0.5f;
        globalGX += GSCALE*(float)c.dx/CIRCLE_MAX;
        globalGY -= GSCALE*(float)c.dy/CIRCLE_MAX;

        tickBalls(&balls);
        drawBalls(&balls, dragging);

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
        
        touchPosition touch;
        hidTouchRead(&touch);
        // Start of drag
        if (kDown & KEY_TOUCH && kHeld & KEY_L) { // L held - move star
            star s = {touch.px, touch.py, STAR_STRENGTH};
            gStar = &s;
        }
        else if (kDown & KEY_TOUCH) {
            lastTouch.px = touch.px;
            lastTouch.py = touch.py;
            last2ndTouch.px = touch.px;
            last2ndTouch.py = touch.py;
            dragging = spawnBall(&balls, touch.px, touch.py);
        }
        else if (kUp & KEY_TOUCH) { // End of drag
            if (dragging != NULL) { // was dragging ball
                letGoBall(dragging, lastTouch.px, lastTouch.py, last2ndTouch.px, last2ndTouch.py);
                dragging = NULL;
            }
            else gStar = NULL; // was dragging star
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

        if (gStar != NULL) printf("\x1b[2;0H %04f;%04f", gStar->x, gStar->y);

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