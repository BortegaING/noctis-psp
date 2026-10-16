// dump.cpp -- arma un modelo del juego en la PC y vuelca sus triangulos (ya posados) a un .bin
// que tools/modelview/view.py dibuja. Uso:
//   dump hero <out.bin> <walkPhase> <speed01> <runK> <grounded> <aiming> <idleT>
//   dump model <out.bin> <nombre>      (modelos del cementerio: ver bcmBuildByName)
//   dump scene <out.bin>               (el cementerio entero: suelo + instancias)
#include "stubs.h"
#include "../../src/char_prims.h"
#include "../../src/gfx_prims.h"
#include "../../src/cand/hunter.h"
#include "../../src/anim.h"
static LineVertex g_hero[20000];
static int g_heroV = 0;
#include "../../src/rig.h"
#define BCM_PREVIEW 1
#include "../../src/cementerio_models.h"

static LineVertex g_buf[60000];

int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "uso: dump hero|model|scene out.bin ...\n"); return 1; }
    const char *mode = argv[1], *out = argv[2];
    if (!strcmp(mode, "hero")) {
        g_heroV = build_hunter(g_hero);
        fprintf(stderr, "hunter: %d vertices (marcas:", g_heroV);
        for (int k = 0; k < HN_NMARK; ++k) fprintf(stderr, " %d", g_hnMark[k]);
        fprintf(stderr, ")\n");
        const float phase = argc > 3 ? atof(argv[3]) : 0.0f, spd = argc > 4 ? atof(argv[4]) : 0.0f;
        const float runK = argc > 5 ? atof(argv[5]) : 0.0f; const int grounded = argc > 6 ? atoi(argv[6]) : 1;
        const int aiming = argc > 7 ? atoi(argv[7]) : 0; const float idleT = argc > 8 ? atof(argv[8]) : 0.0f;
        g_hunterRunK = runK;
        HunterPose pose = hunterPose(phase, idleT, spd, grounded);
        const HunterRig rig = hunterRig(pose, phase, idleT, spd, grounded, 0, aiming, 0.0f, 0.0f);
        const float HIP = 1.70f;
        sceGumLoadIdentity();
        ScePspFVector3 fy = { 0.0f, pose.yawSway, 0.0f }; sceGumRotateXYZ(&fy);
        ScePspFVector3 bob = { pose.bobX * HUNTER_BOB_SCALE, pose.bobY * HUNTER_BOB_SCALE, 0.0f }; sceGumTranslate(&bob);
        ScePspFVector3 up = { 0.0f, HIP, 0.0f }; sceGumTranslate(&up);
        ScePspFVector3 body = { hunterPitchRad(pose), 0.0f, hunterRollRad(pose) }; sceGumRotateXYZ(&body);
        ScePspFVector3 dn = { 0.0f, -HIP, 0.0f }; sceGumTranslate(&dn);
        drawHunterRig(rig);
    } else if (!strcmp(mode, "model")) {
        int n = bcmBuildByName(argc > 3 ? argv[3] : "", g_buf, 60000);
        if (n <= 0) { fprintf(stderr, "modelo desconocido\n"); return 1; }
        sceGumLoadIdentity(); sceGumDrawArray(GU_TRIANGLES, 0, n, 0, g_buf);
    } else if (!strcmp(mode, "pool")) {   // cuanto ocupan TODOS los modelos (alto + lejos) y el suelo
        const int n = bcmBuildAll(g_buf, 60000);
        fprintf(stderr, "pool: %d vertices\n", n);
        for (int m = 0; m < BCM_N; ++m) fprintf(stderr, "  %-11s %5d  lejos %5d\n", kBcmName[m], g_bcm[m].n, g_bcm[m].loOff != g_bcm[m].off ? g_bcm[m].loN : 0);
        fprintf(stderr, "suelo: %d vertices\n", bcmBuildGround(g_buf, 60000));
    } else if (!strcmp(mode, "budget")) {   // budget out ex ez fx fz far: cuanto dibuja bc_Draw desde ahi
        bcmBuildAll(g_buf, 60000);
        static BcmInst inst[400]; const int n = bcmLayout(inst, 400);
        const float ex = atof(argv[3]), ez = atof(argv[4]), fx = atof(argv[5]), fz = atof(argv[6]), far = atof(argv[7]) + 2.0f;
        int per[BCM_N] = { 0 }, cnt[BCM_N] = { 0 }, tot = 0;
        for (int k = 0; k < n; ++k) {
            const BcmModel &M = g_bcm[inst[k].m];
            const float dx = inst[k].x - ex, dz = inst[k].z - ez, r = M.rad * inst[k].s, d = sqrtf(dx * dx + dz * dz);
            if (d - r > (r > 4.0f ? far : far - 8.0f)) continue;
            if (dx * fx + dz * fz < -r - 2.0f) continue;
            const int lo = d > (inst[k].m == BCM_ANGEL ? 13.0f : 18.0f) + 0.3f * r;
            const int v = lo ? M.loN : M.n; per[inst[k].m] += v; cnt[inst[k].m]++; tot += v;
        }
        for (int m = 0; m < BCM_N; ++m) if (cnt[m]) fprintf(stderr, "  %-11s x%2d  %6d\n", kBcmName[m], cnt[m], per[m]);
        fprintf(stderr, "instancias: %d   + suelo %d\n", tot, bcmBuildGround(g_buf, 60000));
    } else if (!strcmp(mode, "scene")) {
        bcmBuildScenePreview(g_buf, 60000);
    }
    dumpOut(out);
    return 0;
}
