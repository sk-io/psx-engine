#include "clip.h"
#include "global.h"
#include "cd.h"
#include "level.h"
#include "player.h"
#include "model.h"
#include "entity.h"

#include <malloc.h>
#include <string.h>

#define FARCLIP 14000
#define CAM_EYE_OFFSET (512 << 12)

static void frame();
void load_tim_4bit(u_char *addr, u_short *tpage, u_short *clut);
void upload_clut_buf();

DB db[2];
int cdb = 0;
draw_state_t ds;
int frames = 0;

int debug_num_polys;

u_short tpage;
u_short cluts[16];
u_short clut_buf[256];

volatile int fps;
volatile int fps_counter;
volatile int fps_measure;
static int vsync_time = 0;

model_t *cube;

void vsync_cb() {
    fps_counter++;
    if (fps_counter >= 60) {
        fps = fps_measure;
        fps_measure = 0;
        fps_counter = 0;
    }
}

void load_tim_cd(char *path, u_short *tpage, u_short *clut) {
    u_char *data;

    data = cd_read_file(path, NULL);
    load_tim_4bit(data, tpage, clut);
    free3(data);
}

int main() {
    ds.clip.x = 0;
    ds.clip.y = 0;
    ds.clip.w = SCR_W;
    ds.clip.h = SCR_H;

    InitHeap3((void*) 0x800F8000, 0x00100000);

    ResetCallback();
    PadInit(0);
    ResetGraph(0);
    SetGraphDebug(0);
    CdInit(0);

    InitGeom();
    SetGeomOffset(SCR_W / 2, SCR_H / 2);
    SetGeomScreen(SCR_W / 2);  // fov

    SetBackColor(0, 0, 0);
    SetFarColor(0, 0, 0);
    SetFogNearFar(1500, 8000, SCR_W / 2);

    FntLoad(960, 256);
    SetDumpFnt(FntOpen(64, 64, 256, 200, 0, 512));
    
    SetDefDrawEnv(&db[0].draw, 0, 0,   SCR_W, SCR_H);
    SetDefDispEnv(&db[0].disp, 0, 240, SCR_W, SCR_H);
    SetDefDrawEnv(&db[1].draw, 0, 240, SCR_W, SCR_H);
    SetDefDispEnv(&db[1].disp, 0, 0,   SCR_W, SCR_H);
    db[0].draw.dfe  = db[1].draw.dfe  = 0;
    db[0].draw.isbg = db[1].draw.isbg = 1;
    db[0].draw.dtd  = db[1].draw.dtd  = 1;
    setRGB0(&db[0].draw, 0, 0, 0);
    setRGB0(&db[1].draw, 0, 0, 0);

    load_tim_cd("\\STONE.TIM;1", &tpage, &cluts[0]);
    load_tim_cd("\\GRASS.TIM;1", &tpage, &cluts[1]);
    load_tim_cd("\\MARBLE.TIM;1", &tpage, &cluts[2]);
    load_tim_cd("\\ASPHALT1.TIM;1", &tpage, &cluts[3]);
    load_tim_cd("\\BRICKS1.TIM;1", &tpage, &cluts[4]);
    load_tim_cd("\\RUST1.TIM;1", &tpage, &cluts[5]);

    upload_clut_buf();

    cube = (model_t*) cd_read_file("\\CUBE.MDL;1", NULL);
    load_model(cube);

    change_level(LEVEL_LEVEL1);

    // ENTITIES

    entities[0].type = ENT_TEST;
    entities[0].model = cube;
    entities[0].pos.vx = -250 << 12;
    entities[0].pos.vy = -1900 << 12;
    entities[0].pos.vz = -217 << 12;
    entities[0].rot.vx = 0;
    entities[0].rot.vy = 2000;
    entities[0].rot.vz = 0;

    VSyncCallback(vsync_cb);

    // enable display
    SetDispMask(1);

    init_sound();
    
    while (1) {
        frame();
        fps_measure++;
    }

    // PadStop();
    // ResetGraph(3);
    // StopCallback();
    return 0;
}

static void draw_frametime() {
    POLY_F4 *quad = (POLY_F4*) ds.pb_ptr;
    int x1 = 10 + vsync_time, y1 = 20;

    setPolyF4(quad);
    setRGB0(quad, 255, 0, 255);
    setXY4(quad, 10, 10, x1, 10, 10, y1, x1, y1);
    DrawPrim(quad);
    ds.pb_ptr = (char*) quad;
}

static void draw_hud() {
    SPRT *test = (SPRT*) ds.pb_ptr;
    DR_MODE dr_mode;

    setDrawMode(&dr_mode, 1, 1, getTPage(0, 0, 640, 0), 0);
    DrawPrim(&dr_mode);
    
    setSprt(test);
    setRGB0(test, 128, 128, 128);
    setXY0(test, 50, 50);
    setWH(test, 16, 16);
    setUV0(test, 0, 0);
    test->clut = cluts[0];
    DrawPrim(test);

    ds.pb_ptr = (char*) test;

    draw_frametime();
}

static void frame() {
    int i;
    long ret;
    long p, flags, otz;
    MATRIX mat;
    VECTOR trans = {0, 0, 0, 0};

    ent_update();

    player_update(&the_player, level.current);
    level_update_active_parts(&the_player.pos);

    // 3D stuff

    cdb = !cdb;
    ds.db = &db[cdb];
    ds.pb_ptr = ds.db->pb;

    debug_num_polys = 0;

    ClearOTagR(db[cdb].ot, OTSIZE);

    trans.vx = -the_player.pos.vx >> 12;
    trans.vy = -(the_player.pos.vy - CAM_EYE_OFFSET) >> 12;
    trans.vz = -the_player.pos.vz >> 12;

    RotMatrix(&the_player.rot, &mat);
    ApplyMatrixLV(&mat, &trans, &trans);

    TransMatrix(&mat, &trans);

    SetRotMatrix(&mat);
    SetTransMatrix(&mat);

    ds.view_persp = &mat;

    draw_level(&ds);
    ent_draw(&ds);

    DrawOTag(db[cdb].ot + OTSIZE - 1);

    // 2D stuff

    draw_hud();

    DrawSync(0);
    FntPrint("FPS: %d\n", fps);
    FntPrint("TIME: %d\n", vsync_time);
    FntPrint("POS: %d %d %d\n", the_player.pos.vx >> 12, the_player.pos.vy >> 12, the_player.pos.vz >> 12);
    FntPrint("ROT: %d %d\n", the_player.rot.vx, the_player.rot.vy);
    FntPrint("POLYS: %d\n", debug_num_polys);
    FntFlush(-1);

    DrawSync(0);
    vsync_time = VSync(0);

    PutDrawEnv(&db[cdb].draw);
    PutDispEnv(&db[cdb].disp);

    if (ds.pb_ptr - db[cdb].pb > PBSIZE) {
        printf("PACKET BUFFER OVERFLOW! %d\n", ds.pb_ptr - db[cdb].pb);
    }
    frames++;
}

void load_tim_4bit(u_char *addr, u_short *tpage, u_short *clut) {
    u_char* tex_start;
    u_long len;
    u_short x, y, w, h;

    len = *(u_long*) (addr + 0x08);
    x = *(u_short*) (addr + 0x0C);
    y = *(u_short*) (addr + 0x0E);
    w = *(u_short*) (addr + 0x10);
    h = *(u_short*) (addr + 0x12);
    
    printf("%u %u %u %u\n", x, y, w, h);

    memcpy(&clut_buf[x], addr + 0x14, 16 * sizeof(u_short));
    *clut = getClut(x, y);

    tex_start = addr + 0x08 + len;
    
    len = *(u_long*) (tex_start);
    x = *(u_short*) (tex_start + 0x04);
    y = *(u_short*) (tex_start + 0x06);
    w = *(u_short*) (tex_start + 0x08);
    h = *(u_short*) (tex_start + 0x0A);

    *tpage = LoadTPage((u_long*) (tex_start + 0x0C), 0, 0, x, y, w * 4, h);
    DrawSync(0);
}

void upload_clut_buf() {
    LoadClut((u_long*) clut_buf, CLUT_X, CLUT_Y);
    DrawSync(0);
}
