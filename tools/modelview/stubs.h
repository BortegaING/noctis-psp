// stubs.h -- lo minimo del motor (main.cpp + pspgum) para compilar los MODELOS en la PC y ver
// como quedan sin abrir el emulador. La pila de matrices imita a pspgum (M = M * T/R, la ultima
// llamada es la mas local) y sceGumDrawArray guarda los triangulos YA transformados en g_out.
#pragma once
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#define RGBA(r, g, b, a) ((unsigned int)(((a) << 24) | ((b) << 16) | ((g) << 8) | (r)))
struct LineVertex { unsigned int color; float x, y, z; };
struct ScePspFVector3 { float x, y, z; };
struct CityBldg { float x, z, w, d, h; unsigned int color; };
#define GU_TRIANGLES 3
#define LINE_FLAGS 0
#define GU_BLEND 1
#define GU_ADD 0
#define GU_SRC_ALPHA 0
#define GU_ONE_MINUS_SRC_ALPHA 0
#define GU_TRUE 1
#define GU_FALSE 0
static inline void sceGuEnable(int) {}
static inline void sceGuDisable(int) {}
static inline void sceGuBlendFunc(int, int, int, int, int) {}
static inline void sceGuDepthMask(int) {}
static inline void *sceGuGetMemory(int n) { return malloc(n); }

static unsigned int brighten(unsigned int c, float f) {
    int r = (int)((c & 0xFF) * f), g = (int)(((c >> 8) & 0xFF) * f), b = (int)(((c >> 16) & 0xFF) * f);
    if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;
    return RGBA(r, g, b, 255);
}
static void addQuad(LineVertex *buf, int &i, float ax, float ay, float az, float bx, float by, float bz,
                    float cx2, float cy2, float cz2, float dx, float dy, float dz, unsigned int col) {
    buf[i++] = { col, ax, ay, az }; buf[i++] = { col, bx, by, bz }; buf[i++] = { col, cx2, cy2, cz2 };
    buf[i++] = { col, ax, ay, az }; buf[i++] = { col, cx2, cy2, cz2 }; buf[i++] = { col, dx, dy, dz };
}
static void addSolidBox(LineVertex *buf, int &i, float cx, float baseY, float cz, float w, float d, float h, unsigned int col) {
    const float x0 = cx - w * 0.5f, x1 = cx + w * 0.5f, z0 = cz - d * 0.5f, z1 = cz + d * 0.5f, y0 = baseY, y1 = baseY + h;
    const unsigned int top = brighten(col, 1.15f), sa = brighten(col, 0.82f), sb = brighten(col, 0.60f);
    addQuad(buf, i, x0,y1,z0, x1,y1,z0, x1,y1,z1, x0,y1,z1, top);
    addQuad(buf, i, x0,y0,z0, x1,y0,z0, x1,y1,z0, x0,y1,z0, sa);
    addQuad(buf, i, x1,y0,z1, x0,y0,z1, x0,y1,z1, x1,y1,z1, sa);
    addQuad(buf, i, x0,y0,z1, x0,y0,z0, x0,y1,z0, x0,y1,z1, sb);
    addQuad(buf, i, x1,y0,z0, x1,y0,z1, x1,y1,z1, x1,y1,z0, sb);
}
static void addPyramid(LineVertex *buf, int &i, float cx, float baseY, float cz, float w, float d, float apexH, unsigned int col) {
    const float x0 = cx - w * 0.5f, x1 = cx + w * 0.5f, z0 = cz - d * 0.5f, z1 = cz + d * 0.5f, y0 = baseY, ay = baseY + apexH;
    const unsigned int a = brighten(col, 1.0f), b = brighten(col, 0.7f);
    buf[i++] = { a, x0, y0, z0 }; buf[i++] = { a, x1, y0, z0 }; buf[i++] = { a, cx, ay, cz };
    buf[i++] = { b, x1, y0, z0 }; buf[i++] = { b, x1, y0, z1 }; buf[i++] = { b, cx, ay, cz };
    buf[i++] = { a, x1, y0, z1 }; buf[i++] = { a, x0, y0, z1 }; buf[i++] = { a, cx, ay, cz };
    buf[i++] = { b, x0, y0, z1 }; buf[i++] = { b, x0, y0, z0 }; buf[i++] = { b, cx, ay, cz };
}

// ---- pila de matrices (4x4, fila mayor; v' = M v) ----
struct Mat { float m[16]; };
static Mat g_M, g_stack[32]; static int g_sp = 0;
static std::vector<LineVertex> g_out;
static Mat matId() { Mat a; memset(a.m, 0, sizeof a.m); a.m[0] = a.m[5] = a.m[10] = a.m[15] = 1; return a; }
static Mat matMul(const Mat &a, const Mat &b) {
    Mat r; for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) { float s = 0; for (int k = 0; k < 4; ++k) s += a.m[i * 4 + k] * b.m[k * 4 + j]; r.m[i * 4 + j] = s; }
    return r;
}
static void sceGumLoadIdentity() { g_M = matId(); }
static void sceGumPushMatrix() { g_stack[g_sp++] = g_M; }
static void sceGumPopMatrix() { g_M = g_stack[--g_sp]; }
static void sceGumTranslate(const ScePspFVector3 *v) { Mat t = matId(); t.m[3] = v->x; t.m[7] = v->y; t.m[11] = v->z; g_M = matMul(g_M, t); }
static void sceGumScale(const ScePspFVector3 *v) { Mat t = matId(); t.m[0] = v->x; t.m[5] = v->y; t.m[10] = v->z; g_M = matMul(g_M, t); }
static void sceGumRotateX(float a) { Mat t = matId(); float c = cosf(a), s = sinf(a); t.m[5] = c; t.m[6] = -s; t.m[9] = s; t.m[10] = c; g_M = matMul(g_M, t); }
static void sceGumRotateY(float a) { Mat t = matId(); float c = cosf(a), s = sinf(a); t.m[0] = c; t.m[2] = s; t.m[8] = -s; t.m[10] = c; g_M = matMul(g_M, t); }
static void sceGumRotateZ(float a) { Mat t = matId(); float c = cosf(a), s = sinf(a); t.m[0] = c; t.m[1] = -s; t.m[4] = s; t.m[5] = c; g_M = matMul(g_M, t); }
static void sceGumRotateXYZ(const ScePspFVector3 *v) { sceGumRotateX(v->x); sceGumRotateY(v->y); sceGumRotateZ(v->z); }
static void sceGumDrawArray(int, int, int n, const void *, const void *verts) {
    const LineVertex *v = (const LineVertex *)verts;
    for (int k = 0; k < n; ++k) {
        LineVertex o = v[k]; const float *m = g_M.m;
        o.x = m[0] * v[k].x + m[1] * v[k].y + m[2] * v[k].z + m[3];
        o.y = m[4] * v[k].x + m[5] * v[k].y + m[6] * v[k].z + m[7];
        o.z = m[8] * v[k].x + m[9] * v[k].y + m[10] * v[k].z + m[11];
        g_out.push_back(o);
    }
}
static void dumpOut(const char *path) {
    FILE *f = fopen(path, "wb"); if (!f) { perror(path); exit(1); }
    fwrite(g_out.data(), sizeof(LineVertex), g_out.size(), f); fclose(f);
    fprintf(stderr, "%s: %zu vertices\n", path, g_out.size());
}
