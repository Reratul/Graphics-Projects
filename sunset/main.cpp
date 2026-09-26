/*
 * Sunset Kite Scene
 * 21-CSE-040 | 21-CSE-006
 *
 * Scene: A boy flies a kite outside. As the sun sets (moves down),
 * the sky darkens, and the boy walks home toward the house.
 *
 * Compile:
 * g++ sunset_kite.cpp -o sunset_kite -lGL -lGLU -lglut
 * Or on Windows with freeglut:
 * g++ sunset_kite.cpp -o sunset_kite -lfreeglut -lopengl32 -lglu32
 */

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <cmath>
#include <cstdio>

// ─────────────────────────────────────────────
//  World dimensions
// ─────────────────────────────────────────────
const float W = 800.0f;
const float H = 600.0f;

// ─────────────────────────────────────────────
//  Animation state
// ─────────────────────────────────────────────
float sunY        = 520.0f;   // sun vertical position (starts high)
float sunX        = 680.0f;   // sun horizontal position
float sunRadius   = 35.0f;
float timeOfDay   = 1.0f;     // 1.0 = full day, 0.0 = night

// Boy state
float boyX        = 560.0f;   // boy position x
float boyY        = 210.0f;   // boy position y (ground level)
bool  boyWalking  = false;    // is the boy walking home?
bool  boyInside   = false;    // has the boy gone inside the house?
float homeX       = 160.0f;   // x position of the house door

// Kite
float kiteAngle   = 0.0f;     // gentle sway angle
float kiteSwayDir = 1.0f;
float kiteHeight  = 120.0f;   // controllable kite height

// Cloud drift
float cloud1X     = 80.0f;
float cloud2X     = 280.0f;

// Animation timer
int   animTimer   = 0;

// ─────────────────────────────────────────────
//  Utility: interpolate color based on timeOfDay
// ─────────────────────────────────────────────
void skyColor() {
    // Day sky -> orange sunset -> dark night
    float r, g, b;
    if (timeOfDay > 0.5f) {
        float t = (timeOfDay - 0.5f) * 2.0f; // 1=day, 0=sunset
        r = 0.4f + 0.4f * t;
        g = 0.6f + 0.2f * t;
        b = 0.9f * t;
    } else {
        float t = timeOfDay * 2.0f; // 1=sunset, 0=night
        r = 0.9f * t;
        g = 0.4f * t;
        b = 0.0f;
    }
    glClearColor(r, g, b, 1.0f);
}

// ─────────────────────────────────────────────
//  Drawing helpers
// ─────────────────────────────────────────────
void drawCircle(float cx, float cy, float r, int segs = 40) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= segs; i++) {
        float angle = 2.0f * 3.14159f * i / segs;
        glVertex2f(cx + r * cosf(angle), cy + r * sinf(angle));
    }
    glEnd();
}

void drawSunRays(float cx, float cy, float r) {
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    for (int i = 0; i < 12; i++) {
        float angle = 2.0f * 3.14159f * i / 12;
        glVertex2f(cx + (r + 5) * cosf(angle), cy + (r + 5) * sinf(angle));
        glVertex2f(cx + (r + 18) * cosf(angle), cy + (r + 18) * sinf(angle));
    }
    glEnd();
    glLineWidth(1.0f);
}

// Puffy cloud made of overlapping circles
void drawCloud(float x, float y) {
    glColor3f(1.0f, 1.0f, 1.0f);
    drawCircle(x,      y,      22);
    drawCircle(x + 25, y + 8,  26);
    drawCircle(x + 52, y,      20);
    drawCircle(x + 15, y - 10, 18);
    drawCircle(x + 38, y - 8,  18);
}

// Simple tree: brown trunk + green blob
void drawTree(float x, float y) {
    // Trunk
    glColor3f(0.4f, 0.25f, 0.1f);
    glBegin(GL_QUADS);
    glVertex2f(x - 6,  y);
    glVertex2f(x + 6,  y);
    glVertex2f(x + 6,  y + 50);
    glVertex2f(x - 6,  y + 50);
    glEnd();
    // Foliage
    glColor3f(0.1f, 0.55f, 0.1f);
    drawCircle(x, y + 70, 32);
    drawCircle(x - 20, y + 58, 22);
    drawCircle(x + 20, y + 58, 22);
}

// House: body, roof, door, window
void drawHouse(float x, float y) {
    // Body
    glColor3f(0.85f, 0.75f, 0.55f);
    glBegin(GL_QUADS);
    glVertex2f(x,       y);
    glVertex2f(x + 120, y);
    glVertex2f(x + 120, y + 80);
    glVertex2f(x,       y + 80);
    glEnd();

    // Roof
    glColor3f(0.6f, 0.2f, 0.1f);
    glBegin(GL_TRIANGLES);
    glVertex2f(x - 10,   y + 80);
    glVertex2f(x + 130,  y + 80);
    glVertex2f(x + 60,   y + 130);
    glEnd();

    // Door (Middle is at x + 60 = 120)
    glColor3f(0.4f, 0.25f, 0.1f);
    glBegin(GL_QUADS);
    glVertex2f(x + 45, y);
    glVertex2f(x + 75, y);
    glVertex2f(x + 75, y + 45);
    glVertex2f(x + 45, y + 45);
    glEnd();

    // Window left
    glColor3f(0.6f, 0.85f, 1.0f);
    glBegin(GL_QUADS);
    glVertex2f(x + 12,  y + 45);
    glVertex2f(x + 35,  y + 45);
    glVertex2f(x + 35,  y + 70);
    glVertex2f(x + 12,  y + 70);
    glEnd();

    // Window right
    glBegin(GL_QUADS);
    glVertex2f(x + 85,  y + 45);
    glVertex2f(x + 108, y + 45);
    glVertex2f(x + 108, y + 70);
    glVertex2f(x + 85,  y + 70);
    glEnd();
}

// Fence posts
void drawFence(float startX, float y, int posts) {
    glColor3f(0.55f, 0.38f, 0.2f);
    for (int i = 0; i < posts; i++) {
        float px = startX + i * 20.0f;
        // Post
        glBegin(GL_QUADS);
        glVertex2f(px,     y);
        glVertex2f(px + 6, y);
        glVertex2f(px + 6, y + 35);
        glVertex2f(px,     y + 35);
        glEnd();
        // Pointy top
        glBegin(GL_TRIANGLES);
        glVertex2f(px,     y + 35);
        glVertex2f(px + 6, y + 35);
        glVertex2f(px + 3, y + 42);
        glEnd();
    }
    // Horizontal rails
    glBegin(GL_QUADS);
    glVertex2f(startX, y + 10);
    glVertex2f(startX + posts * 20.0f, y + 10);
    glVertex2f(startX + posts * 20.0f, y + 14);
    glVertex2f(startX, y + 14);
    glEnd();
    glBegin(GL_QUADS);
    glVertex2f(startX, y + 24);
    glVertex2f(startX + posts * 20.0f, y + 24);
    glVertex2f(startX + posts * 20.0f, y + 28);
    glVertex2f(startX, y + 28);
    glEnd();
}

// Water pump / well silhouette in background
void drawWaterPump(float x, float y) {
    glColor3f(0.3f, 0.3f, 0.3f);
    // Two legs
    glLineWidth(3.0f);
    glBegin(GL_LINES);
    glVertex2f(x,      y);
    glVertex2f(x + 10, y + 50);
    glVertex2f(x + 20, y);
    glVertex2f(x + 10, y + 50);
    glEnd();
    // Cross bar
    glBegin(GL_LINES);
    glVertex2f(x - 5,  y + 45);
    glVertex2f(x + 25, y + 45);
    glEnd();
    // Hanging bucket
    glBegin(GL_LINES);
    glVertex2f(x + 10, y + 45);
    glVertex2f(x + 10, y + 30);
    glEnd();
    glLineWidth(1.0f);
    drawCircle(x + 10, y + 25, 7, 10);
}

// Ground / grass
void drawGround() {
    // Grass strip
    float g = 0.35f * timeOfDay;
    glColor3f(g * 0.5f, g, g * 0.2f);
    glBegin(GL_QUADS);
    glVertex2f(0,   0);
    glVertex2f(W,   0);
    glVertex2f(W,   195);
    glVertex2f(0,   195);
    glEnd();

    // Dirt path (river-like winding path toward house)
    glColor3f(0.65f * timeOfDay, 0.5f * timeOfDay, 0.35f * timeOfDay);
    glBegin(GL_QUADS);
    // Wide at bottom-center, narrows toward horizon
    glVertex2f(320, 0);
    glVertex2f(460, 0);
    glVertex2f(410, 195);
    glVertex2f(360, 195);
    glEnd();
}

// River / stream on right side
void drawRiver() {
    float blue = 0.6f + 0.3f * timeOfDay;
    glColor3f(0.15f, 0.35f, blue);
    glBegin(GL_QUADS);
    glVertex2f(580, 0);
    glVertex2f(650, 0);
    glVertex2f(620, 195);
    glVertex2f(560, 195);
    glEnd();
    // Shimmer
    glColor3f(0.5f, 0.7f, 1.0f);
    glLineWidth(1.5f);
    glBegin(GL_LINES);
    glVertex2f(582, 60);  glVertex2f(600, 55);
    glVertex2f(575, 100); glVertex2f(595, 95);
    glVertex2f(590, 140); glVertex2f(610, 135);
    glEnd();
    glLineWidth(1.0f);
}

// Horizon background figures
void drawBackgroundFigures() {
    float dim = 0.15f + 0.2f * timeOfDay;
    glColor3f(dim, dim * 0.6f, dim * 0.2f);
    // Small silhouette people on horizon
    float hy = 198.0f;
    float heights[] = { 18, 16, 20, 15, 17 };
    float xs[]      = { 230, 300, 450, 510, 680 };
    for (int i = 0; i < 5; i++) {
        float hh = heights[i], hx = xs[i];
        drawCircle(hx, hy + hh * 0.78f, hh * 0.22f, 10); // head
        glBegin(GL_LINES);
        glVertex2f(hx, hy + hh * 0.5f); glVertex2f(hx, hy);      // body
        glVertex2f(hx - 5, hy + hh * 0.3f); glVertex2f(hx + 5, hy + hh * 0.3f); // arms
        glEnd();
    }
}

// ── Boy character ──
void drawBoy(float x, float y) {
    float dim = 0.4f + 0.6f * timeOfDay;

    // Legs (walking animation)
    float legSwing = 0.0f;
    if (boyWalking) {
        legSwing = 12.0f * sinf(animTimer * 0.25f);
    }

    glColor3f(0.2f * dim, 0.4f * dim, 0.7f * dim); // pants
    glLineWidth(3.0f);
    glBegin(GL_LINES);
    // Left leg
    glVertex2f(x, y);
    glVertex2f(x - 7 + legSwing, y - 22);
    // Right leg
    glVertex2f(x, y);
    glVertex2f(x + 7 - legSwing, y - 22);
    glEnd();

    // Body
    glColor3f(0.8f * dim, 0.3f * dim, 0.1f * dim); // shirt
    glBegin(GL_QUADS);
    glVertex2f(x - 8,  y);
    glVertex2f(x + 8,  y);
    glVertex2f(x + 8,  y + 26);
    glVertex2f(x - 8,  y + 26);
    glEnd();

    // Arms
    glColor3f(0.8f * dim, 0.6f * dim, 0.45f * dim);
    glLineWidth(2.5f);
    glBegin(GL_LINES);
    // Right arm (raised, holding kite string)
    if (!boyWalking) {
        glVertex2f(x + 8, y + 22);
        glVertex2f(x + 20, y + 36); // arm up holding string
    } else {
        float armSwing = 10.0f * sinf(animTimer * 0.25f);
        glVertex2f(x + 8, y + 20);
        glVertex2f(x + 16, y + 10 + armSwing);
        glVertex2f(x - 8, y + 20);
        glVertex2f(x - 16, y + 10 - armSwing);
    }
    glEnd();
    glLineWidth(1.0f);

    // Head
    glColor3f(0.85f * dim, 0.65f * dim, 0.45f * dim);
    drawCircle(x, y + 33, 10, 20);

    // Hat
    glColor3f(0.3f * dim, 0.2f * dim, 0.1f * dim);
    glBegin(GL_QUADS);
    glVertex2f(x - 13, y + 40);
    glVertex2f(x + 13, y + 40);
    glVertex2f(x + 10, y + 48);
    glVertex2f(x - 10, y + 48);
    glEnd();
}

// ── Kite + string ──
void drawKite(float bx, float by) {
    if (boyInside) return; // hide kite completely when boy goes inside

    // String from boy's hand to kite
    float kx = bx + 60 + 40.0f * sinf(kiteAngle * 3.14159f / 180.0f);
    float ky = by + kiteHeight + 20.0f * cosf(kiteAngle * 3.14159f / 180.0f);

    glColor3f(0.3f, 0.3f, 0.3f);
    glLineWidth(1.2f);
    glBegin(GL_LINE_STRIP);
    glVertex2f(bx + 20, by + 36); // hand
    glVertex2f(bx + 40, by + 80);
    glVertex2f(kx,      ky);
    glEnd();
    glLineWidth(1.0f);

    // Kite diamond
    float dim = 0.5f + 0.5f * timeOfDay;
    glBegin(GL_QUADS);
    glColor3f(1.0f * dim, 0.1f, 0.1f);
    glVertex2f(kx,       ky + 20); // top
    glVertex2f(kx + 15,  ky);      // right
    glColor3f(0.1f, 0.5f * dim, 1.0f * dim);
    glVertex2f(kx,       ky - 20); // bottom
    glVertex2f(kx - 15,  ky);      // left
    glEnd();

    // Kite tail
    glColor3f(1.0f * dim, 0.8f * dim, 0.0f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_STRIP);
    glVertex2f(kx, ky - 20);
    glVertex2f(kx - 5,  ky - 32);
    glVertex2f(kx + 5,  ky - 44);
    glVertex2f(kx - 3,  ky - 56);
    glEnd();
    glLineWidth(1.0f);
}

// ─────────────────────────────────────────────
//  Main display
// ─────────────────────────────────────────────
void display() {
    skyColor();
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // ── Sun (Drawn first so it disappears behind the ground) ──
    float sunDim = 0.5f + 0.5f * timeOfDay;
    glColor3f(1.0f, 0.85f * sunDim, 0.0f);
    drawCircle(sunX, sunY, sunRadius);
    glColor3f(1.0f, 0.6f * sunDim, 0.0f);
    drawSunRays(sunX, sunY, sunRadius);

    // ── Ground ──
    drawGround();
    drawRiver();

    // ── Horizon background ──
    drawBackgroundFigures();
    drawWaterPump(385, 200);

    // ── Scenery ──
    drawHouse(60.0f, 195.0f);
    drawFence(185.0f, 195.0f, 8);
    drawTree(240.0f, 195.0f);
    drawTree(700.0f, 195.0f);

    // ── Clouds (drift slowly) ──
    drawCloud(cloud1X, 480);
    drawCloud(cloud2X, 510);

    // ── Kite ──
    drawKite(boyX, boyY);

    // ── Boy ──
    if (!boyInside) {
        drawBoy(boyX, boyY);
    }

    // ── HUD label ──
    glColor3f(1, 1, 1);
    const char* label = !boyInside ? "SPACE: Sunset | W/S: Kite Height" : "Night - Boy is inside the home!";
    glRasterPos2f(10, 10);
    for (const char* c = label; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }

    // Student IDs
    glRasterPos2f(10, H - 20);
    const char* ids = "21-CSE-040 | 21-CSE-006";
    for (const char* c = ids; *c; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }

    glFlush();
    glutSwapBuffers();
}

// ─────────────────────────────────────────────
//  Timer: animate everything
// ─────────────────────────────────────────────
void timer(int) {
    animTimer++;

    // Sun sets gradually
    if (sunY > 160.0f) {
        sunY   -= 0.15f;
        sunX   -= 0.05f;
        timeOfDay = (sunY - 160.0f) / (520.0f - 160.0f);
        timeOfDay = timeOfDay < 0 ? 0 : timeOfDay;
    }

    // When sun gets low, boy starts walking home
    if (sunY < 300.0f && !boyWalking) {
        boyWalking = true;
    }

    // Boy walks left toward house door (Door is at x = 120)
    if (boyWalking) {
        if (boyX > 120.0f) {
            boyX -= 0.6f;
        } else {
            boyInside = true; // Enter home
        }
    }

    // Kite sways
    kiteAngle += 0.8f * kiteSwayDir;
    if (kiteAngle > 18.0f || kiteAngle < -18.0f) kiteSwayDir *= -1.0f;

    // Clouds drift right
    cloud1X += 0.12f;
    cloud2X += 0.08f;
    if (cloud1X > W) cloud1X = -120;
    if (cloud2X > W) cloud2X = -120;

    glutPostRedisplay();
    glutTimerFunc(16, timer, 0); // ~60 fps
}

// ─────────────────────────────────────────────
//  Keyboard controls
// ─────────────────────────────────────────────
void keyboard(unsigned char key, int, int) {
    if (key == ' ') {
        // Manual speed-up: jump the sun down faster
        sunY      -= 15.0f;
        timeOfDay  = (sunY - 160.0f) / (520.0f - 160.0f);
        if (timeOfDay < 0) timeOfDay = 0;
    }

    // Kite control (Higher / Lower)
    if ((key == 'w' || key == 'W') && !boyInside) {
        kiteHeight += 10.0f;
        if (kiteHeight > 360.0f) kiteHeight = 360.0f; // boundary check
    }
    if ((key == 's' || key == 'S') && !boyInside) {
        kiteHeight -= 10.0f;
        if (kiteHeight < 40.0f) kiteHeight = 40.0f;   // boundary check
    }

    if (key == 'r' || key == 'R') {
        // Reset
        sunY = 520.0f; sunX = 680.0f;
        timeOfDay = 1.0f;
        boyX = 560.0f; boyWalking = false; boyInside = false;
        kiteAngle = 0.0f;
        kiteHeight = 120.0f;
    }
    if (key == 27) exit(0); // ESC
    glutPostRedisplay();
}

// ─────────────────────────────────────────────
//  Init & Main
// ─────────────────────────────────────────────
void init() {
    glClearColor(0.4f, 0.6f, 0.9f, 1.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, W, 0.0, H, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowPosition(100, 80);
    glutInitWindowSize((int)W, (int)H);
    glutCreateWindow("Sunset Kite Scene - 21-CSE-040 | 21-CSE-006");

    init();
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(16, timer, 0);

    printf("Controls:\n");
    printf("  SPACE  - Speed up sunset\n");
    printf("  W / S  - Fly kite higher / lower\n");
    printf("  R      - Reset scene\n");
    printf("  ESC    - Quit\n");

    glutMainLoop();
    return 0;
}
