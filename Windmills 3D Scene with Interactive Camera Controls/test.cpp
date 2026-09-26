#include <GL/glut.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

float angle1 = 0.0; // Left windmill angle (always rotating)
float angle2 = 0.0; // Right windmill angle (controlled by user)
float speed2 = 0.0; // Right windmill speed/direction

float cloudX = 0.0;
float cloudY = 1.4;
float cloudZ = -2.0;
float cloudSize = 1.0; // Movable cloud size (+ / - keys control kore)

// Camera control variables (starting at a level, front-facing view)
float cameraAngleX = 0.0;
float cameraAngleY = 0.0;
float cameraDist = 6.2;

float sunAngle = 25.0f; // For subtle warm lighting feel

void drawCircle3D(float r, int segs)
{
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0, 0, 0);
    for (int i = 0; i <= segs; i++)
    {
        float theta = 2.0f * M_PI * (float)i / (float)segs;
        glVertex3f(r * cosf(theta), r * sinf(theta), 0);
    }
    glEnd();
}

void drawCylinder(float baseR, float topR, float height, int segs)
{
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segs; i++)
    {
        float theta = 2.0f * M_PI * (float)i / (float)segs;
        float cx = cosf(theta), sy = sinf(theta);
        glNormal3f(cx, sy, 0.0f);
        glVertex3f(baseR * cx, 0.0f, baseR * sy);
        glVertex3f(topR * cx, height, topR * sy);
    }
    glEnd();

    glPushMatrix();
    glRotatef(90, 1, 0, 0);
    drawCircle3D(baseR, segs);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0, height, 0);
    glRotatef(-90, 1, 0, 0);
    drawCircle3D(topR, segs);
    glPopMatrix();
}

void drawSphere(float r, int slices, int stacks)
{
    GLUquadric *q = gluNewQuadric();
    gluSphere(q, r, slices, stacks);
    gluDeleteQuadric(q);
}

void drawBox(float w, float h, float d)
{
    float x = w / 2, y = h / 2, z = d / 2;
    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);
    glVertex3f(-x, -y, z);
    glVertex3f(x, -y, z);
    glVertex3f(x, y, z);
    glVertex3f(-x, y, z);
    glNormal3f(0, 0, -1);
    glVertex3f(-x, -y, -z);
    glVertex3f(-x, y, -z);
    glVertex3f(x, y, -z);
    glVertex3f(x, -y, -z);
    glNormal3f(-1, 0, 0);
    glVertex3f(-x, -y, -z);
    glVertex3f(-x, -y, z);
    glVertex3f(-x, y, z);
    glVertex3f(-x, y, -z);
    glNormal3f(1, 0, 0);
    glVertex3f(x, -y, -z);
    glVertex3f(x, y, -z);
    glVertex3f(x, y, z);
    glVertex3f(x, -y, z);
    glNormal3f(0, 1, 0);
    glVertex3f(-x, y, -z);
    glVertex3f(-x, y, z);
    glVertex3f(x, y, z);
    glVertex3f(x, y, -z);
    glNormal3f(0, -1, 0);
    glVertex3f(-x, -y, -z);
    glVertex3f(x, -y, -z);
    glVertex3f(x, -y, z);
    glVertex3f(-x, -y, z);
    glEnd();
}

void drawBladeBox(float scale)
{
    float w = 0.5f * scale;
    float h = 0.16f * scale;
    float t = 0.03f * scale;

    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glVertex3f(0.05f * scale, t, -h / 2);
    glVertex3f(0.05f * scale + w, t, -h / 2);
    glVertex3f(0.05f * scale + w, t, h / 2);
    glVertex3f(0.05f * scale, t, h / 2);

    glNormal3f(0, -1, 0);
    glVertex3f(0.05f * scale, -t, -h / 2);
    glVertex3f(0.05f * scale, -t, h / 2);
    glVertex3f(0.05f * scale + w, -t, h / 2);
    glVertex3f(0.05f * scale + w, -t, -h / 2);

    glNormal3f(0, 0, 1);
    glVertex3f(0.05f * scale, -t, h / 2);
    glVertex3f(0.05f * scale + w, -t, h / 2);
    glVertex3f(0.05f * scale + w, t, h / 2);
    glVertex3f(0.05f * scale, t, h / 2);

    glNormal3f(0, 0, -1);
    glVertex3f(0.05f * scale, -t, -h / 2);
    glVertex3f(0.05f * scale, t, -h / 2);
    glVertex3f(0.05f * scale + w, t, -h / 2);
    glVertex3f(0.05f * scale + w, -t, -h / 2);

    glNormal3f(-1, 0, 0);
    glVertex3f(0.05f * scale, -t, -h / 2);
    glVertex3f(0.05f * scale, -t, h / 2);
    glVertex3f(0.05f * scale, t, h / 2);
    glVertex3f(0.05f * scale, t, -h / 2);

    glNormal3f(1, 0, 0);
    glVertex3f(0.05f * scale + w, -t, -h / 2);
    glVertex3f(0.05f * scale + w, t, -h / 2);
    glVertex3f(0.05f * scale + w, t, h / 2);
    glVertex3f(0.05f * scale + w, -t, h / 2);
    glEnd();
}

void drawBlade(float scale, int isControlled)
{
    glColor3f(0.25f, 0.55f, 0.85f);
    glPushMatrix();
    glRotatef(90, 0, 0, 1);
    drawCylinder(0.015f * scale, 0.015f * scale, 0.55f * scale, 14);
    glPopMatrix();

    glColor3f(0.93f, 0.95f, 0.98f);
    drawBladeBox(scale);

    if (isControlled)
    {
        if (speed2 < 0)
            glColor3f(0.95f, 0.25f, 0.2f);
        else if (speed2 > 0)
            glColor3f(0.25f, 0.85f, 0.35f);
        else
            glColor3f(1.0f, 1.0f, 1.0f);
    }
    else
    {
        glColor3f(0.95f, 0.25f, 0.2f);
    }

    glPushMatrix();
    glTranslatef(0.6f * scale, 0.0f, 0.0f);
    drawSphere(0.045f * scale, 16, 16);
    glPopMatrix();
}

void drawBlades(float angle, float scale, int isControlled)
{
    for (int i = 0; i < 4; i++)
    {
        glPushMatrix();
        glRotatef(angle + i * 90.0f, 0, 0, 1);
        drawBlade(scale, isControlled);
        glPopMatrix();
    }

    glColor3f(1.0f, 0.85f, 0.1f);
    drawSphere(0.07f * scale, 20, 20);
}

void drawNacelle(float scale)
{
    glColor3f(0.9f, 0.9f, 0.92f);
    glPushMatrix();
    glRotatef(90, 0, 1, 0);
    glTranslatef(0, 0, -0.12f * scale);
    drawCylinder(0.09f * scale, 0.09f * scale, 0.24f * scale, 18);
    glPopMatrix();
}

void drawTower(float scale)
{
    glColor3f(0.92f, 0.92f, 0.94f);
    drawCylinder(0.10f * scale, 0.04f * scale, 1.6f * scale, 24);
}

void drawWindmill(float x, float z, float angle, float scale, int isControlled)
{
    glPushMatrix();
    glTranslatef(x, -0.85f, z);

    drawTower(scale);

    glTranslatef(0.0f, 1.6f * scale, 0.0f);

    drawNacelle(scale);

    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 0.14f * scale);
    drawBlades(angle, scale, isControlled);
    glPopMatrix();

    glPopMatrix();
}

void drawHillPatch(float cx, float cz, float radius, float height, float r, float g, float b, int segs)
{
    glColor3f(r, g, b);
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0, 1, 0);
    glVertex3f(cx, -0.75f, cz);
    for (int i = 0; i <= segs; i++)
    {
        float theta = 2.0f * M_PI * (float)i / (float)segs;
        float x = cx + radius * cosf(theta);
        float z = cz + radius * sinf(theta);
        glVertex3f(x, -0.75f, z);
    }
    glEnd();

    int rings = 7;
    for (int rI = 0; rI < rings; rI++)
    {
        float r0 = radius * (1.0f - (float)rI / rings);
        float r1 = radius * (1.0f - (float)(rI + 1) / rings);
        float y0 = -0.75f + height * ((float)rI / rings);
        float y1 = -0.75f + height * ((float)(rI + 1) / rings);
        glBegin(GL_QUAD_STRIP);
        for (int i = 0; i <= segs; i++)
        {
            float theta = 2.0f * M_PI * (float)i / (float)segs;
            float ct = cosf(theta), st = sinf(theta);
            glNormal3f(ct, 0.55f, st);
            glVertex3f(cx + r0 * ct, y0, cz + r0 * st);
            glVertex3f(cx + r1 * ct, y1, cz + r1 * st);
        }
        glEnd();
    }
}

void drawHills()
{
    drawHillPatch(-1.2f, -1.5f, 1.0f, 0.45f, 0.33f, 0.62f, 0.18f, 28);
    drawHillPatch(0.0f, -2.0f, 1.4f, 0.6f, 0.18f, 0.42f, 0.1f, 30);
    drawHillPatch(1.6f, -1.6f, 1.1f, 0.5f, 0.28f, 0.57f, 0.15f, 28);
    drawHillPatch(-2.4f, -1.8f, 0.9f, 0.35f, 0.32f, 0.62f, 0.17f, 22);
    drawHillPatch(2.8f, -2.0f, 0.85f, 0.32f, 0.22f, 0.47f, 0.12f, 22);
    drawHillPatch(-3.8f, -2.2f, 1.0f, 0.4f, 0.35f, 0.65f, 0.2f, 24);
    drawHillPatch(4.0f, -2.4f, 1.0f, 0.4f, 0.2f, 0.43f, 0.11f, 24);
}

void drawCloudPuff(float cx, float cy, float cz, float size, float shade)
{
    glColor3f(shade, shade, shade * 1.02f);
    glPushMatrix();
    glTranslatef(cx, cy, cz);
    drawSphere(size, 16, 16);
    glPopMatrix();
}

void drawCloud(float cx, float cy, float cz, float size)
{
    drawCloudPuff(cx, cy, cz, size * 0.32f, 1.0f);
    drawCloudPuff(cx - size * 0.3f, cy - size * 0.05f, cz, size * 0.26f, 1.0f);
    drawCloudPuff(cx + size * 0.3f, cy - size * 0.05f, cz, size * 0.26f, 1.0f);
    drawCloudPuff(cx - size * 0.15f, cy + size * 0.18f, cz, size * 0.24f, 1.0f);
    drawCloudPuff(cx + size * 0.15f, cy + size * 0.18f, cz, size * 0.24f, 1.0f);
    drawCloudPuff(cx, cy + size * 0.22f, cz, size * 0.30f, 1.0f);
    drawCloudPuff(cx - size * 0.1f, cy - size * 0.18f, cz, size * 0.16f, 0.92f);
    drawCloudPuff(cx + size * 0.1f, cy - size * 0.18f, cz, size * 0.16f, 0.92f);
}

void drawGround()
{
    // Slightly varied green patches for a less flat, more natural ground
    glColor3f(0.33f, 0.68f, 0.22f);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glVertex3f(-6.0f, -0.85f, -6.0f);
    glVertex3f(-6.0f, -0.85f, 6.0f);
    glVertex3f(6.0f, -0.85f, 6.0f);
    glVertex3f(6.0f, -0.85f, -6.0f);
    glEnd();

    // A subtly darker foreground strip for depth
    glColor3f(0.28f, 0.6f, 0.18f);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glVertex3f(-6.0f, -0.849f, 1.8f);
    glVertex3f(-6.0f, -0.849f, 6.0f);
    glVertex3f(6.0f, -0.849f, 6.0f);
    glVertex3f(6.0f, -0.849f, 1.8f);
    glEnd();
}

// ---------- HOUSE ----------
void drawHouse(float x, float z, float scale)
{
    glPushMatrix();
    glTranslatef(x, -0.85f, z);

    glColor3f(0.88f, 0.78f, 0.6f);
    glPushMatrix();
    glTranslatef(0.0f, 0.18f * scale, 0.0f);
    drawBox(0.5f * scale, 0.36f * scale, 0.45f * scale);
    glPopMatrix();

    glColor3f(0.62f, 0.2f, 0.14f);
    float rw = 0.30f * scale, rd = 0.27f * scale, rh = 0.28f * scale, ry = 0.36f * scale;
    glBegin(GL_TRIANGLES);
    glNormal3f(0, 0.5f, 1);
    glVertex3f(-rw, ry, rd);
    glVertex3f(rw, ry, rd);
    glVertex3f(0, ry + rh, 0);
    glNormal3f(0, 0.5f, -1);
    glVertex3f(rw, ry, -rd);
    glVertex3f(-rw, ry, -rd);
    glVertex3f(0, ry + rh, 0);
    glNormal3f(-1, 0.5f, 0);
    glVertex3f(-rw, ry, -rd);
    glVertex3f(-rw, ry, rd);
    glVertex3f(0, ry + rh, 0);
    glNormal3f(1, 0.5f, 0);
    glVertex3f(rw, ry, rd);
    glVertex3f(rw, ry, -rd);
    glVertex3f(0, ry + rh, 0);
    glEnd();

    glColor3f(0.38f, 0.22f, 0.1f);
    glPushMatrix();
    glTranslatef(0.0f, 0.08f * scale, 0.226f * scale);
    drawBox(0.1f * scale, 0.2f * scale, 0.01f * scale);
    glPopMatrix();

    glColor3f(0.65f, 0.88f, 0.96f);
    glPushMatrix();
    glTranslatef(-0.16f * scale, 0.22f * scale, 0.226f * scale);
    drawBox(0.08f * scale, 0.08f * scale, 0.01f * scale);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0.16f * scale, 0.22f * scale, 0.226f * scale);
    drawBox(0.08f * scale, 0.08f * scale, 0.01f * scale);
    glPopMatrix();

    glColor3f(0.55f, 0.55f, 0.57f);
    glPushMatrix();
    glTranslatef(0.13f * scale, 0.5f * scale, -0.05f * scale);
    drawBox(0.05f * scale, 0.2f * scale, 0.05f * scale);
    glPopMatrix();

    glPopMatrix();
}

//  BUSH 
void drawBush(float x, float z, float size)
{
    glPushMatrix();
    glTranslatef(x, -0.85f, z);
    glColor3f(0.18f, 0.48f, 0.12f);
    drawSphere(size * 0.22f, 14, 14);
    glColor3f(0.22f, 0.53f, 0.15f);
    glPushMatrix();
    glTranslatef(size * 0.18f, size * 0.04f, 0.0f);
    drawSphere(size * 0.17f, 14, 14);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-size * 0.18f, size * 0.02f, size * 0.05f);
    drawSphere(size * 0.16f, 14, 14);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0.0f, size * 0.08f, -size * 0.15f);
    drawSphere(size * 0.15f, 14, 14);
    glPopMatrix();
    glPopMatrix();
}

//  ROCK 
void drawRock(float x, float z, float size, float rotY)
{
    glPushMatrix();
    glTranslatef(x, -0.85f + size * 0.12f, z);
    glRotatef(rotY, 0, 1, 0);
    glColor3f(0.52f, 0.52f, 0.54f);
    glScalef(1.0f, 0.6f, 0.8f);
    drawSphere(size * 0.18f, 12, 10);
    glColor3f(0.43f, 0.43f, 0.46f);
    glPushMatrix();
    glTranslatef(size * 0.12f, size * 0.03f, size * 0.05f);
    drawSphere(size * 0.1f, 10, 8);
    glPopMatrix();
    glPopMatrix();
}

void drawForegroundDecor()
{
    drawBush(-2.6f, 1.6f, 1.0f);
    drawBush(-2.0f, 2.0f, 0.8f);
    drawBush(2.4f, 1.7f, 1.1f);
    drawBush(1.7f, 2.1f, 0.7f);
    drawBush(-0.3f, 2.3f, 0.6f);
    drawBush(3.4f, 2.3f, 0.9f);
    drawBush(-3.4f, 2.1f, 0.85f);

    drawRock(-1.6f, 1.8f, 1.0f, 20.0f);
    drawRock(-1.2f, 2.1f, 0.7f, 60.0f);
    drawRock(1.0f, 1.9f, 0.9f, 100.0f);
    drawRock(1.5f, 2.3f, 0.6f, 10.0f);
    drawRock(0.2f, 2.5f, 0.5f, 150.0f);
    drawRock(-2.8f, 2.4f, 0.6f, 40.0f);
    drawRock(2.9f, 2.0f, 0.55f, 80.0f);
}

void drawSky()
{
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glBegin(GL_QUADS);
    glColor3f(0.45f, 0.7f, 0.92f);
    glVertex3f(-10, -10, -9.99f);
    glVertex3f(10, -10, -9.99f);
    glColor3f(0.7f, 0.87f, 0.97f);
    glVertex3f(10, 10, -9.99f);
    glVertex3f(-10, 10, -9.99f);
    glEnd();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

void display()
{
    glClearColor(0.55f, 0.78f, 0.93f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    gluLookAt(0, 0.3, cameraDist, 0, 0.0, 0, 0, 1, 0);
    glRotatef(cameraAngleX, 1, 0, 0);
    glRotatef(cameraAngleY, 0, 1, 0);

    GLfloat lightPos[] = {2.2f, 4.0f, 3.5f, 1.0f};
    GLfloat lightAmbient[] = {0.4f, 0.4f, 0.42f, 1.0f};
    GLfloat lightDiffuse[] = {1.0f, 0.97f, 0.9f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);

    drawHills();
    drawGround();

    drawCloud(-2.5f, 2.4f, -3.0f, 0.9f);
    drawCloud(2.8f, 2.6f, -3.5f, 1.1f);
    drawCloud(0.0f, 3.0f, -4.0f, 0.8f);
    drawCloud(-4.0f, 2.0f, -4.5f, 0.7f);
    drawCloud(cloudX, cloudY, cloudZ, cloudSize);

    float scale = 0.5f;
    drawWindmill(-1.2f, 0.0f, angle1, scale, 0);
    drawWindmill(1.2f, 0.0f, angle2, scale, 1);

    drawHouse(0.0f, -0.3f, 0.9f);

    drawForegroundDecor();

    glutSwapBuffers();
}

void update(int value)
{
    angle1 -= 2.0f;
    if (angle1 > 360.0f)
        angle1 -= 360.0f;
    else if (angle1 < -360.0f)
        angle1 += 360.0f;

    angle2 += speed2;
    if (angle2 > 360.0f)
        angle2 -= 360.0f;
    else if (angle2 < -360.0f)
        angle2 += 360.0f;

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

void keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
    case 'l':
    case 'L':
        speed2 = 2.0f;
        break;
    case 'r':
    case 'R':
        speed2 = -2.0f;
        break;
    case 's':
    case 'S':
        speed2 = 0.0f;
        break;
    case '1':
        cameraAngleY -= 5.0f;
        break;
    case '2':
        cameraAngleY += 5.0f;
        break;
    case '3':
        cameraAngleX -= 5.0f;
        break;
    case '4':
        cameraAngleX += 5.0f;
        break;
    case '5':
        cameraDist -= 0.3f;
        if (cameraDist < 1.5f)
            cameraDist = 1.5f;
        break;
    case '6':
        cameraDist += 0.3f;
        if (cameraDist > 15.0f)
            cameraDist = 15.0f;
        break;
    case '0':
        cameraAngleX = 0.0f;
        cameraAngleY = 0.0f;
        cameraDist = 6.2f;
        break;
    case '+': // Movable cloud ke boro koro
    case '=': // Same key without shift on most keyboards
        cloudSize += 0.05f;
        if (cloudSize > 3.0f)
            cloudSize = 3.0f;
        break;
    case '-': // Movable cloud ke choto koro
    case '_':
        cloudSize -= 0.05f;
        if (cloudSize < 0.1f)
            cloudSize = 0.1f;
        break;
    case 27:
        exit(0);
        break;
    }
}

void specialKeyboard(int key, int x, int y)
{
    float moveAmount = 0.1f;
    switch (key)
    {
    case GLUT_KEY_UP:
        cloudY += moveAmount;
        break;
    case GLUT_KEY_DOWN:
        cloudY -= moveAmount;
        break;
    case GLUT_KEY_LEFT:
        cloudX -= moveAmount;
        break;
    case GLUT_KEY_RIGHT:
        cloudX += moveAmount;
        break;
    }
    glutPostRedisplay();
}

void reshape(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(50.0, (double)w / (double)(h == 0 ? 1 : h), 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void initGL()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    GLfloat lightAmbient[] = {0.4f, 0.4f, 0.42f, 1.0f};
    GLfloat lightDiffuse[] = {1.0f, 0.97f, 0.9f, 1.0f};
    GLfloat lightSpecular[] = {0.3f, 0.3f, 0.3f, 1.0f};
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);

    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);

    glEnable(GL_FOG);
    GLfloat fogColor[] = {0.55f, 0.78f, 0.93f, 1.0f};
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogf(GL_FOG_START, 4.0f);
    glFogf(GL_FOG_END, 12.0f);
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1200, 1000);
    glutCreateWindow("Two Windmills 3D with Camera Controls - OpenGL");

    initGL();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeyboard);
    glutTimerFunc(0, update, 0);

    glutMainLoop();
    return 0;
}