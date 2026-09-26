#include <GL/glut.h>
#include <cmath>

#ifdef _WIN32
#include <windows.h>   // Required for Beep() and PlaySound-style audio APIs
#endif

float rocketX = -0.05f;
float rocketY = 0.05f;
bool isNight = false;
bool flag = true, flag2 = true;
bool value = false;

// --- CAR ANIMATION ---
float carX = -0.3f;

// --- 3D CAMERA STATE -------------------------------------------------------
// cameraAngle interpolates the camera's look-direction:
//   0   degrees -> looking straight down -X (your original "front on" 2D-style view)
//   90  degrees -> looking straight down -Z (a side view, revealing real depth)
// camDistance/camHeight control how far back and how high the camera sits.
float cameraAngle       = 0.0f;   // current angle (degrees), eased each frame
float targetCameraAngle = 0.0f;   // where we want the angle to end up
float cameraY           = 0.0f;   // existing vertical follow-cam value (kept, now drives camHeight)
float targetCameraY     = 0.0f;

const float camDistance = 1.6f;   // how far the eye sits from the look-at point
// -----------------------------------------------------------------------------

// --- SOUND STATE -------------------------------------------------------------
// Windows-only beep-based "engine sound" placeholder. Beep() is blocking on
// some systems, so we only re-trigger it occasionally from the timer rather
// than every frame, which would otherwise stutter the animation.
bool  rocketMoving   = false;   // true while the rocket is actively ascending
int   soundTickCount = 0;       // counts timer ticks to throttle Beep() calls
const int SOUND_INTERVAL_TICKS = 12; // ~12 * 16ms = ~190ms between beeps
// -----------------------------------------------------------------------------

// --- DEPTH LAYOUT (NEW) -----------------------------------------------------
// Each major object gets pushed to its own Z-band, derived from where it
// currently sits on screen (left-to-right -> back-to-front). This is what
// makes the side view (press C) show real layered depth instead of every
// object collapsing onto the same paper-thin Z=0 plane. Bands are spaced
// far enough apart that depth-fighting between *different* objects can't
// happen; the original flicker was objects at identical Z overlapping.
const float Z_SKY     = -2.2f;  // sun/moon/clouds/stars - furthest back
const float Z_WALL    = -1.2f;  // house wall cluster - back-middle
const float Z_STAND   = -0.3f;  // rocket stand - just behind the rocket
const float Z_ROCKET  =  0.0f;  // rocket + flame - the focal object, Z reference
const float Z_GROUND_NEAR = -2.5f; // ground/road plane far edge (back)
const float Z_GROUND_FAR  =  1.2f; // ground/road plane near edge (front)
// Car Z is computed live from carX in drawCar() so it visibly travels
// through depth as it crosses the screen left-to-right.
// -----------------------------------------------------------------------------

// --- 3D EXTRUSION HELPER -----------------------------------------------
// Draws a flat quad (x0,y0)-(x1,y1)-(x2,y2)-(x3,y3) as a real 3D box: a
// front face, a back face (depth units further from camera), and 4 side
// faces connecting them. This is what gives buildings/the rocket stand
// actual depth/thickness instead of just being a flat plane pushed back
// in Z - the same flat color is used for all faces here, kept simple so
// it reads clearly as "this object has thickness" without needing
// separate per-face shading.
void extrudeQuad(float x0,float y0,float x1,float y1,float x2,float y2,float x3,float y3,
                  float depth, float r, float g, float b)
{
    glColor3f(r, g, b);

    // front face (at local Z = 0)
    glBegin(GL_QUADS);
    glVertex3f(x0,y0,0.0f); glVertex3f(x1,y1,0.0f);
    glVertex3f(x2,y2,0.0f); glVertex3f(x3,y3,0.0f);
    glEnd();

    // back face (at local Z = -depth, i.e. further from camera)
    glBegin(GL_QUADS);
    glVertex3f(x0,y0,-depth); glVertex3f(x1,y1,-depth);
    glVertex3f(x2,y2,-depth); glVertex3f(x3,y3,-depth);
    glEnd();

    // slightly darker shade for the side faces so the extrusion reads
    // visually as a 3D edge rather than disappearing into the front color
    glColor3f(r*0.7f, g*0.7f, b*0.7f);
    float xs[4] = {x0,x1,x2,x3};
    float ys[4] = {y0,y1,y2,y3};
    glBegin(GL_QUADS);
    for(int i = 0; i < 4; i++)
    {
        int j = (i+1) % 4;
        glVertex3f(xs[i], ys[i], 0.0f);
        glVertex3f(xs[j], ys[j], 0.0f);
        glVertex3f(xs[j], ys[j], -depth);
        glVertex3f(xs[i], ys[i], -depth);
    }
    glEnd();
}
// -----------------------------------------------------------------------

// Extrudes an arbitrary N-point polygon (front face fan, back face fan,
// and a ribbon of side quads around the perimeter) - used for the curved
// arch-roof shapes in wall() that aren't simple 4-vertex quads.
void extrudeFan(const float* xs, const float* ys, int n, float depth,
                 float r, float g, float b)
{
    glColor3f(r, g, b);

    glBegin(GL_POLYGON);
    for(int i = 0; i < n; i++) glVertex3f(xs[i], ys[i], 0.0f);
    glEnd();

    glBegin(GL_POLYGON);
    for(int i = n-1; i >= 0; i--) glVertex3f(xs[i], ys[i], -depth);
    glEnd();

    glColor3f(r*0.7f, g*0.7f, b*0.7f);
    glBegin(GL_QUADS);
    for(int i = 0; i < n; i++)
    {
        int j = (i+1) % n;
        glVertex3f(xs[i], ys[i], 0.0f);
        glVertex3f(xs[j], ys[j], 0.0f);
        glVertex3f(xs[j], ys[j], -depth);
        glVertex3f(xs[i], ys[i], -depth);
    }
    glEnd();
}

void wall()
{
    const float WALL_DEPTH = 0.10f; // thickness of the building blocks in Z
    extrudeQuad(0.02f,0.40f, 0.06f,0.40f, 0.06f,0.65f, 0.02f,0.65f,
                WALL_DEPTH, 0.75f,0.45f,0.18f);
    extrudeQuad(0.06f,0.40f, 0.08f,0.42f, 0.08f,0.67f, 0.06f,0.65f,
                WALL_DEPTH, 0.55f,0.32f,0.10f);
    extrudeQuad(0.08f,0.40f, 0.22f,0.40f, 0.22f,0.74f, 0.08f,0.74f,
                WALL_DEPTH, 0.80f,0.48f,0.20f);
    extrudeQuad(0.22f,0.40f, 0.25f,0.43f, 0.25f,0.77f, 0.22f,0.74f,
                WALL_DEPTH, 0.60f,0.35f,0.12f);
    extrudeQuad(0.07f,0.72f, 0.22f,0.72f, 0.22f,0.74f, 0.07f,0.74f,
                WALL_DEPTH, 0.15f,0.15f,0.15f);
    extrudeQuad(0.22f,0.72f, 0.25f,0.75f, 0.25f,0.77f, 0.22f,0.74f,
                WALL_DEPTH, 0.05f,0.05f,0.05f);
    extrudeQuad(0.15f,0.40f, 0.28f,0.40f, 0.28f,0.68f, 0.15f,0.68f,
                WALL_DEPTH, 0.85f,0.53f,0.22f);
    extrudeQuad(0.28f,0.40f, 0.31f,0.43f, 0.31f,0.71f, 0.28f,0.68f,
                WALL_DEPTH, 0.65f,0.38f,0.14f);
    {
        float ax[22], ay[22];
        ax[0]=0.17f; ay[0]=0.66f;
        ax[1]=0.27f; ay[1]=0.66f;
        for(int i = 0; i <= 20; i++) {
            float t = (float)i / 20.0f;
            ax[i+2] = 0.17f + t * (0.27f - 0.17f);
            ay[i+2] = 0.40f + 0.26f * (2.0f*t - 1.0f) * (2.0f*t - 1.0f);
        }
        extrudeFan(ax, ay, 22, WALL_DEPTH, 0.62f,0.52f,0.12f);
    }
    extrudeQuad(0.42f,0.36f, 0.60f,0.36f, 0.59f,0.40f, 0.43f,0.40f,
                WALL_DEPTH, 0.45f,0.28f,0.12f);
    extrudeQuad(0.81f,0.40f, 0.90f,0.40f, 0.90f,0.79f, 0.81f,0.74f,
                WALL_DEPTH, 0.75f,0.45f,0.18f);
    extrudeQuad(0.90f,0.40f, 0.93f,0.42f, 0.93f,0.81f, 0.90f,0.79f,
                WALL_DEPTH, 0.55f,0.32f,0.10f);
    extrudeQuad(0.72f,0.40f, 0.84f,0.40f, 0.84f,0.72f, 0.72f,0.72f,
                WALL_DEPTH, 0.80f,0.48f,0.20f);
    extrudeQuad(0.84f,0.40f, 0.87f,0.43f, 0.87f,0.75f, 0.84f,0.72f,
                WALL_DEPTH, 0.60f,0.35f,0.12f);
    extrudeQuad(0.72f,0.70f, 0.84f,0.70f, 0.84f,0.72f, 0.72f,0.72f,
                WALL_DEPTH, 0.15f,0.15f,0.15f);
    extrudeQuad(0.84f,0.70f, 0.87f,0.73f, 0.87f,0.75f, 0.84f,0.72f,
                WALL_DEPTH, 0.05f,0.05f,0.05f);
    extrudeQuad(0.64f,0.40f, 0.77f,0.40f, 0.77f,0.68f, 0.64f,0.68f,
                WALL_DEPTH, 0.85f,0.53f,0.22f);
    extrudeQuad(0.77f,0.40f, 0.80f,0.43f, 0.80f,0.71f, 0.77f,0.68f,
                WALL_DEPTH, 0.65f,0.38f,0.14f);
    {
        float ax[22], ay[22];
        ax[0]=0.66f; ay[0]=0.66f;
        ax[1]=0.75f; ay[1]=0.66f;
        for(int i = 0; i <= 20; i++) {
            float t = (float)i / 20.0f;
            ax[i+2] = 0.66f + t * (0.75f - 0.66f);
            ay[i+2] = 0.40f + 0.26f * (2.0f*t - 1.0f) * (2.0f*t - 1.0f);
        }
        extrudeFan(ax, ay, 22, WALL_DEPTH, 0.62f,0.52f,0.12f);
    }
}

void flame()
{
    if (value)
    {
        if (flag == true){ glColor3f(1.0, 0.25, 0.0); flag = false; }
        else             { glColor3f(1.0, 0.816, 0.0); flag = true; }
    }
    else { glColor3f(0.3, 1.2, 0.0); }

    float x = rocketX, y = rocketY;
    glBegin(GL_POLYGON);
    glVertex3f(x+0.56, y+.35, 0.0);
    glVertex3f(x+0.54, y+.31, 0.0);
    glVertex3f(x+0.58, y+0.28, 0.0);
    glVertex3f(x+0.61, y+0.31, 0.0);
    glVertex3f(x+0.59, y+0.35, 0.0);

    if (value)
    {
        if (flag2 == true){ glColor3f(1.0, 0.816, 0.0); flag2 = false; }
        else              { glColor3f(1.0, 0.25, 0.0);  flag2 = true; }
    }
    else { glColor3f(0.3, 1.2, 0.0); }

    glVertex3f(x+0.56, y+.35, 0.0);
    glVertex3f(x+0.54, y+.31, 0.0);
    glVertex3f(x+0.58, y+0.28, 0.0);
    glVertex3f(x+0.61, y+0.31, 0.0);
    glVertex3f(x+0.59, y+0.35, 0.0);
    glEnd();
}

void rocket()
{
    float x = rocketX, y = rocketY;
    glBegin(GL_QUADS);

    glColor3f(0.96f, 0.94f, 0.88f);
    glVertex3f(x+0.55, y+.35,  0.0);
    glVertex3f(x+0.6,  y+.35,  0.0);
    glVertex3f(x+0.6,  y+0.6,  0.0);
    glVertex3f(x+0.55, y+0.6,  0.0);

    glColor3f(0.95, 0.95, 0.90);
    glVertex3f(x+0.6,   y+0.6,  0.0);
    glVertex3f(x+0.55,  y+0.6,  0.0);
    glVertex3f(x+0.574, y+0.65, 0.0);
    glVertex3f(x+0.6,   y+0.6,  0.0);

    glColor3f(1.0, 1.0, 0.0);
    glVertex3f(x+0.56, y+0.34, 0.0);
    glVertex3f(x+0.57, y+0.34, 0.0);
    glVertex3f(x+0.57, y+0.35, 0.0);
    glVertex3f(x+0.56, y+0.35, 0.0);

    glVertex3f(x+0.58, y+0.34, 0.0);
    glVertex3f(x+0.59, y+0.34, 0.0);
    glVertex3f(x+0.59, y+0.35, 0.0);
    glVertex3f(x+0.58, y+0.35, 0.0);

    glColor3f(1.0, 0.0, 0.0);
    glVertex3f(x+0.55, y+0.37,  0.0);
    glVertex3f(x+0.55, y+0.425, 0.0);
    glVertex3f(x+0.51, y+0.365, 0.0);
    glVertex3f(x+0.55, y+0.37,  0.0);

    glVertex3f(x+0.6,  y+0.37,  0.0);
    glVertex3f(x+0.6,  y+0.425, 0.0);
    glVertex3f(x+0.64, y+0.365, 0.0);
    glVertex3f(x+0.6,  y+0.37,  0.0);

    glEnd();
    flame();
}

void sun()
{
    glColor3f(0.90, 0.30, 0.0);
    float x = 0.8f, y = 0.8f, r = 0.07f;
    glBegin(GL_POLYGON);
    for(int i = 0; i < 100; i++)
    {
        float angle = 2.0f * 3.1416f * i / 100;
        glVertex2f(x + cos(angle)*r, y + sin(angle)*r);
    }
    glEnd();

    glColor3f(1.0, 0.60, 0.0);
    glBegin(GL_LINES);
    for(int i = 0; i < 12; i++)
    {
        float angle = 2.0f * 3.1416f * i / 12;
        glVertex2f(x + cos(angle)*0.09f, y + sin(angle)*0.09f);
        glVertex2f(x + cos(angle)*0.14f, y + sin(angle)*0.14f);
    }
    glEnd();
}

void drawCloudPuff(float cx, float cy, float r)
{
    glBegin(GL_POLYGON);
    for(int i = 0; i < 40; i++)
    {
        float angle = 2.0f * 3.1416f * i / 40;
        glVertex2f(cx + cos(angle) * r, cy + sin(angle) * r);
    }
    glEnd();
}

void clouds()
{
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glVertex2f(0.10f, 0.80f);
    glVertex2f(0.32f, 0.80f);
    glVertex2f(0.32f, 0.84f);
    glVertex2f(0.10f, 0.84f);
    glEnd();

    drawCloudPuff(0.13f, 0.83f, 0.045f);
    drawCloudPuff(0.19f, 0.85f, 0.06f);
    drawCloudPuff(0.25f, 0.86f, 0.055f);
    drawCloudPuff(0.30f, 0.83f, 0.04f);

    glBegin(GL_QUADS);
    glVertex2f(0.48f, 0.84f);
    glVertex2f(0.68f, 0.84f);
    glVertex2f(0.68f, 0.87f);
    glVertex2f(0.48f, 0.87f);
    glEnd();

    drawCloudPuff(0.51f, 0.86f, 0.035f);
    drawCloudPuff(0.56f, 0.88f, 0.05f);
    drawCloudPuff(0.62f, 0.89f, 0.045f);
    drawCloudPuff(0.66f, 0.86f, 0.035f);
}

void moon()
{
    float x = 0.8f, y = 0.8f, r = 0.06f;

    glColor3f(0.95f, 0.95f, 0.70f);
    glBegin(GL_POLYGON);
    for(int i = 0; i < 100; i++)
    {
        float angle = 2.0f * 3.1416f * i / 100;
        glVertex2f(x + cos(angle)*r, y + sin(angle)*r);
    }
    glEnd();

    if(isNight)
        glColor3f(0.05f, 0.05f, 0.15f);
    else
        glColor3f(0.4f, 0.7f, 1.0f);

    glBegin(GL_POLYGON);
    for(int i = 0; i < 100; i++)
    {
        float angle = 2.0f * 3.1416f * i / 100;
        glVertex2f((x - 0.025f) + cos(angle)*r, y + sin(angle)*r);
    }
    glEnd();
}

void stars()
{
    glColor3f(1.0f, 1.0f, 0.9f);
    glPointSize(2.5f);
    glBegin(GL_POINTS);
    float pts[][2] = {
        {0.05f,0.95f},{0.12f,0.85f},{0.22f,0.92f},{0.35f,0.88f},
        {0.48f,0.96f},{0.55f,0.82f},{0.63f,0.91f},{0.72f,0.78f},
        {0.18f,0.75f},{0.30f,0.70f},{0.42f,0.80f},{0.58f,0.74f},
        {0.68f,0.86f},{0.10f,0.65f},{0.25f,0.60f},{0.38f,0.68f},
        {0.50f,0.63f},{0.20f,0.99f},{0.44f,0.72f},{0.76f,0.93f},
        {0.88f,0.68f},{0.93f,0.85f},{0.08f,0.78f},{0.60f,0.98f}
    };
    for(auto& p : pts)
        glVertex2f(p[0], p[1]);
    glEnd();
}

void rocket_stand()
{
    const float STAND_DEPTH = 0.06f; // how thick the stand looks in Z

    extrudeQuad(0.40f,0.60f, 0.45f,0.60f, 0.45f,0.35f, 0.40f,0.35f,
                STAND_DEPTH, 0.95f, 0.93f, 0.88f);

    extrudeQuad(0.60f,0.60f, 0.65f,0.60f, 0.65f,0.35f, 0.60f,0.35f,
                STAND_DEPTH, 0.95f, 0.93f, 0.88f);

    extrudeQuad(0.40f,0.55f, 0.65f,0.55f, 0.65f,0.60f, 0.40f,0.60f,
                STAND_DEPTH, 0.95f, 0.93f, 0.88f);

    // Outline kept on the front face only, same as the original look.
    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(2.0f);

    glBegin(GL_LINE_LOOP);
    glVertex2f(0.40f, 0.60f); glVertex2f(0.45f, 0.60f);
    glVertex2f(0.45f, 0.35f); glVertex2f(0.40f, 0.35f);
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex2f(0.60f, 0.60f); glVertex2f(0.65f, 0.60f);
    glVertex2f(0.65f, 0.35f); glVertex2f(0.60f, 0.35f);
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex2f(0.40f, 0.55f); glVertex2f(0.65f, 0.55f);
    glVertex2f(0.65f, 0.60f); glVertex2f(0.40f, 0.60f);
    glEnd();
}

void road()
{
    // Reverted to a flat strip (single Z), same as the original 2D layout.
    glBegin(GL_QUADS);
    glColor3f(0.25f, 0.25f, 0.25f);
    glVertex3f(0.0f, 0.0f,  Z_GROUND_FAR);
    glVertex3f(1.0f, 0.0f,  Z_GROUND_FAR);
    glVertex3f(1.0f, 0.08f, Z_GROUND_FAR);
    glVertex3f(0.0f, 0.08f, Z_GROUND_FAR);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    float dashX = 0.02f;
    while(dashX < 1.0f)
    {
        glVertex3f(dashX,        0.04f, Z_GROUND_FAR);
        glVertex3f(dashX + 0.06f, 0.04f, Z_GROUND_FAR);
        dashX += 0.12f;
    }
    glEnd();

    glColor3f(1.0f, 0.85f, 0.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.075f, Z_GROUND_FAR); glVertex3f(1.0f, 0.075f, Z_GROUND_FAR);
    glVertex3f(0.0f, 0.005f, Z_GROUND_FAR); glVertex3f(1.0f, 0.005f, Z_GROUND_FAR);
    glEnd();
}

void drawCar()
{
    glPushMatrix();
    // Car sits a tiny bit closer to the camera than the road/ground plane
    // (Z_GROUND_FAR + small epsilon). Placing it at the exact same Z as
    // the road caused depth-buffer Z-fighting - both surfaces competing
    // for the same depth value - which made the car randomly disappear.
    // The offset is small enough that it still visually sits "on" the road.
    glTranslatef(carX, 0.015f, Z_GROUND_FAR + 0.01f);

    if(isNight) glColor3f(0.6f, 0.1f, 0.1f);
    else glColor3f(0.9f, 0.2f, 0.2f);
    glBegin(GL_QUADS);
    glVertex2f(0.0f, 0.01f);
    glVertex2f(0.12f, 0.01f);
    glVertex2f(0.12f, 0.035f);
    glVertex2f(0.0f, 0.035f);
    glEnd();

    if(isNight) glColor3f(0.4f, 0.1f, 0.1f);
    else glColor3f(0.7f, 0.1f, 0.1f);
    glBegin(GL_QUADS);
    glVertex2f(0.02f, 0.035f);
    glVertex2f(0.10f, 0.035f);
    glVertex2f(0.08f, 0.055f);
    glVertex2f(0.04f, 0.055f);
    glEnd();

    glColor3f(0.7f, 0.9f, 1.0f);
    glBegin(GL_QUADS);
    glVertex2f(0.045f, 0.038f);
    glVertex2f(0.058f, 0.038f);
    glVertex2f(0.058f, 0.05f);
    glVertex2f(0.048f, 0.05f);

    glVertex2f(0.062f, 0.038f);
    glVertex2f(0.075f, 0.038f);
    glVertex2f(0.072f, 0.05f);
    glVertex2f(0.062f, 0.05f);
    glEnd();

    glColor3f(0.1f, 0.1f, 0.1f);
    float radius = 0.012f;
    float wheelPositions[2] = { 0.025f, 0.095f };
    for (int w = 0; w < 2; w++) {
        glBegin(GL_POLYGON);
        for (int i = 0; i < 20; i++) {
            float angle = 2.0f * 3.1416f * i / 20;
            glVertex2f(wheelPositions[w] + cos(angle) * radius, 0.01f + sin(angle) * radius);
        }
        glEnd();
    }

    if(isNight) glColor3f(1.0f, 1.0f, 0.5f);
    else glColor3f(0.5f, 0.5f, 0.5f);
    glBegin(GL_QUADS);
    glVertex2f(0.12f, 0.025f);
    glVertex2f(0.125f, 0.025f);
    glVertex2f(0.125f, 0.032f);
    glVertex2f(0.12f, 0.032f);
    glEnd();

    glPopMatrix();
}

// --- SOUND HELPERS -------------------------------------------------------
// Beep() is the simplest possible "engine sound" with zero external files
// or libraries. It's a Windows-only blocking call (runs on its own thread
// internally so it won't freeze rendering for short durations like this).
// Replace this function later with PlaySound(TEXT("launch.wav"), NULL,
// SND_ASYNC | SND_LOOP) if/when you have a real .wav asset - that's a
// straight drop-in swap, the call sites below don't need to change.
void playEngineTick()
{
#ifdef _WIN32
    Beep(220 + (rand() % 80), 80); // low rumble-ish tone, short duration
#endif
}

void stopEngineSound()
{
#ifdef _WIN32
    // Nothing to explicitly stop with Beep() since each call is short-lived;
    // this function exists so the PlaySound swap-in later has a symmetrical
    // start/stop pair to call (PlaySound(NULL, NULL, 0) would go here).
#endif
}
// ---------------------------------------------------------------------------

void keyboard(unsigned char key, int x, int y)
{
    if(key == 'n' || key == 'N')
    {
        isNight = !isNight;
        if(isNight)
            glClearColor(0.05f, 0.05f, 0.15f, 1.0f);
        else
            glClearColor(0.4f,  0.7f,  1.0f,  1.0f);

        glutPostRedisplay();
    }

    // --- NEW: 'C' toggles the camera between front view (looking down -X)
    // and side view (looking down -Z), smoothly eased in the timer loop.
    if(key == 'c' || key == 'C')
    {
        targetCameraAngle = (targetCameraAngle < 45.0f) ? 90.0f : 0.0f;
        glutPostRedisplay();
    }
}

void specialKey(int key, int x, int y)
{
    switch(key)
    {
    case GLUT_KEY_UP:
        if(!value) {
            // Rocket just started moving this frame - sound state turns on.
            rocketMoving = true;
        }
        value = true;
        rocketY += 0.05f;
        break;
    case GLUT_KEY_DOWN:
        rocketY -= 0.05f;
        break;
    }

    if(rocketY < 0.0f)  rocketY = 0.0f;
    if(rocketY > 0.35f) rocketY = 0.35f;

    // Rocket has settled back at the pad - stop the engine sound.
    if(rocketY <= 0.0f) {
        rocketMoving = false;
        stopEngineSound();
    }

    targetCameraY = (rocketY / 0.35f) * 0.25f;

    glutPostRedisplay();
}

void timer(int val)
{
    // 1. Move Car
    carX += 0.005f;
    if (carX > 1.1f) {
        carX = -0.15f;
    }

    // 2. Smoothly ease the vertical follow-cam (kept from before)
    cameraY += (targetCameraY - cameraY) * 0.1f;

    // 3. Smoothly ease the camera's look-direction angle (X-axis -> Z-axis)
    cameraAngle += (targetCameraAngle - cameraAngle) * 0.06f;

    // 4. While the rocket is ascending, play a throttled engine sound tick.
    //    Beep() blocks briefly, so we only call it every SOUND_INTERVAL_TICKS
    //    timer ticks instead of every single frame.
    if(rocketMoving)
    {
        soundTickCount++;
        if(soundTickCount >= SOUND_INTERVAL_TICKS)
        {
            playEngineTick();
            soundTickCount = 0;
        }
    }
    else
    {
        soundTickCount = 0;
    }

    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // --- 3D CAMERA ---------------------------------------------------------
    // Orbits its look-direction between the X-axis (angle=0, "front view")
    // and the Z-axis (angle=90, "side view"). Elevation is kept near-zero
    // and the camera sits far back with a narrow FOV so each object's own
    // 2D shape isn't visibly stretched/skewed by perspective - this keeps
    // shapes looking like the original flat artwork at any orbit angle,
    // while the Z-translation between objects still reveals real depth.
    float rad = cameraAngle * 3.1416f / 180.0f;

    float sceneCenterZ = (Z_GROUND_NEAR + Z_GROUND_FAR) * 0.5f; // ~ -0.65
    float centerX = 0.5f, centerY = 0.35f + cameraY, centerZ = sceneCenterZ;

    // Camera sits much further back than before; combined with the
    // narrower FOV in gluPerspective, this approximates a near-orthographic
    // view so individual object shapes stay undistorted at every angle.
    float orbitDist = camDistance + 6.0f;

    float eyeX = centerX + orbitDist * sin(rad);
    float eyeZ = centerZ + orbitDist * cos(rad);
    float eyeY = centerY; // no elevation - eye stays level with scene center,
                           // removing the top-down skew on every object

    gluLookAt(eyeX, eyeY, eyeZ,        // eye position
              centerX, centerY, centerZ, // look-at target (scene's true center)
              0.0f, 1.0f, 0.0f);       // up vector
    // -----------------------------------------------------------------------

    // ground - reverted to a flat strip at a single Z, same as the
    // original 2D layout (no longer stretched across depth).
    glBegin(GL_QUADS);
    if(isNight) glColor3f(0.05f, 0.25f, 0.02f);
    else        glColor3f(0.3f,  1.0f,  0.0f);
    glVertex3f(0.0, 0.08, Z_GROUND_FAR);
    glVertex3f(1.0, 0.08, Z_GROUND_FAR);
    glVertex3f(1.0, 0.40, Z_GROUND_FAR);
    glVertex3f(0.0, 0.40, Z_GROUND_FAR);
    glEnd();

    // road() draws a flat strip at Z_GROUND_FAR; drawCar() places the car
    // on that same Z so it's visible on the road from the front view.
    road();
    drawCar();

    // Sky elements (sun/moon/clouds/stars) pushed to the back-most band.
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, Z_SKY);
    if(isNight) { stars(); moon(); }
    else        { sun(); clouds();}
    glPopMatrix();

    // Wall/houses cluster sits behind the rocket stand.
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, Z_WALL);
    wall();
    glPopMatrix();

    // Rocket stand sits just behind the rocket itself.
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, Z_STAND);
    rocket_stand();
    glPopMatrix();

    // Rocket stays at the reference Z (0) - the focal point of the scene.
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, Z_ROCKET);
    rocket();
    glPopMatrix();

    glutSwapBuffers(); // double-buffered now (see glutInitDisplayMode below)
}

void init()
{
    glClearColor(0.4f, 0.7f, 1.0f, 1.0f);

    // --- Perspective narrowed (from 45 to ~12 degrees) and far plane
    // extended to match the much greater camera orbit distance now used
    // in display(). A narrow FOV at long distance approximates an
    // orthographic-style view, which is what keeps individual object
    // shapes from looking stretched/skewed at any camera angle.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(12.0, 800.0/600.0, 0.1, 20.0);
    // -----------------------------------------------------------------------

    glEnable(GL_DEPTH_TEST);
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    // NOTE: switched to double buffering (GLUT_DOUBLE) + depth buffer
    // (GLUT_DEPTH) since we now have real 3D perspective and a moving
    // camera; single buffering would show visible tearing/flicker here.
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Assignment Graphics");
    glutDisplayFunc(display);
    glutSpecialFunc(specialKey);
    glutKeyboardFunc(keyboard);

    glutTimerFunc(0, timer, 0);

    init();
    glutMainLoop();
    return 0;
}
