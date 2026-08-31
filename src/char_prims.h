#pragma once
// PROJECT NOCTIS - primitivas ORGANICAS para el personaje (NADA de cubos).
// Se incluye en main.cpp DESPUES de: struct LineVertex; RGBA(); brighten().
// Culling esta DESACTIVADO (main.cpp:708) => el winding no importa, todo se ve.
//
// Convenciones del modelo: pies en y=0, centrado en x=0,z=0, mira hacia -Z
// (la camara esta detras). Unidades ~ metros; alto humano ~3.4-3.8.
//
// Filosofia: un humano NO son cajas. Los miembros son CILINDROS CONICOS entre
// dos puntos (pueden apuntar a cualquier direccion => poses), la cabeza es un
// ELIPSOIDE facetado, el torso/abrigo es un LOFT de anillos. Sombreado por
// normal (luz suave desde arriba-izq-frente) => volumen, no caras planas.

#include <math.h>

// ---- color helpers ----
static unsigned int cp_lerp(unsigned int a, unsigned int b, float t) {
    if (t < 0) t = 0; if (t > 1) t = 1;
    int ar=a&0xFF, ag=(a>>8)&0xFF, ab=(a>>16)&0xFF;
    int br=b&0xFF, bg=(b>>8)&0xFF, bb=(b>>16)&0xFF;
    int r=ar+(int)((br-ar)*t), g=ag+(int)((bg-ag)*t), bl=ab+(int)((bb-ab)*t);
    return RGBA(r,g,bl,255);
}
static unsigned int cp_scale(unsigned int c, float f) {
    int r=(int)((c&0xFF)*f), g=(int)(((c>>8)&0xFF)*f), b=(int)(((c>>16)&0xFF)*f);
    if(r>255)r=255; if(g>255)g=255; if(b>255)b=255;
    if(r<0)r=0; if(g<0)g=0; if(b<0)b=0;
    return RGBA(r,g,b,255);
}
// luz direccional suave, normalizada (arriba, un poco a la izq y al frente -Z)
static inline float cp_light(float nx, float ny, float nz) {
    const float lx=-0.35f, ly=0.82f, lz=-0.45f;          // ya ~normalizada
    float d = nx*lx + ny*ly + nz*lz;                     // -1..1
    float f = 0.55f + 0.55f * (d*0.5f+0.5f);             // 0.55..1.10 (nunca negro)
    return f;
}

// =====================================================================
// addLimb: CILINDRO CONICO entre p0 y p1 (cualquier orientacion).
//   r0 = radio en p0, r1 = radio en p1. sides = 6..10 (mas = mas redondo).
//   colB = color en p0, colT = color en p1 (gradiente a lo largo).
//   Cada lado se sombrea por su normal => cara iluminada / cara en sombra.
//   Verts = sides*6 (lados) + sides*6 (2 tapas) = sides*12.
// Es el CABALLO DE BATALLA: brazos, piernas, cuello, dedos, hoja, todo.
// =====================================================================
static void addLimb(LineVertex *buf, int &i,
                    float x0,float y0,float z0, float x1,float y1,float z1,
                    float r0, float r1, int sides,
                    unsigned int colB, unsigned int colT)
{
    if (sides < 3) sides = 3; if (sides > 16) sides = 16;
    float dx=x1-x0, dy=y1-y0, dz=z1-z0;
    float len=sqrtf(dx*dx+dy*dy+dz*dz); if(len<1e-5f) return;
    dx/=len; dy/=len; dz/=len;                           // eje
    // vector 'up' no paralelo al eje
    float ux,uy,uz;
    if (fabsf(dy) < 0.9f) { ux=0; uy=1; uz=0; } else { ux=1; uy=0; uz=0; }
    // u = normalize(cross(up, d))
    float ax=uy*dz-uz*dy, ay=uz*dx-ux*dz, az=ux*dy-uy*dx;
    float al=sqrtf(ax*ax+ay*ay+az*az); ax/=al; ay/=al; az/=al;
    // v = cross(d, u)
    float bx=dy*az-dz*ay, by=dz*ax-dx*az, bz=dx*ay-dy*ax;
    const float TAU = 6.2831853f;
    // centros de tapa (para los fans)
    float cx0=x0, cy0=y0, cz0=z0, cx1=x1, cy1=y1, cz1=z1;
    for (int k=0;k<sides;k++){
        float a0=TAU*k/sides, a1=TAU*(k+1)/sides;
        float c0=cosf(a0), s0=sinf(a0), c1=cosf(a1), s1=sinf(a1);
        // normales de anillo (para sombreado lateral)
        float n0x=ax*c0+bx*s0, n0y=ay*c0+by*s0, n0z=az*c0+bz*s0;
        float n1x=ax*c1+bx*s1, n1y=ay*c1+by*s1, n1z=az*c1+bz*s1;
        float f0=cp_light(n0x,n0y,n0z), f1=cp_light(n1x,n1y,n1z);
        // puntos de los dos anillos
        float p0x=x0+r0*n0x, p0y=y0+r0*n0y, p0z=z0+r0*n0z;   // ring0 @a0
        float p1x=x0+r0*n1x, p1y=y0+r0*n1y, p1z=z0+r0*n1z;   // ring0 @a1
        float q0x=x1+r1*n0x, q0y=y1+r1*n0y, q0z=z1+r1*n0z;   // ring1 @a0
        float q1x=x1+r1*n1x, q1y=y1+r1*n1y, q1z=z1+r1*n1z;   // ring1 @a1
        unsigned int cB0=cp_scale(colB,f0), cB1=cp_scale(colB,f1);
        unsigned int cT0=cp_scale(colT,f0), cT1=cp_scale(colT,f1);
        // lado (quad p0,p1,q1,q0)
        buf[i++]={cB0,p0x,p0y,p0z}; buf[i++]={cB1,p1x,p1y,p1z}; buf[i++]={cT1,q1x,q1y,q1z};
        buf[i++]={cB0,p0x,p0y,p0z}; buf[i++]={cT1,q1x,q1y,q1z}; buf[i++]={cT0,q0x,q0y,q0z};
        // tapa inferior (fan al centro p0)
        unsigned int capB=cp_scale(colB,0.72f), capT=cp_scale(colT,0.92f);
        buf[i++]={capB,cx0,cy0,cz0}; buf[i++]={capB,p1x,p1y,p1z}; buf[i++]={capB,p0x,p0y,p0z};
        // tapa superior (fan al centro p1)
        buf[i++]={capT,cx1,cy1,cz1}; buf[i++]={capT,q0x,q0y,q0z}; buf[i++]={capT,q1x,q1y,q1z};
    }
}

// =====================================================================
// addBall: ELIPSOIDE facetado (cabeza, articulaciones, pomo). Sombreado
//   por normal. stacks=lat (4..8), slices=lon (6..10).
//   Verts ~ stacks*slices*6.
// =====================================================================
static void addBall(LineVertex *buf, int &i,
                    float cx,float cy,float cz,
                    float rx,float ry,float rz,
                    int stacks,int slices, unsigned int col)
{
    if(stacks<2)stacks=2; if(slices<3)slices=3;
    const float PI=3.14159265f, TAU=6.2831853f;
    for(int st=0; st<stacks; st++){
        float t0=PI*st/stacks - PI*0.5f;      // -pi/2..pi/2
        float t1=PI*(st+1)/stacks - PI*0.5f;
        float y0=sinf(t0), y1=sinf(t1);
        float r0=cosf(t0), r1=cosf(t1);
        for(int sl=0; sl<slices; sl++){
            float a0=TAU*sl/slices, a1=TAU*(sl+1)/slices;
            float c0=cosf(a0),s0=sinf(a0),c1=cosf(a1),s1=sinf(a1);
            // 4 normales unitarias del cuad
            float n00x=r0*c0,n00y=y0,n00z=r0*s0;
            float n01x=r0*c1,n01y=y0,n01z=r0*s1;
            float n10x=r1*c0,n10y=y1,n10z=r1*s0;
            float n11x=r1*c1,n11y=y1,n11z=r1*s1;
            float f00=cp_light(n00x,n00y,n00z), f01=cp_light(n01x,n01y,n01z);
            float f10=cp_light(n10x,n10y,n10z), f11=cp_light(n11x,n11y,n11z);
            // posiciones (escaladas al elipsoide)
            #define BP(nx,ny,nz) cx+rx*(nx), cy+ry*(ny), cz+rz*(nz)
            float P00x=cx+rx*n00x,P00y=cy+ry*n00y,P00z=cz+rz*n00z;
            float P01x=cx+rx*n01x,P01y=cy+ry*n01y,P01z=cz+rz*n01z;
            float P10x=cx+rx*n10x,P10y=cy+ry*n10y,P10z=cz+rz*n10z;
            float P11x=cx+rx*n11x,P11y=cy+ry*n11y,P11z=cz+rz*n11z;
            #undef BP
            unsigned int c00=cp_scale(col,f00),c01=cp_scale(col,f01);
            unsigned int c10=cp_scale(col,f10),c11=cp_scale(col,f11);
            buf[i++]={c00,P00x,P00y,P00z}; buf[i++]={c01,P01x,P01y,P01z}; buf[i++]={c11,P11x,P11y,P11z};
            buf[i++]={c00,P00x,P00y,P00z}; buf[i++]={c11,P11x,P11y,P11z}; buf[i++]={c10,P10x,P10y,P10z};
        }
    }
}

// =====================================================================
// addLoft: apila ANILLOS {y, radio} en una superficie continua (torso que
//   estrecha en cintura, abrigo que se acampana). Encadena addLimb por banda.
//   ys[] ascendente, rs[] radios. n = nro de anillos (>=2).
//   colBot=color abajo, colTop=color arriba (gradiente vertical).
// =====================================================================
static void addLoft(LineVertex *buf, int &i, float cx, float cz,
                    const float *ys, const float *rs, int n, int sides,
                    unsigned int colBot, unsigned int colTop)
{
    if(n<2) return;
    float total = ys[n-1]-ys[0]; if(total<1e-5f) total=1.0f;
    for(int k=0;k<n-1;k++){
        float tB=(ys[k]-ys[0])/total, tT=(ys[k+1]-ys[0])/total;
        unsigned int cB=cp_lerp(colBot,colTop,tB);
        unsigned int cT=cp_lerp(colBot,colTop,tT);
        addLimb(buf,i, cx,ys[k],cz, cx,ys[k+1],cz, rs[k],rs[k+1], sides, cB, cT);
    }
}
