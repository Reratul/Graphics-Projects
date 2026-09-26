#define GLUT_DISABLE_ATEXIT_HACK
#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

#define PI 3.14159265f
#define NUM_PASSENGERS 8

// =====================================================================
// Global Variables & States
// =====================================================================

// Mouse Interaction & Camera States
float camAngle = -40.0f;
float camHeight = 22.0f;
float camDist = 110.0f;
int startX, startY;
bool isDragging = false;

// Airplane Dynamics
float planeX = -25.0f;
float planeY = 5.2f;
float planeZ = 5.0f;
float planePitch = 0.0f;
float fanAngle = 0.0f;
float flapAngle = 0.0f;
bool gearDown = true;

// Simulation logic conditions
bool landingMode = false;
bool takeoffMode = false;
bool landed = true;

float runwayTargetX = -25.0f;
float runwayTargetY = 5.2f;
float runwayTargetZ = 5.0f;

float fanSpeed = 12.0f;
float beaconAngle = 0.0f;
bool beaconState = true;

// Mechanical control definitions
float doorAngle = 0.0f;
bool doorOpen = false;
float stairOffset = 0.0f;
float radarAngle = 0.0f;

// Cloud
float cloudX = 0.0f;

// Sun & Environment
float sunY = 55.0f;
float sunX = -65.0f;
bool isDaytime = true;
float skyR = 0.60f, skyG = 0.75f, skyB = 0.95f;

// Passenger System Structures & States
typedef enum {
    STATE_INSIDE_PLANE,
    STATE_DISEMBARKING,
    STATE_WAITING_OUTSIDE,
    STATE_BOARDING,
    STATE_BOARDED
} PassengerState;

typedef struct {
    float x, y, z;
    float targetX, targetY, targetZ;
    float r, g, b; // Random cloth colors
    PassengerState state;
} Passenger;

Passenger passengers[NUM_PASSENGERS];
bool startManWalking = false; 
bool boardingMode = false;     // Triggers boarding sequence before actual takeoff

/* =====================================================================
   Text Rendering Helper
   ===================================================================== */
void draw3DText(const char *text)
{
    glPushMatrix();
    glScalef(0.018f, 0.018f, 0.018f);
    glLineWidth(3.0f);
    for (const char *c = text; *c != '\0'; c++)
    {
        glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
    }
    glPopMatrix();
}

/* =====================================================================
   Scenery & Environment Logic
   ===================================================================== */
void drawCitySkyline()
{
    glEnable(GL_LIGHTING);
    for (float x = -120; x <= 120; x += 24)
    {
        float bHeight = 45.0f + sin(x * 0.5f) * 15.0f;

        if (isDaytime)
        {
            if ((int)x % 48 == 0)
                glColor3f(0.25f, 0.28f, 0.32f);
            else
                glColor3f(0.30f, 0.33f, 0.38f);
        }
        else
        {
            glColor3f(0.05f, 0.05f, 0.08f);
        }

        glPushMatrix();
        glTranslatef(x + 5.0f, bHeight / 2.0f, -95.0f);
        glScalef(14.0f, bHeight, 12.0f);
        glutSolidCube(1);
        glPopMatrix();

        glDisable(GL_LIGHTING);
        if (isDaytime)
            glColor3f(0.95f, 0.90f, 0.60f);
        else
            glColor3f(1.0f, 0.95f, 0.50f);
            
        for (float wh = 10.0f; wh < bHeight - 5.0f; wh += 12.0f)
        {
            glPushMatrix();
            glTranslatef(x + 5.0f, wh, -88.9f);
            glScalef(1.5f, 2.0f, 0.1f);
            glutSolidCube(1);
            glPopMatrix();
        }
        glEnable(GL_LIGHTING);
    }
}

void drawSun()
{
    glPushMatrix();
    glTranslatef(sunX, sunY, -140);
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 0.88f, 0.40f);
    glutSolidSphere(10.0, 32, 32);
    glEnable(GL_LIGHTING);
    glPopMatrix();
}

void drawMountains()
{
    glEnable(GL_LIGHTING);

    if (isDaytime)
        glColor3f(0.48f, 0.42f, 0.33f);
    else
        glColor3f(0.08f, 0.07f, 0.06f);

    glBegin(GL_TRIANGLE_STRIP);
    glVertex3f(-160, 0, -85);
    glVertex3f(-110, 38, -85);
    glVertex3f(-60, 0, -85);
    glVertex3f(-10, 42, -85);
    glVertex3f(40, 0, -85);
    glVertex3f(90, 35, -85);
    glVertex3f(160, 0, -85);
    glEnd();

    if (isDaytime)
        glColor3f(0.42f, 0.37f, 0.28f);
    else
        glColor3f(0.06f, 0.05f, 0.04f);

    glBegin(GL_TRIANGLES);
    glVertex3f(-110, 0, -90);
    glVertex3f(-50, 48, -90);
    glVertex3f(20, 0, -90);
    glVertex3f(10, 0, -90);
    glVertex3f(75, 44, -90);
    glVertex3f(150, 0, -90);
    glEnd();
}

void drawCloud(float x, float y, float z)
{
    glDisable(GL_LIGHTING);
    if (isDaytime)
        glColor3f(0.98f, 0.98f, 1.0f);
    else
        glColor3f(0.18f, 0.18f, 0.22f);
        
    glPushMatrix();
    glTranslatef(x, y, z);
    glutSolidSphere(6.0, 24, 24);
    glTranslatef(5.0f, -0.5f, 0);
    glutSolidSphere(4.5, 20, 20);
    glTranslatef(-10.0f, 0.3f, 0);
    glutSolidSphere(4.5, 20, 20);
    glTranslatef(5.0f, 2.5f, -1.0f);
    glutSolidSphere(4.0, 20, 20);
    glPopMatrix();
    glEnable(GL_LIGHTING);
}

void drawTree(float x, float z)
{
    GLUquadric *q = gluNewQuadric();
    glPushMatrix();
    glTranslatef(x, 0, z);

    if (isDaytime)
        glColor3f(0.40f, 0.26f, 0.16f);
    else
        glColor3f(0.08f, 0.05f, 0.03f);

    glPushMatrix();
    glRotatef(-90, 1, 0, 0);
    gluCylinder(q, 0.55, 0.30, 5.2, 12, 12);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0, 3.4f, 0);
    glRotatef(35, 0, 0, 1);
    glRotatef(-90, 1, 0, 0);
    gluCylinder(q, 0.18, 0.08, 1.6, 8, 8);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0, 3.0f, 0);
    glRotatef(-40, 0, 0, 1);
    glRotatef(-90, 1, 0, 0);
    gluCylinder(q, 0.18, 0.08, 1.6, 8, 8);
    glPopMatrix();

    glTranslatef(0, 5.0f, 0);

    if (isDaytime)
        glColor3f(0.20f, 0.42f, 0.16f);
    else
        glColor3f(0.04f, 0.08f, 0.03f);
    glutSolidSphere(2.6, 14, 14);

    if (isDaytime)
        glColor3f(0.26f, 0.50f, 0.20f);
    else
        glColor3f(0.05f, 0.10f, 0.04f);

    glPushMatrix();
    glTranslatef(1.6f, 0.8f, 0.5f);
    glutSolidSphere(1.7, 12, 12);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-1.6f, 0.6f, -0.6f);
    glutSolidSphere(1.7, 12, 12);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0.4f, 1.6f, 1.4f);
    glutSolidSphere(1.6, 12, 12);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-0.6f, 1.8f, -1.2f);
    glutSolidSphere(1.6, 12, 12);
    glPopMatrix();

    if (isDaytime)
        glColor3f(0.34f, 0.58f, 0.26f);
    else
        glColor3f(0.06f, 0.12f, 0.05f);
        
    glPushMatrix();
    glTranslatef(0, 2.6f, 0);
    glutSolidSphere(1.4, 12, 12);
    glPopMatrix();

    glPopMatrix();
    gluDeleteQuadric(q);
}

void drawAirportTerminalAndTower()
{
    GLUquadric *q = gluNewQuadric();
    glPushMatrix();
    glRotatef(12.0f, 0, 1, 0);
    glTranslatef(0, 0, -45);

    // Terminal Complex Base
    if (isDaytime)
        glColor3f(0.68f, 0.70f, 0.72f);
    else
        glColor3f(0.08f, 0.08f, 0.10f);
        
    glPushMatrix();
    glTranslatef(12.0f, 8.0f, 0);
    glScalef(52, 16, 24);
    glutSolidCube(1);
    glPopMatrix();

    // "AIRPORT" Text
    glDisable(GL_LIGHTING);
    if (isDaytime)
        glColor3f(0.0f, 0.0f, 0.0f);
    else
        glColor3f(0.9f, 0.85f, 0.3f);
        
    glPushMatrix();
    glTranslatef(0.0f, 16.5f, 12.3f);
    draw3DText("AIRPORT");
    glPopMatrix();
    glEnable(GL_LIGHTING);

    // Glass Windows
    if (isDaytime)
        glColor3f(0.18f, 0.45f, 0.70f);
    else
        glColor3f(0.95f, 0.88f, 0.35f);

    for (float i = -10; i <= 34; i += 8)
    {
        glPushMatrix();
        glTranslatef(i, 5.0f, 12.25f);
        glScalef(6.0f, 5.0f, 0.5f);
        glutSolidCube(1);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(i, 11.0f, 12.25f);
        glScalef(6.0f, 5.0f, 0.5f);
        glutSolidCube(1);
        glPopMatrix();
    }

    // ATC Tower Shaft
    glPushMatrix();
    glTranslatef(-28, 0, -2);

    if (isDaytime)
        glColor3f(0.92f, 0.92f, 0.95f);
    else
        glColor3f(0.10f, 0.10f, 0.12f);
        
    glPushMatrix();
    glRotatef(-90, 1, 0, 0);
    gluCylinder(q, 4.0, 3.4, 28, 20, 20);
    glPopMatrix();

    // Observation Deck
    glTranslatef(0, 26, 0);
    if (isDaytime)
        glColor3f(0.18f, 0.35f, 0.50f);
    else
        glColor3f(0.05f, 0.10f, 0.15f);
        
    glPushMatrix();
    glRotatef(-90, 1, 0, 0);
    gluCylinder(q, 4.0, 5.8, 6.0, 16, 16);
    glPopMatrix();

    if (isDaytime)
        glColor3f(0.45f, 0.47f, 0.50f);
    else
        glColor3f(0.08f, 0.08f, 0.10f);
        
    glPushMatrix();
    glTranslatef(0, 6.0f, 0);
    glRotatef(-90, 1, 0, 0);
    gluDisk(q, 0, 5.9, 16, 1);
    glPopMatrix();

    // Beacon
    glDisable(GL_LIGHTING);
    glPushMatrix();
    glTranslatef(0, 6.6f, 0);
    if (landed && !landingMode && !takeoffMode)
        glColor3f(0.0f, 1.0f, 0.0f);
    else if (landingMode)
        glColor3f(1.0f, 0.0f, 0.0f);
    else if (beaconState)
        glColor3f(1.0f, 0.0f, 0.0f);
    else
        glColor3f(0.2f, 0.0f, 0.0f);
    glutSolidSphere(1.2, 14, 14);
    glPopMatrix();
    glEnable(GL_LIGHTING);

    // Radar
    glPushMatrix();
    glTranslatef(0, 7.5f, 0);
    glRotatef(radarAngle, 0, 1, 0);
    if (isDaytime)
        glColor3f(0.25f, 0.25f, 0.27f);
    else
        glColor3f(0.08f, 0.08f, 0.09f);
    glScalef(6.5f, 0.4f, 1.0f);
    glutSolidCube(1);
    glPopMatrix();

    glPopMatrix();
    glPopMatrix();
    gluDeleteQuadric(q);
}

void drawRunway()
{
    glPushMatrix();
    glRotatef(12.0f, 0, 1, 0);

    if (isDaytime)
        glColor3f(0.18f, 0.18f, 0.20f);
    else
        glColor3f(0.06f, 0.06f, 0.07f);

    glBegin(GL_QUADS);
    glVertex3f(-120, 0.02f, -16);
    glVertex3f(120, 0.02f, -16);
    glVertex3f(120, 0.02f, 16);
    glVertex3f(-120, 0.02f, 16);
    glEnd();

    // Runway Lights
    if (isDaytime)
        glColor3f(0.95f, 0.95f, 0.95f);
    else
        glColor3f(0.9f, 0.85f, 0.3f);

    glBegin(GL_QUADS);
    glVertex3f(-120, 0.03f, -15.2f);
    glVertex3f(120, 0.03f, -15.2f);
    glVertex3f(120, 0.03f, -14.5f);
    glVertex3f(-120, 0.03f, -14.5f);
    glVertex3f(-120, 0.03f, 14.5f);
    glVertex3f(120, 0.03f, 14.5f);
    glVertex3f(120, 0.03f, 15.2f);
    glVertex3f(-120, 0.03f, 15.2f);
    glEnd();

    for (float x = -110; x < 110; x += 22)
    {
        glBegin(GL_QUADS);
        glVertex3f(x, 0.03f, -0.6f);
        glVertex3f(x + 10, 0.03f, -0.6f);
        glVertex3f(x + 10, 0.03f, 0.6f);
        glVertex3f(x, 0.03f, 0.6f);
        glEnd();
    }
    glPopMatrix();
}

void drawGroundAndFence()
{
    if (isDaytime)
        glColor3f(0.42f, 0.45f, 0.45f);
    else
        glColor3f(0.06f, 0.07f, 0.07f);

    glBegin(GL_QUADS);
    glVertex3f(-180, 0, -150);
    glVertex3f(180, 0, -150);
    glVertex3f(180, 0, 150);
    glVertex3f(-180, 0, 150);
    glEnd();

    glDisable(GL_LIGHTING);
    if (isDaytime)
        glColor3f(0.28f, 0.28f, 0.30f);
    else
        glColor3f(0.10f, 0.10f, 0.12f);
        
    glLineWidth(1.8f);
    glPushMatrix();
    glRotatef(12.0f, 0, 1, 0);
    glTranslatef(0, 0, 34);
    glBegin(GL_LINES);
    for (float x = -110; x < 110; x += 4.5f)
    {
        glVertex3f(x, 0, 0);
        glVertex3f(x, 6.0f, 0);
        glVertex3f(x, 0, 0);
        glVertex3f(x + 4.5f, 6.0f, 0);
        glVertex3f(x, 6.0f, 0);
        glVertex3f(x + 4.5f, 0, 0);
    }
    glVertex3f(-110, 6.0f, 0);
    glVertex3f(110, 6.0f, 0);
    glVertex3f(-110, 0, 0);
    glVertex3f(-110, 6.0f, 0);
    glVertex3f(110, 0, 0);
    glVertex3f(110, 6.0f, 0);
    glEnd();
    glPopMatrix();
    glEnable(GL_LIGHTING);
}

/* =====================================================================
   Airplane Subsystems & Passenger Rendering
   ===================================================================== */
void drawEngineFan()
{
    glPushMatrix();
    glRotatef(fanAngle, 0, 0, 1);
    glColor3f(0.10f, 0.10f, 0.12f);
    for (int i = 0; i < 6; i++)
    {
        glRotatef(60, 0, 0, 1);
        glBegin(GL_TRIANGLES);
        glVertex3f(0, 0, 0);
        glVertex3f(0.35f, 1.8f, 0);
        glVertex3f(-0.35f, 1.8f, 0);
        glEnd();
    }
    glPopMatrix();
}

void drawPassengerCharacter(Passenger p)
{
    glPushMatrix();
    glTranslatef(p.x, p.y, p.z);
    
    // Head
    glColor3f(0.95f, 0.75f, 0.65f);
    glPushMatrix();
    glTranslatef(0.0f, 1.2f, 0.0f);
    glutSolidSphere(0.35, 16, 16);
    glPopMatrix();
    
    // Body (Shirt/Jacket)
    glColor3f(p.r, p.g, p.b); 
    glPushMatrix();
    glTranslatef(0.0f, 0.5f, 0.0f);
    glScalef(0.5f, 0.9f, 0.4f);
    glutSolidCube(1);
    glPopMatrix();
    
    // Legs (Pant)
    glColor3f(0.1f, 0.1f, 0.15f);
    glPushMatrix();
    glTranslatef(0.0f, -0.3f, 0.0f);
    glScalef(0.4f, 0.7f, 0.3f);
    glutSolidCube(1);
    glPopMatrix();
    
    glPopMatrix();
}

void drawAirplane()
{
    GLUquadric *q = gluNewQuadric();
    glPushMatrix();

    glTranslatef(planeX, planeY, planeZ);
    glRotatef(102.0f, 0, 1, 0);
    glRotatef(planePitch, 0, 0, 1);

    // Fuselage
    if (isDaytime)
        glColor3f(0.94f, 0.95f, 0.97f);
    else
        glColor3f(0.15f, 0.15f, 0.18f);
    gluCylinder(q, 3.4, 3.4, 36, 24, 24);

    // Tail taper
    glPushMatrix();
    glRotatef(180, 0, 1, 0);
    gluCylinder(q, 3.4f, 1.0f, 4.0f, 20, 20);
    glTranslatef(0, 0, 4.0f);
    gluDisk(q, 0, 1.0f, 20, 1);
    glPopMatrix();

    // Nose
    glPushMatrix();
    glTranslatef(0, 0, 36);
    glScalef(1.0f, 1.0f, 1.8f);
    glutSolidSphere(3.4, 24, 24);
    glPopMatrix();

    // Wings
    if (isDaytime)
        glColor3f(0.88f, 0.90f, 0.92f);
    else
        glColor3f(0.12f, 0.12f, 0.14f);
        
    glPushMatrix();
    glTranslatef(0, -0.4f, 16);
    glBegin(GL_QUADS);
    glVertex3f(-25, 0, 0);
    glVertex3f(25, 0, 0);
    glVertex3f(19, 0, 8);
    glVertex3f(-19, 0, 8);
    glEnd();
    glPopMatrix();

    // Passenger Windows
    glDisable(GL_LIGHTING);
    if (isDaytime)
        glColor3f(0.1f, 0.1f, 0.15f);
    else
        glColor3f(0.95f, 0.88f, 0.35f);

    for (float wz = 10.0f; wz <= 30.0f; wz += 3.5f)
    {
        glPushMatrix();
        glTranslatef(3.35f, 1.0f, wz);
        glScalef(0.1f, 0.8f, 1.2f);
        glutSolidCube(1);
        glPopMatrix();
    }
    for (float wz = 10.0f; wz <= 30.0f; wz += 3.5f)
    {
        glPushMatrix();
        glTranslatef(-3.35f, 1.0f, wz);
        glScalef(0.1f, 0.8f, 1.2f);
        glutSolidCube(1);
        glPopMatrix();
    }
    glEnable(GL_LIGHTING);

    // Wing Flaps
    if (isDaytime)
        glColor3f(0.88f, 0.90f, 0.92f);
    else
        glColor3f(0.10f, 0.10f, 0.12f);
        
    glPushMatrix();
    glTranslatef(-20, -0.4f, 18);
    glRotatef(flapAngle, 1, 0, 0);
    glScalef(6.5f, 0.2f, 3.2f);
    glutSolidCube(1);
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(20, -0.4f, 18);
    glRotatef(flapAngle, 1, 0, 0);
    glScalef(6.5f, 0.2f, 3.2f);
    glutSolidCube(1);
    glPopMatrix();

    // Tail Assembly
    glDisable(GL_LIGHTING);
    if (isDaytime)
        glColor3f(0.88f, 0.90f, 0.92f);
    else
        glColor3f(0.12f, 0.12f, 0.14f);

    glPushMatrix();
    glTranslatef(0, 3.4f, 2.0f);
    {
        float t = 0.5f;
        glBegin(GL_POLYGON);
        glVertex3f(-t / 2, 0.0f, 6.0f);
        glVertex3f(-t / 2, 0.0f, 0.0f);
        glVertex3f(-t / 2, 11.0f, -3.5f);
        glVertex3f(-t / 2, 11.0f, -0.5f);
        glEnd();
        glBegin(GL_POLYGON);
        glVertex3f(t / 2, 0.0f, 0.0f);
        glVertex3f(t / 2, 0.0f, 6.0f);
        glVertex3f(t / 2, 11.0f, -0.5f);
        glVertex3f(t / 2, 11.0f, -3.5f);
        glEnd();
        glBegin(GL_QUAD_STRIP);
        glVertex3f(-t / 2, 0.0f, 6.0f);
        glVertex3f(t / 2, 0.0f, 6.0f);
        glVertex3f(-t / 2, 0.0f, 0.0f);
        glVertex3f(t / 2, 0.0f, 0.0f);
        glVertex3f(-t / 2, 11.0f, -3.5f);
        glVertex3f(t / 2, 11.0f, -3.5f);
        glVertex3f(-t / 2, 11.0f, -0.5f);
        glVertex3f(t / 2, 11.0f, -0.5f);
        glVertex3f(-t / 2, 0.0f, 6.0f);
        glVertex3f(t / 2, 0.0f, 6.0f);
        glEnd();
    }
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0, 0.5f, 3.0f);
    {
        float t = 0.3f;
        for (int side = -1; side <= 1; side += 2)
        {
            float tipX = side * 11.0f;
            glBegin(GL_TRIANGLES);
            glVertex3f(0.0f, t / 2, 4.0f);
            glVertex3f(tipX, t / 2, -1.5f);
            glVertex3f(0.0f, t / 2, 0.0f);
            glEnd();
            glBegin(GL_TRIANGLES);
            glVertex3f(0.0f, -t / 2, 4.0f);
            glVertex3f(0.0f, -t / 2, 0.0f);
            glVertex3f(tipX, -t / 2, -1.5f);
            glEnd();
            glBegin(GL_QUAD_STRIP);
            glVertex3f(0.0f, t / 2, 4.0f);
            glVertex3f(0.0f, -t / 2, 4.0f);
            glVertex3f(tipX, t / 2, -1.5f);
            glVertex3f(tipX, -t / 2, -1.5f);
            glVertex3f(0.0f, t / 2, 0.0f);
            glVertex3f(0.0f, -t / 2, 0.0f);
            glVertex3f(0.0f, t / 2, 4.0f);
            glVertex3f(0.0f, -t / 2, 4.0f);
            glEnd();
        }
    }
    glPopMatrix();
    glEnable(GL_LIGHTING);

    // Engines
    for (int side = -1; side <= 1; side += 2)
    {
        glPushMatrix();
        glTranslatef(side * 11.5f, -2.6f, 20.5f);
        glRotatef(180, 0, 1, 0);
        if (isDaytime)
            glColor3f(0.40f, 0.42f, 0.44f);
        else
            glColor3f(0.08f, 0.08f, 0.09f);
        gluCylinder(q, 2.0, 2.0, 6.5, 20, 20);
        glTranslatef(0, 0, 0.2f);
        drawEngineFan();
        glPopMatrix();
    }

    // Landing Gear
    if (gearDown)
    {
        glPushMatrix();
        glTranslatef(0, -3.4f, 28);
        glColor3f(0.7f, 0.7f, 0.7f);
        glPushMatrix();
        glRotatef(90, 1, 0, 0);
        gluCylinder(q, 0.15, 0.15, 2.5, 8, 8);
        glPopMatrix();
        glTranslatef(0, -2.5f, 0);
        glColor3f(0.12f, 0.12f, 0.12f);
        glPushMatrix();
        glTranslatef(-0.4f, 0, 0);
        glRotatef(90, 0, 1, 0);
        gluCylinder(q, 1.0, 1.0, 0.3, 16, 8);
        gluDisk(q, 0, 1.0, 16, 1);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(0.1f, 0, 0);
        glRotatef(90, 0, 1, 0);
        gluCylinder(q, 1.0, 1.0, 0.3, 16, 8);
        gluDisk(q, 0, 1.0, 16, 1);
        glPopMatrix();
        glPopMatrix();

        glPushMatrix();
        glTranslatef(-6.2f, -3.4f, 13);
        glColor3f(0.7f, 0.7f, 0.7f);
        glPushMatrix();
        glRotatef(90, 1, 0, 0);
        gluCylinder(q, 0.22, 0.22, 2.5, 8, 8);
        glPopMatrix();
        glTranslatef(0, -2.5f, 0);
        glColor3f(0.12f, 0.12f, 0.12f);
        glPushMatrix();
        glTranslatef(-0.5f, 0, 0);
        glRotatef(90, 0, 1, 0);
        gluCylinder(q, 1.3, 1.3, 0.4, 16, 8);
        gluDisk(q, 0, 1.3, 16, 1);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(0.1f, 0, 0);
        glRotatef(90, 0, 1, 0);
        gluCylinder(q, 1.3, 1.3, 0.4, 16, 8);
        gluDisk(q, 0, 1.3, 16, 1);
        glPopMatrix();
        glPopMatrix();

        glPushMatrix();
        glTranslatef(6.2f, -3.4f, 13);
        glColor3f(0.7f, 0.7f, 0.7f);
        glPushMatrix();
        glRotatef(90, 1, 0, 0);
        gluCylinder(q, 0.22, 0.22, 2.5, 8, 8);
        glPopMatrix();
        glTranslatef(0, -2.5f, 0);
        glColor3f(0.12f, 0.12f, 0.12f);
        glPushMatrix();
        glTranslatef(-0.5f, 0, 0);
        glRotatef(90, 0, 1, 0);
        gluCylinder(q, 1.3, 1.3, 0.4, 16, 8);
        gluDisk(q, 0, 1.3, 16, 1);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(0.1f, 0, 0);
        glRotatef(90, 0, 1, 0);
        gluCylinder(q, 1.3, 1.3, 0.4, 16, 8);
        gluDisk(q, 0, 1.3, 16, 1);
        glPopMatrix();
        glPopMatrix();
    }

    // Door interior dark
    glDisable(GL_LIGHTING);
    glPushMatrix();
    glTranslatef(3.30f, 0.8f, 24.0f);
    glColor3f(0.00f, 0.00f, 0.00f);
    glScalef(0.05f, 3.2f, 2.05f);
    glutSolidCube(1);
    glPopMatrix();
    glEnable(GL_LIGHTING);

    // Door
    glPushMatrix();
    glTranslatef(3.41f, 0.8f, 24.0f);
    glTranslatef(0, 0, -1.1f);
    glRotatef(doorAngle, 0, 1, 0);
    glTranslatef(0, 0, 1.1f);
    if (isDaytime)
        glColor3f(0.60f, 0.62f, 0.65f);
    else
        glColor3f(0.10f, 0.10f, 0.12f);
    glScalef(0.25f, 3.4f, 2.2f);
    glutSolidCube(1);
    glPopMatrix();

    // Stairs
    if (doorOpen || stairOffset < -0.01f)
    {
        glPushMatrix();
        glTranslatef(5.56f, -3.05f, 24.0f);
        glRotatef(-45.0f, 0, 0, 1);
        if (isDaytime)
            glColor3f(0.75f, 0.75f, 0.75f);
        else
            glColor3f(0.15f, 0.15f, 0.15f);
        glScalef(8.5f, 0.3f, 2.2f);
        glutSolidCube(1);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(7.7f, -4.75f, 24.0f);
        glScalef(0.5f, 0.9f, 0.5f);
        if (isDaytime)
            glColor3f(0.5f, 0.5f, 0.5f);
        else
            glColor3f(0.10f, 0.10f, 0.10f);
        glutSolidCube(1);
        glPopMatrix();
    }

    // =====================================================================
    // DRAW MULTIPLE PASSENGERS (Fixed inside Airplane Matrix block)
    // =====================================================================
    for (int i = 0; i < NUM_PASSENGERS; i++)
    {
        // Only draw passengers if they are currently active in the sequence
        if (passengers[i].state != STATE_BOARDED)
        {
            drawPassengerCharacter(passengers[i]);
        }
    }

    gluDeleteQuadric(q);
    glPopMatrix(); // Airplane main block ends
}

/* =====================================================================
   Animation Cycles System
   ===================================================================== */
void update(int value)
{
    radarAngle += 1.8f;
    if (radarAngle > 360)
        radarAngle = 0;

    cloudX += 0.05f;
    if (cloudX > 300.0f)
        cloudX = -300.0f;

    if (isDaytime)
    {
        if (sunY < 55.0f) sunY += 0.15f;
        if (sunX < -65.0f) sunX += 0.08f;
        skyR += 0.002f; if (skyR > 0.60f) skyR = 0.60f;
        skyG += 0.002f; if (skyG > 0.75f) skyG = 0.75f;
        skyB += 0.002f; if (skyB > 0.95f) skyB = 0.95f;
    }
    else
    {
        if (sunY > -20.0f) sunY -= 0.15f;
        if (sunX > -100.0f) sunX -= 0.08f;
        skyR -= 0.002f; if (skyR < 0.02f) skyR = 0.02f;
        skyG -= 0.002f; if (skyG < 0.02f) skyG = 0.02f;
        skyB -= 0.002f; if (skyB < 0.05f) skyB = 0.05f;
    }

    fanAngle += fanSpeed;
    if (fanAngle > 360)
        fanAngle -= 360;

    // Door and Stair mechanical animation logic
    if (doorOpen)
    {
        if (doorAngle < 85.0f) doorAngle += 3.0f;
        if (stairOffset > -1.6f) stairOffset -= 0.1f;
    }
    else
    {
        if (doorAngle > 0.0f) doorAngle -= 3.0f;
        if (stairOffset < 0.0f) stairOffset += 0.1f;
    }

    // MULTI-PASSENGER ANIMATION LOGIC (CRITICAL FIXES FOR VISIBILITY)
    bool allBoarded = true;
    for (int i = 0; i < NUM_PASSENGERS; i++)
    {
        // 1. Disembarking Logic (M Key pressed)
        // FIX: previously triggered as soon as passenger (i-1) merely CHANGED
        // state to DISEMBARKING, which happens the very same frame for many
        // passengers in a row (state changes are instant, not gradual), so
        // everybody started walking together and looked like duplicates
        // spawning at once. Now passenger i only starts once passenger (i-1)
        // has actually walked far enough away from the doorway (x > 3.6f),
        // which guarantees a visible gap between consecutive passengers —
        // just like a real boarding/deboarding queue.
        if (startManWalking && passengers[i].state == STATE_INSIDE_PLANE && doorAngle >= 85.0f)
        {
            if (i == 0 ||
                passengers[i-1].state == STATE_WAITING_OUTSIDE ||
                (passengers[i-1].state == STATE_DISEMBARKING && passengers[i-1].x > 3.6f))
            {
                passengers[i].state = STATE_DISEMBARKING;
            }
        }

        if (passengers[i].state == STATE_DISEMBARKING)
        {
            // Move outwards towards the door edge
            if (passengers[i].x < 4.5f) 
            {
                passengers[i].x += 0.04f; 
            }
            // Step down the stairs properly
            else if (passengers[i].x >= 4.5f && passengers[i].x < 8.2f)
            {
                passengers[i].x += 0.04f;
                passengers[i].y -= 0.04f; 
            }
            // Walk further away out onto the platform/ground area
            else if (passengers[i].x >= 8.2f && passengers[i].x < passengers[i].targetX)
            {
                passengers[i].x += 0.05f; 
            }
            else 
            {
                passengers[i].state = STATE_WAITING_OUTSIDE;
            }
        }

        // 2. Boarding Logic (T Key pressed)
        // FIX: same issue as disembarking — wait until passenger (i-1) has
        // actually cleared the stair/door area (x < 7.0f) before passenger i
        // starts walking toward the plane, so boarding also happens one by
        // one with a visible gap instead of everyone moving at once.
        if (boardingMode && passengers[i].state == STATE_WAITING_OUTSIDE)
        {
            if (i == 0 ||
                passengers[i-1].state == STATE_BOARDED ||
                (passengers[i-1].state == STATE_BOARDING && passengers[i-1].x < 7.0f))
            {
                passengers[i].state = STATE_BOARDING;
            }
        }

        if (passengers[i].state == STATE_BOARDING)
        {
            if (passengers[i].x > 8.2f)
            {
                passengers[i].x -= 0.05f;
            }
            else if (passengers[i].x <= 8.2f && passengers[i].x > 4.5f)
            {
                passengers[i].x -= 0.04f;
                passengers[i].y += 0.04f;
            }
            else if (passengers[i].x <= 4.5f && passengers[i].x > 2.8f) // Clear position inside
            {
                passengers[i].x -= 0.04f;
            }
            else
            {
                passengers[i].state = STATE_BOARDED;
            }
        }

        if (passengers[i].state != STATE_BOARDED) {
            allBoarded = false;
        }
    }

    // Trigger exact Takeoff phase after everyone boards and door closes
    if (boardingMode && allBoarded)
    {
        doorOpen = false;
        if (doorAngle <= 0.0f) {
            boardingMode = false;
            takeoffMode = true;
            landed = false;
        }
    }

    beaconAngle += 1.0f;
    if ((int)beaconAngle % 22 == 0)
        beaconState = !beaconState;

    if (landingMode)
    {
        gearDown = true;
        fanSpeed = 16.0f;
        planePitch = -2.5f;

        if (planeX < runwayTargetX) planeX += 0.2f;
        if (planeX > runwayTargetX) planeX -= 0.2f;

        planeZ = 5.0f + (planeX * tan(12.0f * PI / 180.0f));
        if (planeY > runwayTargetY) planeY -= 0.18f;

        if (planeY <= runwayTargetY)
        {
            planeY = runwayTargetY;
            planePitch = 0.0f;
            landed = true;
            landingMode = false;
        }
    }

    if (landed && !takeoffMode && !boardingMode)
    {
        if (planeX < 5.0f)
        {
            planeX += 0.30f;
            planeZ = 5.0f + (planeX * tan(12.0f * PI / 180.0f));
            if (fanSpeed > 3.0f) fanSpeed -= 0.12f;
        }
        else
        {
            if (fanSpeed > 0.0f) fanSpeed -= 0.25f;
            if (fanSpeed < 0.5f) fanSpeed = 0.0f;
        }
    }

    if (takeoffMode)
    {
        fanSpeed = 65.0f;
        planePitch = 7.5f;
        planeX += 0.1f;
        planeZ = 5.0f + (planeX * tan(12.0f * PI / 180.0f));
        planeY += 0.1f;

        if (planeY > 65.0f)
        {
            takeoffMode = false;
            landed = false;
        }
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

/* =====================================================================
   Display
   ===================================================================== */
void display()
{
    glClearColor(skyR, skyG, skyB, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    float rad = camAngle * PI / 180.0f;
    float camX = camDist * sin(rad);
    float camZ = camDist * cos(rad);
    gluLookAt(camX, camHeight, camZ, -10, 3, -5, 0, 1, 0);

    drawGroundAndFence();
    drawSun();
    drawCitySkyline();
    drawMountains();

    drawCloud(-75 + cloudX, 58, -115);
    drawCloud(5 + cloudX, 62, -115);
    drawCloud(80 + cloudX, 56, -115);

    glPushMatrix();
    glRotatef(12.0f, 0, 1, 0);
    for (float i = -85; i <= -35; i += 10)
        drawTree(i, -56.0f);
    for (float i = 45; i <= 85; i += 10)
        drawTree(i, -56.0f);
    glPopMatrix();

    drawAirportTerminalAndTower();
    drawRunway();
    drawAirplane();

    glutSwapBuffers();
}

/* =====================================================================
   Input Handlers
   ===================================================================== */
void mouseButton(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON)
    {
        if (state == GLUT_DOWN) { isDragging = true; startX = x; startY = y; }
        else isDragging = false;
    }
    if (button == 3) { camDist -= 2.5f; if (camDist < 25.0f) camDist = 25.0f; }
    else if (button == 4) { camDist += 2.5f; if (camDist > 220.0f) camDist = 220.0f; }
    glutPostRedisplay();
}

void mouseMotion(int x, int y)
{
    if (isDragging)
    {
        camAngle += (x - startX) * 0.25f;
        camHeight += (startY - y) * 0.20f;
        if (camHeight < 4.0f) camHeight = 4.0f;
        if (camHeight > 90.0f) camHeight = 90.0f;
        startX = x; startY = y;
        glutPostRedisplay();
    }
}

void keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
    case '+': case '=':
        camDist -= 3.0f; if (camDist < 25.0f) camDist = 25.0f; break;
    case '-':
        camDist += 3.0f; if (camDist > 220.0f) camDist = 220.0f; break;
    case 'l': case 'L':
        if (!landingMode)
            {
                PlaySound((LPCSTR)"C:\\Users\\ASUS\\Desktop\\Computer Graphics Lab\\lab\\landing.wav", NULL, SND_FILENAME | SND_ASYNC);
                planeX = -90.0f; planeY = 45.0f;
                planeZ = 5.0f + (planeX * tan(12.0f * PI / 180.0f));
                landingMode = true; takeoffMode = false; landed = false;
                for(int i=0; i<NUM_PASSENGERS; i++) {
                    passengers[i].x = 2.8f; passengers[i].y = 0.8f; passengers[i].z = 24.0f; // Adjusted inside
                    passengers[i].state = STATE_INSIDE_PLANE;
                }
            }
        break;
    case 't': case 'T':
        if (landed && !takeoffMode) 
        { 
            bool passengersOutside = false;
            for(int i=0; i<NUM_PASSENGERS; i++) {
                if(passengers[i].state == STATE_WAITING_OUTSIDE) passengersOutside = true;
            }

            if(passengersOutside) {
                doorOpen = true;
                boardingMode = true;
            } else {
                PlaySound((LPCSTR)"C:\\Users\\ASUS\\Desktop\\Computer Graphics Lab\\lab\\takeoff.wav", NULL, SND_FILENAME | SND_ASYNC);
                takeoffMode = true; landingMode = false; 
            }
        } 
        break;
    case 'w': case 'W':
        flapAngle += 5.0f; if (flapAngle > 35.0f) flapAngle = 35.0f; break;
    case 's': case 'S':
        flapAngle -= 5.0f; if (flapAngle < -35.0f) flapAngle = -35.0f; break;
    case 'f': case 'F':
        fanSpeed += 5.0f; if (fanSpeed > 85.0f) fanSpeed = 85.0f; break;
    case 'g': case 'G':
        fanSpeed -= 5.0f; if (fanSpeed < 0.0f) fanSpeed = 0.0f; break;
    
    // DOOR OPEN (O Key)
    case 'o': case 'O':
        if (landed && !takeoffMode && fanSpeed == 0.0f) doorOpen = true; 
        break;
        
    // DOOR CLOSE (C Key)
    case 'c': case 'C':
        doorOpen = false; 
        startManWalking = false; 
        break;
    case 'r': case 'R':
        isDaytime = !isDaytime; break;
        
    // MAN WALKING TRIGGER (M Key)
    case 'm': case 'M':
        if (doorOpen && doorAngle >= 85.0f) 
        {
            startManWalking = true; 
        }
        break;
    case 27: // ESC key
        exit(0);
    }
    glutPostRedisplay();
}

void specialKeys(int key, int x, int y)
{
    if (key == GLUT_KEY_LEFT) camAngle -= 2.5f;
    if (key == GLUT_KEY_RIGHT) camAngle += 2.5f;
    if (key == GLUT_KEY_UP) camHeight += 1.2f;
    if (key == GLUT_KEY_DOWN) camHeight -= 1.2f;
    glutPostRedisplay();
}

void reshape(int w, int h)
{
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(46.0, (float)w / (float)h, 1.0, 500.0);
    glMatrixMode(GL_MODELVIEW);
}

void init()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat lightPos[] = {40.0f, 85.0f, 50.0f, 1.0f};
    GLfloat lightAmb[] = {0.55f, 0.55f, 0.58f, 1.0f};
    GLfloat lightDiff[] = {1.0f, 1.0f, 0.95f, 1.0f};

    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiff);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glClearColor(0.60f, 0.75f, 0.95f, 1.0f);

    // Initialize Passengers data (FIXED ALIGNMENT TO DOOR ENTRY)
    for (int i = 0; i < NUM_PASSENGERS; i++)
    {
        // Placed exactly inside the doorway boundary 
        passengers[i].x = 2.8f;
        passengers[i].y = 0.8f;
        passengers[i].z = 24.0f;
        
        // FIX: Target standing spots are now scattered randomly in front of
        // the terminal building instead of one straight evenly-spaced line,
        // so passengers fan out like a real crowd once they finish
        // disembarking. targetX still keeps them clear of the stairs/plane,
        // targetZ is randomized within a wide range in front of the terminal.
        passengers[i].targetX = 11.0f + (float)(rand() % 1000) / 1000.0f * 9.0f;   // ~11.0 to 20.0
        passengers[i].targetY = -4.5f;
        passengers[i].targetZ = 14.0f + (float)(rand() % 1000) / 1000.0f * 24.0f;  // spread along the building front
        
        // Random cloth colors
        passengers[i].r = (float)(rand() % 100) / 100.0f;
        passengers[i].g = (float)(rand() % 100) / 100.0f;
        passengers[i].b = (float)(rand() % 100) / 100.0f;
        
        passengers[i].state = STATE_INSIDE_PLANE;
    }
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1200, 800);
    glutCreateWindow("3D Airport Simulation - Fixed Multi-Passenger System");

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutSpecialFunc(specialKeys);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMotion);

    glutTimerFunc(16, update, 0);
    glutMainLoop();
    return 0;
}