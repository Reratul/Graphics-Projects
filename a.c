#include <GL/gl.h>
#include <stdio.h>
#include <math.h>
#include <GL/glut.h>
#include <windows.h>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

float bx = 60;
float by = -10;
float ax = 0;
float waveTime = 0.0;

float doorY = 195;
bool soundPlayed = false;
bool isMoving = true;

float moodFactor = 1.0;
DWORD stopTime = 0;
bool reachedDock = false;
bool waterSoundStarted = false;

float sunR = 255, sunG = 215, sunB = 0;
float moonR = 240, moonG = 240, moonB = 245;

void drawCircle(GLfloat rx, GLfloat ry, GLfloat cx, GLfloat cy)
{
    glBegin(GL_POLYGON);
    glVertex2f(cx, cy);
    for (int i = 0; i <= 360; i++)
    {
        float angle = i * 3.1416 / 180;
        float x = rx * cos(angle);
        float y = ry * sin(angle);
        glVertex2f((x + cx), (y + cy));
    }
    glEnd();
}

void init(void)
{
    glClearColor(0.0, 0.9, 0.9, 0.0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, 500, 0, 500);
}

void clouds()
{
    glPushMatrix();
    glTranslatef(ax, 0, 0);

    glColor3ub(255 * moodFactor, 255 * moodFactor, 255 * moodFactor);
    drawCircle(20, 30, 460, 460);
    drawCircle(15, 20, 445, 460);
    drawCircle(15, 20, 475, 460);

    drawCircle(20, 30, 390, 420);
    drawCircle(15, 20, 405, 420);
    drawCircle(15, 20, 375, 420);
    glPopMatrix();

    if (isMoving) {
        ax += 0.05;
        if (ax > 100)
            ax = -400;
    }
}

void boat() {
    glPushMatrix();
    glTranslatef(bx, by, 0);

    glColor3ub(0, 0, 0);
    glBegin(GL_POLYGON);
    glVertex2d(325, 220);
    glVertex2d(400, 220);
    glVertex2d(425, 250);
    glVertex2d(300, 250);
    glEnd();

    glColor3ub(205 * moodFactor, 133 * moodFactor, 63 * moodFactor);
    glBegin(GL_POLYGON);
    glVertex2d(325, 250);
    glVertex2d(400, 250);
    glVertex2d(390, 280);
    glVertex2d(335, 280);
    glEnd();

    glColor3ub(160 * moodFactor, 82 * moodFactor, 45 * moodFactor);
    glBegin(GL_POLYGON);
    glVertex2d(360, 280);
    glVertex2d(370, 280);
    glVertex2d(370, 310);
    glVertex2d(360, 310);
    glEnd();

    glColor3ub(128 * moodFactor, 0, 128 * moodFactor);
    glBegin(GL_POLYGON);
    glVertex2d(335, 290);
    glVertex2d(400, 290);
    glVertex2d(400, 375);
    glVertex2d(335, 375);
    glEnd();

    glPopMatrix();

    if (isMoving && bx > -215) {
        bx -= 0.06;
        by += 0.0075;
    }

    if (bx <= -215) {
        if (!reachedDock) {
            stopTime = GetTickCount();
            reachedDock = true;
        }

        if (!soundPlayed) {
            PlaySound(TEXT("D:\\LabTest\\grapicslab\\horn.wav"), NULL, SND_FILENAME | SND_ASYNC);
            soundPlayed = true;
        }

        if (soundPlayed && doorY > 150) {
            doorY -= 0.15;
        }

        if (reachedDock && (GetTickCount() - stopTime >= 4000)) {
            if (moodFactor > 0.1) {
                moodFactor -= 0.0006;
            }

            if (!waterSoundStarted) {
                PlaySound(TEXT("D:\\LabTest\\grapicslab\\wave.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
                waterSoundStarted = true;
            }
        }
    }
}

void drawRiverAndWaves() {
    glColor3ub(100 * moodFactor, 149 * moodFactor, 237 * moodFactor);
    glBegin(GL_POLYGON);
    glVertex2d(300, 300);
    glVertex2d(250, 150);
    glVertex2d(400, 150);
    glVertex2d(450, 300);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2d(300, 150);
    glVertex2d(250, 0);
    glVertex2d(400, 0);
    glVertex2d(450, 150);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2d(-40, 200);
    glVertex2d(0, 300);
    glVertex2d(500, 300);
    glVertex2d(500, 200);
    glEnd();

    glColor3ub(220 * moodFactor, 235 * moodFactor, 255 * moodFactor);
    glLineWidth(2.0);

    glBegin(GL_LINE_STRIP);
    for (int x = 0; x <= 500; x += 10) {
        float y = 250 + 3 * sin(x * 0.05 + waveTime);
        glVertex2f(x, y);
    }
    glEnd();
    glBegin(GL_LINE_STRIP);
    for (int x = 270; x <= 420; x += 10) {
        float y = 150 + 2 * sin(x * 0.08 + waveTime);
        glVertex2f(x, y);
    }
    glEnd();

    glBegin(GL_LINE_STRIP);
    for (int x = 270; x <= 380; x += 10) {
        float y = 50 + 2 * sin(x * 0.1 + waveTime);
        glVertex2f(x, y);
    }
    glEnd();

    if (isMoving) {
        waveTime += 0.01;
    }
}

void keyboard(unsigned char key, int x, int y)
{
    switch (key) {
        case 's':
        case 'S':
            isMoving = true;
            break;
        case 'p':
        case 'P':
            isMoving = false;
            break;
        case 'r':
        case 'R':
            bx = 60;
            by = -10;
            doorY = 195;
            moodFactor = 1.0;
            soundPlayed = false;
            reachedDock = false;
            isMoving = true;

            PlaySound(NULL, 0, 0);
            waterSoundStarted = false;
            break;
    }
    glutPostRedisplay();
}

void display(void)
{
    float bgR = 0.0;
    float bgG = 0.9 * moodFactor;
    float bgB = 0.9 * (moodFactor < 0.2 ? 0.2 : moodFactor);

    glClearColor(bgR, bgG, bgB, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);

    float lightFactor = 1.0 - moodFactor;
    float lightR = 160 * moodFactor + 255 * lightFactor;
    float lightG = 82 * moodFactor + 215 * lightFactor;
    float lightB = 45 * moodFactor + 0 * lightFactor;

    glColor3ub(0, 255 * moodFactor, 0);
    glBegin(GL_POLYGON);
    glVertex2d(0, 0);
    glVertex2d(500, 0);
    glVertex2d(500, 300);
    glVertex2d(0, 300);
    glEnd();

    drawRiverAndWaves();

    glColor3ub(184 * moodFactor, 134 * moodFactor, 11 * moodFactor);
    glBegin(GL_POLYGON);
    glVertex2d(-40, 300);
    glVertex2d(200, 300);
    glVertex2d(100, 450);
    glEnd();

    glColor3ub(218 * moodFactor, 165 * moodFactor, 32 * moodFactor);
    glBegin(GL_POLYGON);
    glVertex2d(150, 300);
    glVertex2d(350, 300);
    glVertex2d(250, 450);
    glEnd();

    glColor3ub(184 * moodFactor, 134 * moodFactor, 11 * moodFactor);
    glBegin(GL_POLYGON);
    glVertex2d(300, 300);
    glVertex2d(520, 300);
    glVertex2d(400, 450);
    glEnd();

    boat();

    glColor3ub(139 * moodFactor, 69 * moodFactor, 19 * moodFactor);
    glBegin(GL_POLYGON);
    glVertex2d(50, 150);
    glVertex2d(70, 150);
    glVertex2d(70, 300);
    glVertex2d(50, 300);
    glEnd();

    glColor3ub(0, 128 * moodFactor, 0); drawCircle(30, 40, 35, 320);
    glColor3ub(0, 128 * moodFactor, 0); drawCircle(30, 40, 85, 320);
    glColor3ub(0, 128 * moodFactor, 0); drawCircle(25, 30, 45, 370);
    glColor3ub(0, 128 * moodFactor, 0); drawCircle(30, 30, 70, 370);
    glColor3ub(0, 128 * moodFactor, 0); drawCircle(25, 30, 55, 400);

    glColor3ub(210 * moodFactor, 105 * moodFactor, 30 * moodFactor);
    glBegin(GL_POLYGON);
    glVertex2d(100, 220);
    glVertex2d(200, 220);
    glVertex2d(175, 270);
    glVertex2d(130, 270);
    glEnd();

    glColor3ub(244 * moodFactor, 164 * moodFactor, 96 * moodFactor);
    glBegin(GL_POLYGON);
    glVertex2d(100, 170);
    glVertex2d(185, 170);
    glVertex2d(185, 220);
    glVertex2d(100, 220);
    glEnd();

    glColor3ub(160 * moodFactor, 82 * moodFactor, 45 * moodFactor);
    glBegin(GL_POLYGON);
    glVertex2d(100, 170);
    glVertex2d(190, 170);
    glVertex2d(190, 160);
    glVertex2d(100, 160);
    glEnd();

    glColor3ub(lightR, lightG, lightB);
    glBegin(GL_POLYGON);
    glVertex2d(140, 170);
    glVertex2d(165, 170);
    glVertex2d(165, 200);
    glVertex2d(140, 200);
    glEnd();

    glColor3ub(160 * moodFactor, 82 * moodFactor, 45 * moodFactor);
    glBegin(GL_POLYGON);
    glVertex2d(0, 220);
    glVertex2d(135, 220);
    glVertex2d(110, 270);
    glVertex2d(25, 270);
    glEnd();

    glColor3ub(255 * moodFactor, 222 * moodFactor, 173 * moodFactor);
    glBegin(GL_POLYGON);
    glVertex2d(10, 220);
    glVertex2d(50, 220);
    glVertex2d(25, 255);
    glEnd();

    glColor3ub(255 * moodFactor, 222 * moodFactor, 173 * moodFactor);
    glBegin(GL_POLYGON);
    glVertex2d(10, 150);
    glVertex2d(50, 150);
    glVertex2d(50, 220);
    glVertex2d(10, 220);
    glEnd();

    glColor3ub(222 * moodFactor, 184 * moodFactor, 135 * moodFactor);
    glBegin(GL_POLYGON);
    glVertex2d(50, 150);
    glVertex2d(125, 150);
    glVertex2d(125, 220);
    glVertex2d(50, 220);
    glEnd();

    glColor3ub(160 * moodFactor, 82 * moodFactor, 45 * moodFactor);
    glBegin(GL_POLYGON);
    glVertex2d(10, 150);
    glVertex2d(125, 150);
    glVertex2d(125, 140);
    glVertex2d(10, 140);
    glEnd();

    glColor3ub(255 * lightFactor, 215 * lightFactor, 0);
    glBegin(GL_POLYGON);
    glVertex2d(75, 150);
    glVertex2d(95, 150);
    glVertex2d(95, 195);
    glVertex2d(75, 195);
    glEnd();

    glColor3ub(160 * moodFactor, 82 * moodFactor, 45 * moodFactor);
    glBegin(GL_POLYGON);
    glVertex2d(75, 150);
    glVertex2d(95, 150);
    glVertex2d(95, doorY);
    glVertex2d(75, doorY);
    glEnd();

    glColor3ub(lightR, lightG, lightB);
    glBegin(GL_POLYGON);
    glVertex2d(20, 200);
    glVertex2d(35, 200);
    glVertex2d(35, 175);
    glVertex2d(20, 175);
    glEnd();

    float currentR = sunR * moodFactor + moonR * (1.0 - moodFactor);
    float currentG = sunG * moodFactor + moonG * (1.0 - moodFactor);
    float currentB = sunB * moodFactor + moonB * (1.0 - moodFactor);

    glColor3ub(currentR, currentG, currentB);
    drawCircle(25, 30, 175, 450);

    clouds();

    glutPostRedisplay();
    glutSwapBuffers();
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(900, 500);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Village Scenery - Assignment Final");
    init();
    glutDisplayFunc(display);

    glutKeyboardFunc(keyboard);

    glutMainLoop();
    return 0;
}


