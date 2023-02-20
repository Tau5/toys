#include <pspkernel.h>
#include <pspgu.h>
#include <pspctrl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pspdisplay.h>

PSP_MODULE_INFO("toy2", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_VFPU | THREAD_ATTR_USER);

#define BUFFER_WIDTH 512
#define BUFFER_HEIGHT 272
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT BUFFER_HEIGHT
#define ENEMIES 50

char list[0x20000] __attribute__((aligned(64)));

int exit_callback(int arg1, int arg2, void *common)
{
    sceKernelExitGame();
    return 0;
}

int CallbackThread(SceSize args, void* argp) {
	int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();
 
	return 0;
}

void initGu(){
    sceGuInit();

    //Set up buffers
    sceGuStart(GU_DIRECT, list);
    sceGuDrawBuffer(GU_PSM_8888,(void*)0,BUFFER_WIDTH);
    sceGuDispBuffer(SCREEN_WIDTH,SCREEN_HEIGHT,(void*)0x88000,BUFFER_WIDTH);
    sceGuDepthBuffer((void*)0x110000,BUFFER_WIDTH);

    //Set up viewport
    sceGuOffset(2048 - (SCREEN_WIDTH / 2), 2048 - (SCREEN_HEIGHT / 2));
    sceGuViewport(2048, 2048, SCREEN_WIDTH, SCREEN_HEIGHT);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuScissor(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    //Set some stuff
    sceGuDepthRange(65535, 0); //Use the full buffer for depth testing - buffer is reversed order

    sceGuDepthFunc(GU_GEQUAL); //Depth buffer is reversed, so GEQUAL instead of LEQUAL
    sceGuEnable(GU_DEPTH_TEST); //Enable depth testing

    sceGuFinish();
    sceGuDisplay(GU_TRUE);
}

void endGu(){
    sceGuDisplay(GU_FALSE);
    sceGuTerm();
}

void startFrame(unsigned int color){
    sceGuStart(GU_DIRECT, list);
    sceGuClearColor(color); // White background
    sceGuClear(GU_COLOR_BUFFER_BIT);
}

void endFrame(){
    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuSwapBuffers();
}

typedef struct {
    unsigned short u, v;
    short x, y, z;
} Vertex;

void drawRect(float x, float y, float w, float h, unsigned int color) {

    Vertex* vertices = (struct Vertex*)sceGuGetMemory(2 * sizeof(Vertex));

    vertices[0].x = x;
    vertices[0].y = y;

    vertices[1].x = x + w;
    vertices[1].y = y + h;

    sceGuColor(color); // Red, colors are ABGR
    sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, 0, vertices);
    
}

int SetupCallbacks(void) {
	int thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
	if (thid >= 0) {
		sceKernelStartThread(thid, 0, 0);
	}
	return thid;
}

bool point_in_box(int x1, int y1, int x2, int y2, int px, int py) {
    return (px >= x1 && px <= x2 && py >= y1 && py <= y2);
}

bool check_collision(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2) {
    if (point_in_box(x1, y1, x1+w1, y1+h1, x2, y2)) return true;
    if (point_in_box(x1, y1, x1+w1, y1+h1, x2+w2, y2)) return true;
    if (point_in_box(x1, y1, x1+w1, y1+h1, x2, y2+h2)) return true;
    if (point_in_box(x1, y1, x1+w1, y1+h1, x2+w2, y2+h2)) return true;
    return false;
}
struct Rect {
    int x;
    int y;
    int w;
    int h;
    unsigned int color;
};

int main() {
    sceDisplaySetMode(PSP_DISPLAY_SETBUF_IMMEDIATE, SCREEN_WIDTH, SCREEN_HEIGHT);
    SetupCallbacks();
    initGu();
    struct Rect enemies[ENEMIES];
    for (int r = 0; r < ENEMIES; r++) {
        enemies[r].color = 0xFF00FFFF;
        enemies[r].h = 10;
        enemies[r].w = 10;
        enemies[r].y = -rand()%480;
        enemies[r].x = rand()%SCREEN_WIDTH-enemies[r].w;
    }
    SceCtrlData pad;
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
    int running = 1;
    unsigned int color = 0x0;
    int x = SCREEN_WIDTH/2-5;
    int y = SCREEN_HEIGHT-20;
    while(running){
        sceCtrlReadBufferPositive(&pad, 1);
        if (pad.Buttons != 0) {
            if (pad.Buttons & PSP_CTRL_UP) {
                y--;
            }
            if (pad.Buttons & PSP_CTRL_DOWN) {
                y++;
            } 
            if (pad.Buttons & PSP_CTRL_LEFT) {
                x--;
            } 
            if (pad.Buttons & PSP_CTRL_RIGHT) {
                x++;
            }            
        }
        startFrame(color);
            bool colliding = false;
            for (int r = 0; r < ENEMIES; r++) {
                enemies[r].y+=3;
                drawRect(enemies[r].x, enemies[r].y, enemies[r].w, enemies[r].h, enemies[r].color);
                if (enemies[r].y > SCREEN_HEIGHT+20) {
                    enemies[r].x = rand()%SCREEN_WIDTH;
                    enemies[r].y = -rand()%480;
                }
                if (check_collision(enemies[r].x, enemies[r].y, enemies[r].w, enemies[r].h, x, y, 10, 10)) colliding = true;
            }
            if (colliding) {
                drawRect(x, y, 10, 10, 0xFFFF0000);
            } else {
                drawRect(x, y, 10, 10, 0xFF0000FF);
            }
            
        endFrame();
    }
    
    return 0;
}