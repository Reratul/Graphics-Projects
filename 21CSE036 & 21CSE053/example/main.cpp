#include <windows.h>
#include <GL/glut.h>
#include <GL/gl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// =============================================
//  GLOBAL POSITIONS
// =============================================
// World half-extent: vehicles spawn at -WORLD_HALF and wrap at +WORLD_HALF
// so they always travel fully edge-to-edge across the screen.
// The Normal camera halfW at aspect 1000/650 ≈ 107.7, so 115 gives a
// small margin that puts the spawn point just off the left edge.
const float WORLD_HALF = 115.0f;

float boatX = -WORLD_HALF;
float boatY = -6.5f;
float boatSpeed = 0.6f;

float boatBobPhase  = 0.0f;
float boatBobOffset = 0.0f;
float boatRockAngle = 0.0f;

float car1X = -WORLD_HALF;      // starts at left edge, moves right
float car2X = -WORLD_HALF * 0.5f; // starts mid-left
float car3X =  WORLD_HALF - 22.0f; // starts at right edge (car is 22 wide), moves left
float carSpeed = 2.0f;

int selectedCar = 1;
int controlMode = 0;

// ---- Bus stop state for car 2 ----
int stopStage    = 0;
float doorAngle  = 0.0f;
bool  doorOpening = false;
bool  doorClosing = false;
bool  doorFullyOpen = false;
int   waitTimer  = 0;
float autoCarSpeed = 1.2f;

const float BUS_STOP_X = 5.0f;
const int WAIT_TICKS = 167;

// =============================================
//  PSEUDO-3D DEPTH SETTINGS
// =============================================
const float DEPTH_DX = 3.2f;
const float DEPTH_DY = 2.4f;

float parallaxBuilding = 0.0f;
float parallaxTree     = 0.0f;
float parallaxRoad     = 0.0f;
float camCenterPrevX = 0.0f;

// =============================================
//  VIEWPORT / ASPECT TRACKING
// =============================================
int winW = 1000, winH = 650;

// =============================================
//  CAMERA SYSTEM
//  Cameras: 0=Normal, 1=Boat, 2=Car, 3=TopDown
//  (Angled removed)
// =============================================
#define NUM_CAMERA_MODES 4

#define CAM_NORMAL   0
#define CAM_BOAT     1
#define CAM_CAR      2
#define CAM_TOPDOWN  3

int cameraMode = CAM_NORMAL;

struct CamState {
    float l, r, b, t;
    float rotZ;
    float scaleY;
    float transY;
};

CamState camCur = { -70, 70, -70, 70, 0.0f, 1.0f, 0.0f };
CamState camTgt = { -70, 70, -70, 70, 0.0f, 1.0f, 0.0f };

const float CAM_LERP = 0.12f;

float topDownBlend = 0.0f;

// ---- Smooth rotation ----
// extraRotZ is the CURRENT smoothed value; extraRotZTarget is what
// the user has requested. Each frame we lerp current toward target.
float extraRotZ       = 0.0f;
float extraRotZTarget = 0.0f;
const float MANUAL_ROT_STEP  = 8.0f;
const float ROT_LERP_SPEED   = 0.15f;   // fraction per frame (smooth)

// ---- Smooth zoom ----
// zoomFactor multiplies the camera half-extents: <1 = zoom in, >1 = zoom out.
// zoomFactor lerps toward zoomFactorTarget each frame.
float zoomFactor       = 1.0f;
float zoomFactorTarget = 1.0f;
const float ZOOM_STEP       = 0.12f;   // additive step per key press
const float ZOOM_MIN        = 0.25f;
const float ZOOM_MAX        = 3.5f;
const float ZOOM_LERP_SPEED = 0.12f;

float lerp(float a, float b, float t) { return a + (b - a) * t; }

float aspectHalfWidth(float halfHeight)
{
    float aspect = (winH == 0) ? 1.0f : (float)winW / (float)winH;
    return halfHeight * aspect;
}

CamState buildTarget(int mode)
{
    CamState s;
    s.rotZ   = 0.0f;
    s.scaleY = 1.0f;
    s.transY = 0.0f;

    switch (mode) {
        case CAM_NORMAL: {
            float halfH = 70.0f * zoomFactor;
            float halfW = aspectHalfWidth(halfH);
            s.l = -halfW; s.r = halfW; s.b = -halfH; s.t = halfH;
            break;
        }
        case CAM_BOAT: {
            float cx = boatX;
            float halfH = 25.0f * zoomFactor;
            float halfW = aspectHalfWidth(halfH);
            s.l = cx - halfW; s.r = cx + halfW;
            s.b = boatY - 15.0f * zoomFactor;
            s.t = boatY + 35.0f * zoomFactor;
            break;
        }
        case CAM_CAR: {
            float cx = (selectedCar == 1) ? car1X
                     : (selectedCar == 3) ? car3X : car2X;
            float halfH = 21.0f * zoomFactor;
            float halfW = aspectHalfWidth(halfH);
            s.l = cx + 11.0f - halfW; s.r = cx + 11.0f + halfW;
            s.b = -32; s.t = -32 + halfH * 2.0f;
            break;
        }
        case CAM_TOPDOWN: {
            // halfH = 130 covers the full world Y range [-115..+115+car_length]
            // halfW = aspect * halfH at 1000/650 ≈ 200, which covers the
            // full top-down scene width [-200..+200] at zoom=1.
            float halfH = 130.0f * zoomFactor;
            float halfW = aspectHalfWidth(halfH);
            s.l = -halfW; s.r = halfW; s.b = -halfH; s.t = halfH;
            s.rotZ   = 0.0f;
            s.scaleY = 1.0f;
            s.transY = 0.0f;
            break;
        }
    }
    return s;
}

void stepCamera()
{
    camTgt = buildTarget(cameraMode);

    camCur.l      = lerp(camCur.l,      camTgt.l,      CAM_LERP);
    camCur.r      = lerp(camCur.r,      camTgt.r,      CAM_LERP);
    camCur.b      = lerp(camCur.b,      camTgt.b,      CAM_LERP);
    camCur.t      = lerp(camCur.t,      camTgt.t,      CAM_LERP);
    camCur.rotZ   = lerp(camCur.rotZ,   camTgt.rotZ,   CAM_LERP);
    camCur.scaleY = lerp(camCur.scaleY, camTgt.scaleY, CAM_LERP);
    camCur.transY = lerp(camCur.transY, camTgt.transY, CAM_LERP);

    // Smooth rotation: lerp current toward target every frame
    extraRotZ = lerp(extraRotZ, extraRotZTarget, ROT_LERP_SPEED);

    // Smooth zoom: lerp current toward target every frame
    zoomFactor = lerp(zoomFactor, zoomFactorTarget, ZOOM_LERP_SPEED);

    float camCenterX = (camCur.l + camCur.r) * 0.5f;
    float dCam = camCenterX - camCenterPrevX;
    camCenterPrevX = camCenterX;

    parallaxBuilding += dCam * 0.10f;
    parallaxTree     += dCam * 0.22f;
    parallaxRoad     += dCam * 0.0f;

    float blendTarget = (cameraMode == CAM_TOPDOWN) ? 1.0f : 0.0f;
    topDownBlend = lerp(topDownBlend, blendTarget, CAM_LERP);
}

void applyCameraView()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(camCur.l, camCur.r, camCur.b, camCur.t, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float effRotZ = camCur.rotZ + extraRotZ;

    if (fabsf(effRotZ) > 0.01f || fabsf(camCur.scaleY - 1.0f) > 0.001f) {
        glTranslatef(0.0f, camCur.transY, 0.0f);
        glRotatef(effRotZ, 0.0f, 0.0f, 1.0f);
        glScalef(1.0f, camCur.scaleY, 1.0f);
        glTranslatef(0.0f, -camCur.transY, 0.0f);
    }
}

// =============================================
//  SMOKE PARTICLES
// =============================================
#define MAX_SMOKE 40

struct SmokeParticle {
    float x, y;
    float vy;
    float alpha;
    float size;
    bool  active;
};

SmokeParticle smoke[MAX_SMOKE];
int   smokeSpawnTimer = 0;
bool  boatMoving      = true;

void initSmoke()
{
    for (int i = 0; i < MAX_SMOKE; i++)
        smoke[i].active = false;
}

void spawnSmoke(float sx, float sy)
{
    for (int i = 0; i < MAX_SMOKE; i++) {
        if (!smoke[i].active) {
            smoke[i].x     = sx + ((rand() % 100) / 100.0f - 0.5f) * 0.8f;
            smoke[i].y     = sy;
            smoke[i].vy    = 0.08f + (rand() % 100) / 100.0f * 0.06f;
            smoke[i].alpha = 0.75f;
            smoke[i].size  = 1.2f + (rand() % 100) / 100.0f * 1.0f;
            smoke[i].active = true;
            break;
        }
    }
}

void updateSmoke()
{
    for (int i = 0; i < MAX_SMOKE; i++) {
        if (!smoke[i].active) continue;
        smoke[i].y     += smoke[i].vy;
        smoke[i].alpha -= 0.012f;
        smoke[i].size  += 0.04f;
        if (smoke[i].alpha <= 0.0f)
            smoke[i].active = false;
    }
}

void drawSmoke()
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (int i = 0; i < MAX_SMOKE; i++) {
        if (!smoke[i].active) continue;
        float s = smoke[i].size;
        float a = smoke[i].alpha;
        glColor4f(0.3f, 0.3f, 0.3f, a);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(smoke[i].x, smoke[i].y);
        for (int j = 0; j <= 20; j++) {
            float ang = 2.0f * 3.14159f * j / 20;
            glVertex2f(smoke[i].x + s * cosf(ang),
                       smoke[i].y + s * sinf(ang));
        }
        glEnd();
    }
    glDisable(GL_BLEND);
}

// =============================================
//  BOAT SOUND
// =============================================
volatile bool boatSoundShouldPlay = true;
volatile bool boatSoundThreadRunning = false;

DWORD WINAPI continuousBoatSound(LPVOID)
{
    while (boatSoundShouldPlay) {
        Beep(220, 140);
        Beep(260, 90);
        Sleep(120);
    }
    boatSoundThreadRunning = false;
    return 0;
}

void startBoatSound()
{
    if (boatSoundThreadRunning) return;
    boatSoundShouldPlay    = true;
    boatSoundThreadRunning = true;
    HANDLE hThread = CreateThread(NULL, 0, continuousBoatSound, NULL, 0, NULL);
    if (hThread) CloseHandle(hThread);
}

// =============================================
//  UTILITY
// =============================================
void circle(float rx, float ry, float cx, float cy)
{
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= 100; i++) {
        float a = 2.0f * 3.14159f * i / 100;
        glVertex2f(cx + rx * cosf(a), cy + ry * sinf(a));
    }
    glEnd();
}

void rect(float x1, float y1, float x2, float y2)
{
    glBegin(GL_QUADS);
    glVertex2f(x1, y1);
    glVertex2f(x2, y1);
    glVertex2f(x2, y2);
    glVertex2f(x1, y2);
    glEnd();
}

void extrudeTop(float x1, float y2, float x2, float dx, float dy,
                 float r, float g, float b)
{
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex2f(x1,      y2);
    glVertex2f(x2,      y2);
    glVertex2f(x2 + dx, y2 + dy);
    glVertex2f(x1 + dx, y2 + dy);
    glEnd();
}

void extrudeSide(float x2, float y1, float y2, float dx, float dy,
                  float r, float g, float b)
{
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex2f(x2,      y1);
    glVertex2f(x2,      y2);
    glVertex2f(x2 + dx, y2 + dy);
    glVertex2f(x2 + dx, y1 + dy);
    glEnd();
}

// =============================================
//  SUN
// =============================================
void sun()
{
    float cx = 32.0f, cy = 42.0f;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f, 0.95f, 0.4f, 0.18f);
    circle(9.0f, 9.0f, cx, cy);
    glColor4f(1.0f, 0.92f, 0.3f, 0.28f);
    circle(7.0f, 7.0f, cx, cy);
    glDisable(GL_BLEND);
    glColor3f(1.0f, 0.88f, 0.2f);
    for (int i = 0; i < 8; i++) {
        float base = i * 3.14159f / 4.0f;
        float half = 0.22f;
        float r1 = 5.5f, r2 = 8.8f;
        glBegin(GL_TRIANGLES);
        glVertex2f(cx + r1 * cosf(base - half), cy + r1 * sinf(base - half));
        glVertex2f(cx + r1 * cosf(base + half), cy + r1 * sinf(base + half));
        glVertex2f(cx + r2 * cosf(base),        cy + r2 * sinf(base));
        glEnd();
    }
    glColor3f(1.0f, 0.92f, 0.1f);
    circle(5.0f, 5.0f, cx, cy);
    glColor3f(1.0f, 1.0f, 0.8f);
    circle(2.2f, 2.2f, cx - 0.8f, cy + 0.8f);
}

// =============================================
//  ROUND TREE
// =============================================
void roundTree(float x, float groundY)
{
    float px = x + parallaxTree;

    glColor3f(0.45f, 0.28f, 0.05f);
    rect(px - 1.2f, groundY, px + 1.2f, groundY + 7.0f);
    extrudeSide(px + 1.2f, groundY, groundY + 7.0f, 0.9f, 0.6f,
                0.30f, 0.18f, 0.03f);

    glColor3f(0.09f, 0.42f, 0.09f);
    circle(4.3f, 3.9f, px - 0.8f, groundY + 11.6f);
    circle(3.3f, 3.3f, px - 4.1f, groundY + 10.1f);
    circle(3.3f, 3.3f, px + 2.9f, groundY + 10.1f);

    glColor3f(0.13f, 0.55f, 0.13f);
    circle(4.5f, 4.0f, px,        groundY + 12.0f);
    circle(3.5f, 3.5f, px - 3.5f, groundY + 10.5f);
    circle(3.5f, 3.5f, px + 3.5f, groundY + 10.5f);
    circle(3.0f, 3.0f, px - 2.0f, groundY + 14.5f);
    circle(3.0f, 3.0f, px + 2.0f, groundY + 14.5f);

    glColor3f(0.22f, 0.68f, 0.20f);
    circle(3.5f, 3.0f, px,        groundY + 16.0f);
    circle(1.6f, 1.3f, px - 0.9f, groundY + 16.8f);
}

// =============================================
//  PINE TREE
// =============================================
void pineTree(float x, float groundY)
{
    float px = x + parallaxTree;

    glColor3f(0.45f, 0.28f, 0.05f);
    rect(px - 1.0f, groundY, px + 1.0f, groundY + 5.0f);
    extrudeSide(px + 1.0f, groundY, groundY + 5.0f, 0.8f, 0.55f,
                0.30f, 0.18f, 0.03f);

    glColor3f(0.03f, 0.28f, 0.03f);
    glBegin(GL_TRIANGLES);
    glVertex2f(px - 6.6f + 0.6f, groundY + 5.0f + 0.4f);
    glVertex2f(px + 6.6f + 0.6f, groundY + 5.0f + 0.4f);
    glVertex2f(px + 0.6f,        groundY + 13.0f + 0.4f);
    glEnd();

    glColor3f(0.05f, 0.40f, 0.05f);
    glBegin(GL_TRIANGLES);
    glVertex2f(px - 7.0f, groundY + 5.0f);
    glVertex2f(px + 7.0f, groundY + 5.0f);
    glVertex2f(px,        groundY + 13.0f);
    glEnd();
    glColor3f(0.06f, 0.45f, 0.06f);
    glBegin(GL_TRIANGLES);
    glVertex2f(px - 5.5f, groundY + 9.0f);
    glVertex2f(px + 5.5f, groundY + 9.0f);
    glVertex2f(px,        groundY + 17.0f);
    glEnd();
    glColor3f(0.08f, 0.50f, 0.08f);
    glBegin(GL_TRIANGLES);
    glVertex2f(px - 4.0f, groundY + 13.0f);
    glVertex2f(px + 4.0f, groundY + 13.0f);
    glVertex2f(px,        groundY + 20.0f);
    glEnd();

    glColor4f(0.35f, 0.75f, 0.25f, 0.35f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_TRIANGLES);
    glVertex2f(px,        groundY + 20.0f);
    glVertex2f(px + 4.0f, groundY + 13.0f);
    glVertex2f(px + 1.2f, groundY + 13.0f);
    glEnd();
    glDisable(GL_BLEND);
}

// =============================================
//  BUILDING
// =============================================
void building(float x1, float y1, float x2, float y2,
              float r, float g, float b, int cols, int rows)
{
    float px1 = x1 + parallaxBuilding;
    float px2 = x2 + parallaxBuilding;

    float dx = DEPTH_DX, dy = DEPTH_DY;

    extrudeSide(px2, y1, y2, dx, dy, r * 0.45f, g * 0.45f, b * 0.45f);
    extrudeTop(px1, y2, px2, dx, dy, r * 1.25f > 1 ? 1 : r * 1.25f,
                                       g * 1.25f > 1 ? 1 : g * 1.25f,
                                       b * 1.25f > 1 ? 1 : b * 1.25f);

    glColor3f(r, g, b);
    rect(px1, y1, px2, y2);
    glColor3f(r * 0.6f, g * 0.6f, b * 0.6f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(px1, y1); glVertex2f(px2, y1);
    glVertex2f(px2, y2); glVertex2f(px1, y2);
    glEnd();

    glColor3f(r * 0.7f, g * 0.7f, b * 0.7f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(px1, y2); glVertex2f(px2, y2);
    glVertex2f(px2 + dx, y2 + dy); glVertex2f(px1 + dx, y2 + dy);
    glEnd();

    float bw = px2 - px1, bh = y2 - y1;
    float pw = bw / (cols * 2 + 1);
    float ph = bh / (rows * 2 + 1);
    for (int c = 0; c < cols; c++) {
        for (int ro = 0; ro < rows; ro++) {
            float wx1 = px1 + pw + c * 2 * pw;
            float wy1 = y1 + ph + ro * 2 * ph;
            float wx2 = wx1 + pw, wy2 = wy1 + ph;
            glColor3f(0.8f, 0.9f, 1.0f);
            rect(wx1, wy1, wx2, wy2);
            glColor3f(0.5f, 0.5f, 0.5f);
            glBegin(GL_LINES);
            glVertex2f((wx1+wx2)/2, wy1); glVertex2f((wx1+wx2)/2, wy2);
            glVertex2f(wx1, (wy1+wy2)/2); glVertex2f(wx2, (wy1+wy2)/2);
            glEnd();
        }
    }
}

// =============================================
//  ZIGZAG
// =============================================
void zigzag(float xStart, float xEnd, float y, float amp, float freq)
{
    glColor3f(0.1f, 0.55f, 0.1f);
    glBegin(GL_TRIANGLE_STRIP);
    int steps = (int)((xEnd - xStart) * freq);
    for (int i = 0; i <= steps; i++) {
        float x = xStart + (xEnd - xStart) * i / steps;
        float peak = (i % 2 == 0) ? y + amp : y;
        glVertex2f(x, y);
        glVertex2f(x, peak);
    }
    glEnd();
}

// =============================================
//  STOP SIGN
// =============================================
void stopSign(float x)
{
    glColor3f(0.6f, 0.6f, 0.6f);
    rect(x - 0.3f, -28.0f, x + 0.3f, -17.0f);
    glColor3f(0.42f, 0.42f, 0.42f);
    rect(x + 0.1f, -28.0f, x + 0.3f, -17.0f);

    float cx = x, cy = -14.5f, r = 3.5f;

    glColor3f(0.55f, 0.03f, 0.03f);
    glBegin(GL_POLYGON);
    for (int i = 0; i < 8; i++) {
        float angle = 3.14159f / 8.0f + i * 3.14159f / 4.0f;
        glVertex2f(cx + 0.5f + r * cosf(angle), cy - 0.3f + r * sinf(angle));
    }
    glEnd();

    glColor3f(0.85f, 0.05f, 0.05f);
    glBegin(GL_POLYGON);
    for (int i = 0; i < 8; i++) {
        float angle = 3.14159f / 8.0f + i * 3.14159f / 4.0f;
        glVertex2f(cx + r * cosf(angle), cy + r * sinf(angle));
    }
    glEnd();
    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 8; i++) {
        float angle = 3.14159f / 8.0f + i * 3.14159f / 4.0f;
        glVertex2f(cx + r * cosf(angle), cy + r * sinf(angle));
    }
    glEnd();
    glColor3f(1.0f, 1.0f, 1.0f);
    float tx = cx - 2.8f, ty = cy - 0.9f;
    rect(tx,        ty + 1.6f, tx + 1.3f, ty + 1.9f);
    rect(tx,        ty + 0.8f, tx + 1.3f, ty + 1.1f);
    rect(tx,        ty,        tx + 1.3f, ty + 0.3f);
    rect(tx,        ty + 0.8f, tx + 0.3f, ty + 1.6f);
    rect(tx + 1.0f, ty,        tx + 1.3f, ty + 0.8f);
    tx += 1.6f;
    rect(tx,        ty + 1.6f, tx + 1.3f, ty + 1.9f);
    rect(tx + 0.5f, ty,        tx + 0.8f, ty + 1.6f);
    tx += 1.6f;
    rect(tx,        ty + 1.6f, tx + 1.3f, ty + 1.9f);
    rect(tx,        ty,        tx + 1.3f, ty + 0.3f);
    rect(tx,        ty,        tx + 0.3f, ty + 1.9f);
    rect(tx + 1.0f, ty,        tx + 1.3f, ty + 1.9f);
    tx += 1.6f;
    rect(tx,        ty,        tx + 0.3f, ty + 1.9f);
    rect(tx,        ty + 1.6f, tx + 1.3f, ty + 1.9f);
    rect(tx,        ty + 0.8f, tx + 1.3f, ty + 1.1f);
    rect(tx + 1.0f, ty + 1.1f, tx + 1.3f, ty + 1.6f);
    glColor3f(1.0f, 1.0f, 1.0f);
    rect(x - 4.0f, -16.6f, x + 4.0f, -15.6f);
    glColor3f(0.05f, 0.35f, 0.85f);
    rect(x - 3.7f, -16.4f, x + 3.7f, -15.8f);
}

// =============================================
//  BOAT
// =============================================
void boat(float ox, float oy, float rockDeg, float bobOffset)
{
    glPushMatrix();
    glTranslatef(ox, oy + bobOffset, 0.0f);
    glRotatef(rockDeg, 0.0f, 0.0f, 1.0f);
    glTranslatef(-ox, -(oy + bobOffset), 0.0f);

    float hdx = 2.6f, hdy = 1.8f;

    glColor3f(0.38f, 0.18f, 0.0f);
    glBegin(GL_POLYGON);
    glVertex2f(ox - 15.0f + hdx, oy + bobOffset + hdy);
    glVertex2f(ox + 15.0f + hdx, oy + bobOffset + hdy);
    glVertex2f(ox + 18.0f + hdx, oy + bobOffset - 5.0f + hdy);
    glVertex2f(ox - 12.0f + hdx, oy + bobOffset - 5.0f + hdy);
    glEnd();

    glColor3f(0.6f, 0.3f, 0.0f);
    glBegin(GL_POLYGON);
    glVertex2f(ox - 15.0f, oy + bobOffset);
    glVertex2f(ox + 15.0f, oy + bobOffset);
    glVertex2f(ox + 18.0f, oy + bobOffset - 5.0f);
    glVertex2f(ox - 12.0f, oy + bobOffset - 5.0f);
    glEnd();

    glColor3f(0.78f, 0.42f, 0.05f);
    glBegin(GL_QUADS);
    glVertex2f(ox - 15.0f, oy + bobOffset);
    glVertex2f(ox + 15.0f, oy + bobOffset);
    glVertex2f(ox + 15.0f, oy + bobOffset + 0.5f);
    glVertex2f(ox - 15.0f, oy + bobOffset + 0.5f);
    glEnd();

    extrudeSide(ox + 10.0f, oy + bobOffset, oy + bobOffset + 7.0f, hdx, hdy,
                0.55f, 0.55f, 0.55f);
    glColor3f(0.85f, 0.85f, 0.85f);
    rect(ox - 10.0f, oy + bobOffset, ox + 10.0f, oy + bobOffset + 7.0f);
    glColor3f(0.6f, 0.85f, 1.0f);
    rect(ox - 8.0f, oy+bobOffset+1.5f, ox - 4.5f, oy+bobOffset+5.5f);
    rect(ox - 2.5f, oy+bobOffset+1.5f, ox + 2.5f, oy+bobOffset+5.5f);
    rect(ox + 4.5f, oy+bobOffset+1.5f, ox + 8.0f, oy+bobOffset+5.5f);

    extrudeSide(ox + 8.0f, oy + bobOffset + 7.0f, oy + bobOffset + 11.0f,
                hdx * 0.8f, hdy * 0.8f, 0.42f, 0.42f, 0.42f);
    extrudeTop(ox - 6.0f, oy + bobOffset + 11.0f, ox + 8.0f,
               hdx * 0.8f, hdy * 0.8f, 0.85f, 0.85f, 0.85f);
    glColor3f(0.7f, 0.7f, 0.7f);
    rect(ox - 6.0f, oy + bobOffset + 7.0f, ox + 8.0f, oy + bobOffset + 11.0f);
    glColor3f(0.6f, 0.85f, 1.0f);
    rect(ox - 4.5f, oy+bobOffset+8.0f, ox - 1.5f, oy+bobOffset+10.0f);
    rect(ox + 1.0f, oy+bobOffset+8.0f, ox + 5.5f, oy+bobOffset+10.0f);

    extrudeSide(ox + 6.0f, oy+bobOffset+11.0f, oy+bobOffset+14.5f, 1.2f, 0.9f,
                0.25f, 0.25f, 0.25f);
    glColor3f(0.4f, 0.4f, 0.4f);
    rect(ox + 3.0f, oy+bobOffset+11.0f, ox + 6.0f, oy+bobOffset+14.5f);

    glColor3f(0.55f, 0.25f, 0.0f);
    glBegin(GL_TRIANGLES);
    glVertex2f(ox+15.0f, oy+bobOffset);
    glVertex2f(ox+20.0f, oy+bobOffset-2.5f);
    glVertex2f(ox+15.0f, oy+bobOffset-5.0f);
    glEnd();

    glPopMatrix();
}

// =============================================
//  CAR
// =============================================
void car(float ox, float oy, float r, float g, float b, float doorOpenAmt = 0.0f)
{
    float W = 22.0f, H = 6.0f;
    const float dx = 3.0f;
    const float dy = 2.0f;

    glColor3f(r, g, b);
    rect(ox, oy, ox + W, oy + H);

    glColor3f(
        r * 1.15f > 1.0f ? 1.0f : r * 1.15f,
        g * 1.15f > 1.0f ? 1.0f : g * 1.15f,
        b * 1.15f > 1.0f ? 1.0f : b * 1.15f
    );
    glBegin(GL_QUADS);
    glVertex2f(ox, oy + H);
    glVertex2f(ox + W, oy + H);
    glVertex2f(ox + W + dx, oy + H + dy);
    glVertex2f(ox + dx, oy + H + dy);
    glEnd();

    glColor3f(r * 0.65f, g * 0.65f, b * 0.65f);
    glBegin(GL_QUADS);
    glVertex2f(ox + W, oy);
    glVertex2f(ox + W, oy + H);
    glVertex2f(ox + W + dx, oy + H + dy);
    glVertex2f(ox + W + dx, oy + dy);
    glEnd();

    glColor3f(r * 0.85f, g * 0.85f, b * 0.85f);
    glBegin(GL_QUADS);
    glVertex2f(ox + 3.0f,     oy + H);
    glVertex2f(ox + W - 2.0f, oy + H);
    glVertex2f(ox + W - 5.0f, oy + H + 5.0f);
    glVertex2f(ox + 6.0f,     oy + H + 5.0f);
    glEnd();

    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex2f(ox + 6.0f,          oy + H + 5.0f);
    glVertex2f(ox + W - 5.0f,      oy + H + 5.0f);
    glVertex2f(ox + W - 5.0f + dx, oy + H + 5.0f + dy);
    glVertex2f(ox + 6.0f + dx,     oy + H + 5.0f + dy);
    glEnd();

    glColor3f(0.6f, 0.88f, 1.0f);
    rect(ox + 6.5f,  oy + H + 0.8f, ox + 10.5f,    oy + H + 4.2f);
    rect(ox + 11.5f, oy + H + 0.8f, ox + W - 5.5f, oy + H + 4.2f);

    glColor3f(0.45f, 0.75f, 0.95f);
    glBegin(GL_QUADS);
    glVertex2f(ox + W - 5.5f,      oy + H + 0.8f);
    glVertex2f(ox + W - 5.5f + dx, oy + H + 0.8f + dy);
    glVertex2f(ox + W - 5.5f + dx, oy + H + 3.8f + dy);
    glVertex2f(ox + W - 5.5f,      oy + H + 3.8f);
    glEnd();

    glColor3f(0.25f, 0.25f, 0.25f);
    glBegin(GL_LINES);
    glVertex2f(ox + 11.0f, oy + H + 0.8f);
    glVertex2f(ox + 11.0f, oy + H + 4.2f);
    glEnd();

    glColor3f(0.08f, 0.08f, 0.08f);
    circle(3.5f, 3.5f, ox + 5.5f,     oy - 0.5f);
    circle(3.5f, 3.5f, ox + W - 4.5f, oy - 0.5f);
    glColor3f(0.70f, 0.70f, 0.70f);
    circle(1.5f, 1.5f, ox + 5.5f,     oy - 0.5f);
    circle(1.5f, 1.5f, ox + W - 4.5f, oy - 0.5f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f, 0.15f);
    glBegin(GL_QUADS);
    glVertex2f(ox + 2.0f,     oy - 3.8f);
    glVertex2f(ox + W + 2.0f, oy - 3.8f);
    glVertex2f(ox + W + 5.0f, oy - 2.6f);
    glVertex2f(ox + 5.0f,     oy - 2.6f);
    glEnd();
    glDisable(GL_BLEND);

    glColor3f(1.0f, 1.0f, 0.7f);
    rect(ox + W - 1.5f, oy + 1.5f, ox + W + 0.5f, oy + 3.5f);
    glColor3f(1.0f, 0.15f, 0.15f);
    rect(ox - 0.5f, oy + 1.5f, ox + 1.0f, oy + 3.5f);

    if (doorOpenAmt > 0.0f)
    {
        float t      = doorOpenAmt;
        float eased  = t * t * (3.0f - 2.0f * t);

        float doorW     = 7.0f;
        float doorBodyH = H;
        float doorTopH  = doorBodyH + 4.2f;
        float hingeX    = ox + 3.5f;
        float hingeY    = oy + doorTopH;
        float angle     = eased * -70.0f;

        glColor3f(0.05f, 0.05f, 0.05f);
        rect(hingeX, oy, hingeX + doorW, oy + doorTopH);

        glPushMatrix();
        glTranslatef(hingeX, hingeY, 0.0f);
        glRotatef(angle, 0.0f, 0.0f, 1.0f);
        glTranslatef(-hingeX, -hingeY, 0.0f);

        glColor3f(r * 0.78f, g * 0.78f, b * 0.78f);
        rect(hingeX, oy, hingeX + doorW, oy + doorBodyH);

        glColor3f(0.55f, 0.82f, 1.0f);
        rect(hingeX + 0.6f,
             oy + doorBodyH + 0.4f,
             hingeX + doorW - 0.6f,
             oy + doorTopH  - 0.3f);

        glPopMatrix();
    }
}

// =============================================
//  RIVER
// =============================================
void river(float y1, float y2)
{
    glColor3f(0.18f, 0.52f, 0.78f);
    rect(-115.0f, y1, 115.0f, y2);

    glColor3f(0.13f, 0.40f, 0.62f);
    rect(-115.0f, y2 - 1.2f, 115.0f, y2);

    glColor3f(0.30f, 0.66f, 0.92f);
    rect(-115.0f, y1, 115.0f, y1 + 0.8f);

    float blues[][3] = {
        {0.22f,0.58f,0.85f},{0.26f,0.62f,0.88f},
        {0.20f,0.54f,0.80f},{0.24f,0.60f,0.86f},{0.19f,0.50f,0.76f}
    };
    for (int i = 0; i < 5; i++) {
        float y = y1 + (y2 - y1) * (i + 0.5f) / 5;
        glColor3f(blues[i][0], blues[i][1], blues[i][2]);
        glLineWidth(1.0f);
        glBegin(GL_LINES);
        glVertex2f(-115.0f, y); glVertex2f(115.0f, y);
        glEnd();
    }
}

// =============================================
//  SELECTION INDICATOR
// =============================================
void drawSelector(float cx, float cy)
{
    glColor3f(1.0f, 1.0f, 0.0f);
    glLineWidth(2.0f);
    glBegin(GL_TRIANGLES);
    glVertex2f(cx,        cy + 0.5f);
    glVertex2f(cx - 0.8f, cy + 1.8f);
    glVertex2f(cx + 0.8f, cy + 1.8f);
    glEnd();
}

// =============================================
//  ROAD
// =============================================
void drawRoad()
{
    glColor3f(0.22f, 0.22f, 0.22f);
    rect(-115.0f, -28.0f, 115.0f, -14.0f);

    glColor3f(0.18f, 0.18f, 0.18f);
    rect(-115.0f, -17.0f, 115.0f, -14.0f);
    glColor3f(0.26f, 0.26f, 0.26f);
    rect(-115.0f, -28.0f, 115.0f, -25.5f);

    glColor3f(0.75f, 0.75f, 0.72f);
    rect(-115.0f, -14.5f, 115.0f, -14.0f);
    glColor3f(0.55f, 0.55f, 0.52f);
    rect(-115.0f, -14.7f, 115.0f, -14.5f);

    glColor3f(0.75f, 0.75f, 0.72f);
    rect(-115.0f, -28.0f, 115.0f, -27.5f);
    glColor3f(0.55f, 0.55f, 0.52f);
    rect(-115.0f, -27.5f, 115.0f, -27.3f);

    glColor3f(0.9f, 0.9f, 0.2f);
    glLineWidth(2.0f);
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(2, 0x00FF);
    glBegin(GL_LINES);
    glVertex2f(-115.0f, -21.0f); glVertex2f(115.0f, -21.0f);
    glEnd();
    glDisable(GL_LINE_STIPPLE);
}

// =============================================
//  TOP-DOWN PLAN VIEW
//  Now includes proper cars drawn in their road lanes
// =============================================
struct PlanBuilding { float x1, x2, footY1, footY2, r, g, b; };

PlanBuilding planBuildings[] = {
    { -100.0f, -90.0f, 22.0f, 38.0f, 0.68f, 0.58f, 0.50f },
    { -89.0f,  -78.0f, 22.0f, 34.0f, 0.72f, 0.62f, 0.52f },
    { -77.0f,  -65.0f, 22.0f, 39.0f, 0.65f, 0.55f, 0.48f },
    { -50.0f,  -40.0f, 22.0f, 34.0f, 0.72f, 0.60f, 0.50f },
    { -39.0f,  -28.0f, 22.0f, 38.0f, 0.65f, 0.55f, 0.48f },
    { -15.0f,   -4.0f, 22.0f, 36.0f, 0.70f, 0.62f, 0.54f },
    {  -3.0f,    8.0f, 22.0f, 40.0f, 0.68f, 0.56f, 0.46f },
    {   9.0f,   20.0f, 22.0f, 34.0f, 0.72f, 0.60f, 0.50f },
    {  21.0f,   33.0f, 22.0f, 38.0f, 0.65f, 0.55f, 0.48f },
    {  34.0f,   50.0f, 22.0f, 36.0f, 0.70f, 0.58f, 0.46f },
    {  64.0f,   76.0f, 22.0f, 39.0f, 0.65f, 0.55f, 0.48f },
    {  77.0f,   88.0f, 22.0f, 34.0f, 0.72f, 0.62f, 0.52f },
    {  89.0f,  100.0f, 22.0f, 38.0f, 0.68f, 0.58f, 0.50f },
};
const int NUM_PLAN_BUILDINGS = sizeof(planBuildings) / sizeof(planBuildings[0]);

void planRoofShadow(float x1, float y1, float x2, float y2)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f, 0.18f);
    rect(x1 + 0.8f, y1 - 0.8f, x2 + 0.8f, y2 - 0.8f);
    glDisable(GL_BLEND);
}

// Draw a car in top-down view.
//   worldX  = the car's world X position → used as Y on screen (road runs vertically)
//   laneX   = fixed X position within the road band for this car's lane
//   facingRight: in side-view this means moving +X. In top-down (road vertical),
//               that means moving in the +Y direction (downward on screen).
void drawTopDownCar(float worldX, float laneX,
                    float bodyR, float bodyG, float bodyB,
                    bool  facingDown,      // car1 & car2 go +Y (right in world = down in topdown)
                    bool  showOpenDoor)
{
    // Car body dimensions in top-down (road runs Y):
    //   carHL = half-length along road (Y axis)
    //   carHW = half-width across road (X axis)
    float carHL = 11.0f;
    float carHW =  4.5f;

    // worldX becomes the Y centre; laneX is the X centre within the band
    float cx = laneX;
    float cy = worldX + carHL;   // shift so worldX is the trailing edge

    float x1 = cx - carHW, x2 = cx + carHW;
    float y1 = cy - carHL, y2 = cy + carHL;

    // Drop shadow
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f, 0.18f);
    rect(x1 + 0.6f, y1 + 0.6f, x2 + 0.6f, y2 + 0.6f);
    glDisable(GL_BLEND);

    // Body
    glColor3f(bodyR, bodyG, bodyB);
    rect(x1, y1, x2, y2);

    // Roof (slightly darker)
    glColor3f(bodyR * 0.72f, bodyG * 0.72f, bodyB * 0.72f);
    rect(x1 + 0.9f, y1 + 2.5f, x2 - 0.9f, y2 - 2.5f);

    // Windshield band near the front end
    float windY1 = facingDown ? y2 - 5.0f : y1;
    float windY2 = facingDown ? y2        : y1 + 5.0f;
    glColor3f(0.65f, 0.88f, 1.0f);
    rect(x1 + 0.6f, windY1, x2 - 0.6f, windY2);

    // Headlights (yellow) at the front
    glColor3f(1.0f, 1.0f, 0.55f);
    float hy = facingDown ? y2 : y1;
    circle(0.8f, 0.8f, cx - 2.8f, hy);
    circle(0.8f, 0.8f, cx + 2.8f, hy);

    // Tail lights (red) at the rear
    glColor3f(1.0f, 0.1f, 0.1f);
    float ty2 = facingDown ? y1 : y2;
    circle(0.65f, 0.65f, cx - 2.5f, ty2);
    circle(0.65f, 0.65f, cx + 2.5f, ty2);

    // Wheels at corners
    glColor3f(0.12f, 0.12f, 0.12f);
    circle(1.3f, 1.3f, x1, y1 + 2.5f);
    circle(1.3f, 1.3f, x2, y1 + 2.5f);
    circle(1.3f, 1.3f, x1, y2 - 2.5f);
    circle(1.3f, 1.3f, x2, y2 - 2.5f);

    // Open door (car2 only): swings outward to the right of the road lane
    if (showOpenDoor && doorAngle > 0.0f) {
        float eased = doorAngle * doorAngle * (3.0f - 2.0f * doorAngle);
        float doorLen = 6.5f * eased;
        glColor3f(bodyR * 0.78f, bodyG * 0.78f, bodyB * 0.78f);
        rect(x2, y1 + 2.5f, x2 + doorLen, y1 + 8.5f);
    }
}

void drawTopDownScene()
{
    // Ground
    glColor3f(0.22f, 0.50f, 0.18f);
    rect(-200.0f, -200.0f, 200.0f, 200.0f);

    // Camera at halfH=130, halfW≈200 (1000/650 aspect).
    // Scene bands run left→right across X: -200 to +200.
    // Y range visible: -130 to +130. Cars use worldX as their Y,
    // and worldX wraps at ±115, so cars are always on screen.
    const float BUILD_X1 = -200.0f, BUILD_X2 = -55.0f;   // ~145 wide
    const float RIVER_X1 =  -55.0f, RIVER_X2 =  -5.0f;   // ~50 wide
    const float ROAD_X1  =   -5.0f, ROAD_X2  = 175.0f;   // ~180 wide — 3 lanes + margins

    // ---- Building zone ----
    glColor3f(0.20f, 0.46f, 0.16f);
    rect(BUILD_X1, -200.0f, BUILD_X2, 200.0f);

    // ---- River band ----
    glColor3f(0.16f, 0.46f, 0.72f);
    rect(RIVER_X1, -200.0f, RIVER_X2, 200.0f);
    // ripple stripe
    glColor3f(0.24f, 0.58f, 0.84f);
    rect(RIVER_X1 + 10.0f, -200.0f, RIVER_X1 + 14.0f, 200.0f);
    // boat (top view): hull + cabin, travelling vertically (boatX → Y)
    glColor3f(0.55f, 0.28f, 0.0f);
    float bx = (RIVER_X1 + RIVER_X2) * 0.5f;
    rect(bx - 4.0f, boatX - 16.0f, bx + 4.0f, boatX + 16.0f);
    glColor3f(0.78f, 0.78f, 0.78f);
    rect(bx - 3.0f, boatX - 6.0f,  bx + 3.0f, boatX + 6.0f);
    // smoke puff above boat
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.5f, 0.5f, 0.5f, 0.35f);
    circle(2.5f, 2.5f, bx + 1.5f, boatX + 17.0f);
    glDisable(GL_BLEND);

    // ---- Road band ----
    glColor3f(0.22f, 0.22f, 0.22f);
    rect(ROAD_X1, -200.0f, ROAD_X2, 200.0f);
    // road edges (kerb)
    glColor3f(0.75f, 0.75f, 0.72f);
    rect(ROAD_X1, -200.0f, ROAD_X1 + 1.0f, 200.0f);
    rect(ROAD_X2 - 1.0f, -200.0f, ROAD_X2, 200.0f);

    // 3 lanes, each ~58 wide. Lane centres:
    //   lane1: ROAD_X1 + 29  = ~24
    //   lane2: ROAD_X1 + 90  = ~85
    //   lane3: ROAD_X1 + 150 = ~145
    float lane1X = ROAD_X1 + 29.0f;
    float lane2X = ROAD_X1 + 90.0f;
    float lane3X = ROAD_X1 + 150.0f;

    // lane dividers
    glColor3f(0.9f, 0.9f, 0.2f);
    glLineWidth(2.0f);
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(2, 0x00FF);
    glBegin(GL_LINES);
    glVertex2f(ROAD_X1 + 58.0f, -200.0f); glVertex2f(ROAD_X1 + 58.0f, 200.0f);
    glVertex2f(ROAD_X1 + 118.0f, -200.0f); glVertex2f(ROAD_X1 + 118.0f, 200.0f);
    glEnd();
    glDisable(GL_LINE_STIPPLE);

    // ---- Stop sign (top view) — sits at roadside, Y = BUS_STOP_X ----
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0,0,0,0.15f);
    circle(1.8f, 1.8f, ROAD_X1 + 2.0f, BUS_STOP_X + 0.8f);
    glDisable(GL_BLEND);
    glColor3f(0.85f, 0.05f, 0.05f);
    circle(1.5f, 1.5f, ROAD_X1 + 2.0f, BUS_STOP_X);

    // ---- Building footprints ----
    for (int i = 0; i < NUM_PLAN_BUILDINGS; i++) {
        PlanBuilding &p = planBuildings[i];
        float footY1 = p.x1 * 1.1f;
        float footY2 = footY1 + (p.x2 - p.x1) * 1.1f;
        int col = i % 3;
        float fx1 = BUILD_X1 + 10.0f + col * 44.0f;
        float fx2 = fx1 + 32.0f;

        planRoofShadow(fx1, footY1, fx2, footY2);
        glColor3f(p.r, p.g, p.b);
        rect(fx1, footY1, fx2, footY2);
        glColor3f(p.r * 1.2f > 1 ? 1 : p.r * 1.2f,
                   p.g * 1.2f > 1 ? 1 : p.g * 1.2f,
                   p.b * 1.2f > 1 ? 1 : p.b * 1.2f);
        rect((fx1 + fx2) * 0.5f, footY1, fx2, footY2);
        glColor3f(p.r * 0.55f, p.g * 0.55f, p.b * 0.55f);
        glLineWidth(1.5f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(fx1, footY1); glVertex2f(fx2, footY1);
        glVertex2f(fx2, footY2); glVertex2f(fx1, footY2);
        glEnd();
    }

    // ---- Trees ----
    float treeYArr[] = { -110.0f, -95.0f, -78.0f, -60.0f, -44.0f, -28.0f,
                          -12.0f,   4.0f,  20.0f,  38.0f,  55.0f,  72.0f,
                           88.0f, 105.0f };
    int numTrees = sizeof(treeYArr) / sizeof(treeYArr[0]);
    for (int i = 0; i < numTrees; i++) {
        float ty = treeYArr[i];
        float tx = BUILD_X1 + 10.0f + (i % 3) * 44.0f + 35.0f;
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0,0,0,0.15f);
        circle(3.8f, 3.8f, tx + 0.7f, ty - 0.7f);
        glDisable(GL_BLEND);
        glColor3f(0.10f, 0.42f, 0.10f);
        circle(3.8f, 3.8f, tx, ty);
        glColor3f(0.22f, 0.64f, 0.20f);
        circle(2.0f, 2.0f, tx - 0.9f, ty + 0.9f);
    }

    // ---- CARS in top-down view ----
    // Road runs vertically (Y axis). Each car's worldX → Y on screen.
    // Lane centres are fixed X positions within the road band.
    // car1 (red)    — lane1, moves down (+Y)
    drawTopDownCar(car1X, lane1X, 0.55f, 0.05f, 0.05f, true,  false);
    // car2 (yellow) — lane2, moves down (+Y), has bus-stop door
    drawTopDownCar(car2X, lane2X, 0.85f, 0.75f, 0.0f,  true,  true);
    // car3 (blue)   — lane3, moves up (-Y, coming from +worldX)
    drawTopDownCar(car3X, lane3X, 0.25f, 0.35f, 0.65f, false, false);

    // Selection highlight ellipse around the player-controlled car
    if (controlMode == 1) {
        float selX, selY;
        if      (selectedCar == 1) { selX = lane1X; selY = car1X + 11.0f; }
        else if (selectedCar == 3) { selX = lane3X; selY = car3X + 11.0f; }
        else                       { selX = lane2X; selY = car2X + 11.0f; }

        glColor3f(1.0f, 1.0f, 0.0f);
        glLineWidth(2.5f);
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 32; i++) {
            float a = 2.0f * 3.14159f * i / 32;
            glVertex2f(selX + 7.0f * cosf(a), selY + 14.0f * sinf(a));
        }
        glEnd();
    }
}

// =============================================
//  DISPLAY
// =============================================
void display()
{
    glClear(GL_COLOR_BUFFER_BIT);
    applyCameraView();

    if (topDownBlend < 0.5f) {

    glColor3f(0.40f, 0.72f, 0.95f);
    rect(-200.0f, -10.0f, 200.0f, 200.0f);
    sun();

    building(-100.0f, 5.0f, -90.0f, 30.0f, 0.68f, 0.58f, 0.50f, 2, 4);
    building(-89.0f, 5.0f, -78.0f, 26.0f, 0.72f, 0.62f, 0.52f, 2, 3);
    building(-77.0f, 5.0f, -65.0f, 33.0f, 0.65f, 0.55f, 0.48f, 2, 4);
    building(-50.0f, 5.0f, -40.0f, 28.0f, 0.72f, 0.60f, 0.50f, 2, 3);
    building(-39.0f, 5.0f, -28.0f, 32.0f, 0.65f, 0.55f, 0.48f, 2, 4);
    building(-15.0f, 5.0f,  -4.0f, 30.0f, 0.70f, 0.62f, 0.54f, 2, 3);
    building( -3.0f, 5.0f,   8.0f, 34.0f, 0.68f, 0.56f, 0.46f, 2, 4);
    building(  9.0f, 5.0f,  20.0f, 28.0f, 0.72f, 0.60f, 0.50f, 2, 3);
    building( 21.0f, 5.0f,  33.0f, 32.0f, 0.65f, 0.55f, 0.48f, 2, 4);
    building( 34.0f, 5.0f,  50.0f, 30.0f, 0.70f, 0.58f, 0.46f, 2, 3);
    building( 64.0f, 5.0f,  76.0f, 33.0f, 0.65f, 0.55f, 0.48f, 2, 4);
    building( 77.0f, 5.0f,  88.0f, 26.0f, 0.72f, 0.62f, 0.52f, 2, 3);
    building( 89.0f, 5.0f, 100.0f, 30.0f, 0.68f, 0.58f, 0.50f, 2, 4);

    roundTree(-95.0f, 5.0f); roundTree(-60.0f, 5.0f);
    roundTree(-44.0f, 5.0f); roundTree(-9.5f, 5.0f); roundTree(25.5f, 5.0f);
    roundTree(59.0f, 5.0f);  roundTree(94.0f, 5.0f);
    pineTree(-83.0f, 5.0f);  pineTree(-22.0f, 5.0f);
    pineTree(4.5f, 5.0f);    pineTree(41.0f, 5.0f);
    pineTree(70.0f, 5.0f);   pineTree(105.0f, 5.0f);

    glColor3f(0.20f, 0.55f, 0.15f);
    rect(-115.0f, 3.5f, 115.0f, 6.5f);
    zigzag(-115.0f, 115.0f, 5.5f, 2.0f, 3.0f);

    river(-10.0f, 3.5f);
    boat(boatX, boatY, boatRockAngle, boatBobOffset);
    drawSmoke();

    glColor3f(0.15f, 0.52f, 0.10f);
    rect(-115.0f, -14.0f, 115.0f, -9.5f);
    zigzag(-115.0f, 115.0f, -9.5f, 2.5f, 3.5f);

    drawRoad();

    glColor3f(0.9f, 0.9f, 0.9f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glVertex2f(-115.0f,-14.5f); glVertex2f(115.0f,-14.5f);
    glVertex2f(-115.0f,-27.5f); glVertex2f(115.0f,-27.5f);
    glEnd();

    glColor3f(0.15f, 0.52f, 0.10f);
    zigzag(-115.0f, 115.0f, -13.8f, -2.0f, 3.5f);

    stopSign(BUS_STOP_X);

    car(car1X, -27.0f, 0.55f, 0.05f, 0.05f);
    if (selectedCar == 1 && controlMode == 1)
        drawSelector(car1X + 11.0f, -13.5f);

    car(car2X, -26.0f, 0.85f, 0.75f, 0.0f, doorAngle);

    glPushMatrix();
    glTranslatef(car3X, 0.0f, 0.0f);
    glScalef(-1.0f, 1.0f, 1.0f);
    car(-11.0f, -27.0f, 0.25f, 0.35f, 0.65f);
    glPopMatrix();
    if (selectedCar == 3 && controlMode == 1)
        drawSelector(car3X, -13.5f);

    glColor3f(0.12f, 0.45f, 0.08f);
    rect(-200.0f, -200.0f, 200.0f, -28.0f);

    } else {
        drawTopDownScene();
    }

    // HUD title
    const char *camName;
    switch (cameraMode) {
        case CAM_NORMAL:  camName = "1:Normal";   break;
        case CAM_BOAT:    camName = "2:Boat";     break;
        case CAM_CAR:     camName = "3:Car";      break;
        case CAM_TOPDOWN: camName = "4:Top-Down"; break;
        default:          camName = "?";
    }

    const char *doorStatus = (stopStage == 1) ? "  [O]=Open/Close Door" : "";

    char title[300];
    sprintf(title,
        "City Scene | [TAB]=Cycle car  [Arrow]=Drive  "
        "[Q/E]=Rotate  [Z/X]=Zoom  [R]=Reset  "
        "Camera [1]Normal [2]Boat [3]Car [4]Top  "
        "Active:%s%s",
        camName, doorStatus);
    glutSetWindowTitle(title);

    glutSwapBuffers();
    boatMoving = true;
}

// =============================================
//  KEYBOARD
// =============================================
void handleKeypress(unsigned char key, int x, int y)
{
    if (key == 27) exit(0);

    if (key == '\t') {
        controlMode = 1;
        selectedCar = (selectedCar == 1) ? 3 : 1;
        glutPostRedisplay();
        return;
    }

    // Camera modes (4 modes now, Angled removed)
    if (key == '1') { cameraMode = CAM_NORMAL;  extraRotZTarget = 0.0f; glutPostRedisplay(); return; }
    if (key == '2') { cameraMode = CAM_BOAT;    extraRotZTarget = 0.0f; glutPostRedisplay(); return; }
    if (key == '3') { cameraMode = CAM_CAR;     extraRotZTarget = 0.0f; glutPostRedisplay(); return; }
    if (key == '4') { cameraMode = CAM_TOPDOWN; extraRotZTarget = 0.0f; glutPostRedisplay(); return; }

    // Smooth rotation: update TARGET, the lerp in stepCamera() does the rest
    if (key == 'e' || key == 'E') { extraRotZTarget -= MANUAL_ROT_STEP; glutPostRedisplay(); return; }
    if (key == 'q' || key == 'Q') { extraRotZTarget += MANUAL_ROT_STEP; glutPostRedisplay(); return; }

    // Smooth zoom: update TARGET, lerp handles the rest
    if (key == 'z' || key == 'Z') {
        zoomFactorTarget -= ZOOM_STEP;
        if (zoomFactorTarget < ZOOM_MIN) zoomFactorTarget = ZOOM_MIN;
        glutPostRedisplay(); return;
    }
    if (key == 'x' || key == 'X') {
        zoomFactorTarget += ZOOM_STEP;
        if (zoomFactorTarget > ZOOM_MAX) zoomFactorTarget = ZOOM_MAX;
        glutPostRedisplay(); return;
    }

    // R resets both rotation AND zoom
    if (key == 'r' || key == 'R') {
        extraRotZTarget = 0.0f;
        zoomFactorTarget = 1.0f;
        glutPostRedisplay(); return;
    }

    if ((key == 'o' || key == 'O') && stopStage == 1) {
        if (!doorOpening && !doorClosing) {
            if (!doorFullyOpen) {
                doorOpening  = true;
                doorClosing  = false;
            } else {
                doorClosing  = true;
                doorOpening  = false;
            }
        }
        glutPostRedisplay();
        return;
    }
}

// =============================================
//  AUTOMATIC BOAT LOGIC
// =============================================
void updateAutoBoat()
{
    boatX += boatSpeed;
    if (boatX > WORLD_HALF)
        boatX = -WORLD_HALF;
    boatMoving = true;
}

// =============================================
//  AUTOMATIC BUS-STOP CAR LOGIC (car 2)
// =============================================
void updateAutoCar()
{
    if (doorOpening) {
        doorAngle += 0.04f;
        if (doorAngle >= 1.0f) {
            doorAngle     = 1.0f;
            doorOpening   = false;
            doorFullyOpen = true;
        }
    }
    if (doorClosing) {
        doorAngle -= 0.04f;
        if (doorAngle <= 0.0f) {
            doorAngle     = 0.0f;
            doorClosing   = false;
            doorFullyOpen = false;
        }
    }

    switch (stopStage) {
        case 0: {
            float target = BUS_STOP_X - 8.0f;
            if (car2X < target) {
                car2X += autoCarSpeed;
                if (car2X >= target) {
                    car2X         = target;
                    stopStage     = 1;
                    waitTimer     = WAIT_TICKS;
                    doorAngle     = 0.0f;
                    doorFullyOpen = false;
                    doorOpening   = false;
                    doorClosing   = false;
                }
            } else {
                stopStage = 1;
                waitTimer = WAIT_TICKS;
            }
            break;
        }
        case 1: {
            waitTimer--;
            if (waitTimer <= 0) {
                if (doorAngle > 0.0f) {
                    doorClosing = true;
                    doorOpening = false;
                }
                if (doorAngle <= 0.0f && !doorClosing)
                    stopStage = 2;
            }
            break;
        }
        case 2: {
            car2X += autoCarSpeed;
            if (car2X > WORLD_HALF) {
                car2X         = -WORLD_HALF;
                stopStage     = 0;
                doorAngle     = 0.0f;
                doorFullyOpen = false;
                doorOpening   = false;
                doorClosing   = false;
            }
            break;
        }
    }
}

// =============================================
//  TIMER
// =============================================
void timer(int value)
{
    updateAutoBoat();
    updateAutoCar();
    stepCamera();

    boatBobPhase += 0.06f;
    if (boatBobPhase > 2.0f * 3.14159f) boatBobPhase -= 2.0f * 3.14159f;
    boatBobOffset = sinf(boatBobPhase) * 0.35f;
    boatRockAngle = sinf(boatBobPhase * 0.7f) * 1.5f;

    smokeSpawnTimer++;
    if (smokeSpawnTimer >= 2) {
        smokeSpawnTimer = 0;
        spawnSmoke(boatX + 4.5f, boatY + 14.5f + boatBobOffset);
    }
    updateSmoke();

    glutPostRedisplay();
    glutTimerFunc(30, timer, 0);
}

// =============================================
//  SPECIAL KEYS
// =============================================
void handleSpecialKey(int key, int x, int y)
{
    float move = 0.0f;
    if (key == GLUT_KEY_LEFT)  move = -carSpeed;
    if (key == GLUT_KEY_RIGHT) move =  carSpeed;

    if (selectedCar == 1) {
        car1X += move;
        if (car1X < -WORLD_HALF)      car1X = -WORLD_HALF;
        if (car1X >  WORLD_HALF - 22.0f) car1X = WORLD_HALF - 22.0f;
    }
    else if (selectedCar == 3) {
        car3X += move;
        if (car3X < -WORLD_HALF + 22.0f) car3X = -WORLD_HALF + 22.0f;
        if (car3X >  WORLD_HALF - 22.0f) car3X =  WORLD_HALF - 22.0f;
    }

    glutPostRedisplay();
}

// =============================================
//  RESHAPE
// =============================================
void reshape(int w, int h)
{
    if (h == 0) h = 1;
    winW = w;
    winH = h;
    glViewport(0, 0, w, h);
    glutPostRedisplay();
}

// =============================================
//  INIT & MAIN
// =============================================
void init()
{
    glClearColor(0.40f, 0.72f, 0.95f, 1.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-50, 50, -50, 50, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    initSmoke();
    srand(42);
    startBoatSound();
    camCenterPrevX = (camCur.l + camCur.r) * 0.5f;
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    glutInitWindowSize(1000, 650);
    glutInitWindowPosition(50, 50);
    glutCreateWindow("City Scene");
    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(handleKeypress);
    glutSpecialFunc(handleSpecialKey);
    glutTimerFunc(30, timer, 0);
    glutMainLoop();
    return 0;
}
