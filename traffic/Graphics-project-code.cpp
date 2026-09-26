#include <GL/glut.h>
#include <iostream>
#include <cmath>

// Camera Rotation Variables (Puro scene 3D move korar jonno)
float cam_angleX = 22.0f;
float cam_angleY = -12.0f;
float cam_distance = 8.5f;

// Car 1 (Left Car - Red Car) Variables
float car1_x = -6.0f, car1_z = 0.5f;
float car1_speed = 0.02f;

// Car Door Variables
bool isCarDoorOpen = false;
float doorAngle = 0.0f;

// --- FIXED: Blue Car Z-axis value strictly aligned inside right road lane ---
float car2_x = 3.5f, car2_y = 0.0f, car2_z = -0.7f; // Perfectly positioned inside the road width
float car2_rotY = 0.0f;

// Bird Animation Variables
float bird_x = 0.0f;
float wingAngle = 0.0f;
bool wingUp = true;

// Environment Variables
bool isStorageDoorOpen = false;
int trafficLightStatus = 0; // 0: Red, 1: Yellow, 2: Green

// Clear & Solid Stop Line Position
float stopLineX = -1.5f;

void init() {
    glClearColor(0.5f, 0.8f, 1.0f, 1.0f); // Sky Blue Background
    glEnable(GL_DEPTH_TEST);              // Enable 3D Depth Test
    glShadeModel(GL_SMOOTH);
}

// Function to draw a 3D Mountain
void drawMountain(float x, float z, float height, float width) {
    glPushMatrix();
    glTranslatef(x, -1.0f, z);
    glColor3f(0.45f, 0.45f, 0.45f); // Grey Mountain
    glBegin(GL_TRIANGLES);
        glVertex3f(0.0f, height, 0.0f);
        glVertex3f(-width, 0.0f, width);
        glVertex3f(width, 0.0f, width);

        glVertex3f(0.0f, height, 0.0f);
        glVertex3f(width, 0.0f, width);
        glVertex3f(width, 0.0f, -width);

        glVertex3f(0.0f, height, 0.0f);
        glVertex3f(width, 0.0f, -width);
        glVertex3f(-width, 0.0f, -width);

        glVertex3f(0.0f, height, 0.0f);
        glVertex3f(-width, 0.0f, -width);
        glVertex3f(-width, 0.0f, width);
    glEnd();
    glPopMatrix();
}

// Function to draw a Cloud
void drawCloud(float x, float y, float z) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glColor3f(0.95f, 0.95f, 0.95f);
    glPushMatrix(); glutSolidSphere(0.35, 16, 16); glPopMatrix();
    glPushMatrix(); glTranslatef(0.25f, 0.0f, 0.0f); glutSolidSphere(0.28, 16, 16); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.25f, 0.0f, 0.0f); glutSolidSphere(0.28, 16, 16); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 0.15f, 0.0f); glutSolidSphere(0.3, 16, 16); glPopMatrix();
    glPopMatrix();
}

// Function to draw a 3D Animated Flying Bird
void drawBird(float x, float y, float z) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glColor3f(0.1f, 0.1f, 0.1f);
    glPushMatrix(); glScalef(0.15f, 0.06f, 0.06f); glutSolidSphere(0.5, 8, 8); glPopMatrix();

    // Left Wing
    glPushMatrix();
    glRotatef(wingAngle, 1.0f, 0.0f, 0.0f);
    glBegin(GL_TRIANGLES);
        glVertex3f(0.0f, 0.0f, 0.0f); glVertex3f(-0.05f, 0.05f, 0.25f); glVertex3f(0.05f, 0.0f, 0.0f);
    glEnd();
    glPopMatrix();

    // Right Wing
    glPushMatrix();
    glRotatef(-wingAngle, 1.0f, 0.0f, 0.0f);
    glBegin(GL_TRIANGLES);
        glVertex3f(0.0f, 0.0f, 0.0f); glVertex3f(-0.05f, 0.05f, -0.25f); glVertex3f(0.05f, 0.0f, 0.0f);
    glEnd();
    glPopMatrix();

    glPopMatrix();
}

// Bot Gach (Banyan Tree Model)
void drawBanyanTree(float x, float z) {
    glPushMatrix();
    glTranslatef(x, -1.0f, z);
    glColor3f(0.35f, 0.18f, 0.05f);
    glPushMatrix(); glScalef(0.7f, 2.2f, 0.7f); glutSolidCube(1.0); glPopMatrix();
    glPushMatrix(); glTranslatef(0.3f, 0.5f, 0.0f); glRotatef(25, 0, 0, 1); glScalef(0.3f, 1.2f, 0.3f); glutSolidCube(1.0); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.3f, 0.5f, 0.0f); glRotatef(-25, 0, 0, 1); glScalef(0.3f, 1.2f, 0.3f); glutSolidCube(1.0); glPopMatrix();

    glColor3f(0.0f, 0.45f, 0.08f);
    glPushMatrix(); glTranslatef(0.0f, 1.6f, 0.0f); glutSolidSphere(1.1, 16, 16); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.7f, 1.5f, 0.0f); glutSolidSphere(0.85, 16, 16); glPopMatrix();
    glPushMatrix(); glTranslatef(-1.3f, 1.3f, 0.1f); glutSolidSphere(0.7, 16, 16); glPopMatrix();
    glPushMatrix(); glTranslatef(0.7f, 1.5f, 0.0f); glutSolidSphere(0.85, 16, 16); glPopMatrix();
    glPushMatrix(); glTranslatef(1.3f, 1.3f, -0.1f); glutSolidSphere(0.7, 16, 16); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.3f, 1.4f, 0.6f); glutSolidSphere(0.75, 16, 16); glPopMatrix();
    glPushMatrix(); glTranslatef(0.4f, 1.5f, -0.6f); glutSolidSphere(0.75, 16, 16); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 2.1f, -0.1f); glutSolidSphere(0.8, 16, 16); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.4f, 1.9f, -0.2f); glutSolidSphere(0.7, 16, 16); glPopMatrix();
    glPushMatrix(); glTranslatef(0.4f, 1.9f, 0.2f); glutSolidSphere(0.7, 16, 16); glPopMatrix();
    glPopMatrix();
}

// Red Car Model with Blue Interior Door Panel
void drawRedCarWithDoor() {
    glPushMatrix();
    glColor3f(0.8f, 0.1f, 0.1f);
    glPushMatrix(); glScalef(1.4f, 0.4f, 0.6f); glutSolidCube(1.0); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.1f, 0.3f, 0.0f); glScalef(0.7f, 0.3f, 0.5f); glutSolidCube(1.0); glPopMatrix();

    glPushMatrix();
    glTranslatef(0.1f, 0.0f, 0.29f); glColor3f(0.1f, 0.2f, 0.8f); glScalef(0.38f, 0.33f, 0.02f); glutSolidCube(1.0);
    glPopMatrix();

    // 3D DOOR ANIMATION
    glPushMatrix();
    glTranslatef(0.1f, 0.0f, 0.31f); glRotatef(-doorAngle, 0.0f, 1.0f, 0.0f);
    if (doorAngle > 5.0f) glColor3f(0.1f, 0.2f, 0.8f); else glColor3f(0.7f, 0.0f, 0.0f);
    glPushMatrix(); glTranslatef(-0.2f, 0.0f, 0.0f); glScalef(0.4f, 0.35f, 0.03f); glutSolidCube(1.0); glPopMatrix();
    glColor3f(1.0f, 1.0f, 1.0f); glPushMatrix(); glTranslatef(-0.35f, 0.0f, 0.02f); glutSolidCube(0.03); glPopMatrix();
    glPopMatrix();

    // Wheels
    glColor3f(0.1f, 0.1f, 0.1f);
    float wheelPositions[4][3] = {{-0.45f, -0.2f, 0.3f}, {0.45f, -0.2f, 0.3f}, {-0.45f, -0.2f, -0.3f}, {-0.45f, -0.2f, -0.3f}};
    for(int i = 0; i < 4; i++) {
        glPushMatrix(); glTranslatef(wheelPositions[i][0], wheelPositions[i][1], wheelPositions[i][2]); glutSolidTorus(0.05, 0.12, 8, 8); glPopMatrix();
    }
    glPopMatrix();
}

// Right Car Model (Blue 3D Controlled Car)
void drawBlueCar() {
    glPushMatrix();
    glColor3f(0.1f, 0.3f, 0.7f);
    glPushMatrix(); glScalef(1.1f, 0.38f, 0.55f); glutSolidCube(1.0); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.1f, 0.28f, 0.0f); glScalef(0.6f, 0.28f, 0.45f); glutSolidCube(1.0); glPopMatrix();
    glColor3f(0.1f, 0.1f, 0.1f);
    float wheelPositions[4][3] = {{-0.38f, -0.18f, 0.28f}, {0.38f, -0.18f, 0.28f}, {-0.38f, -0.18f, -0.28f}, {0.38f, -0.18f, -0.28f}};
    for(int i = 0; i < 4; i++) {
        glPushMatrix(); glTranslatef(wheelPositions[i][0], wheelPositions[i][1], wheelPositions[i][2]); glutSolidTorus(0.04, 0.11, 8, 8); glPopMatrix();
    }
    glPopMatrix();
}

// Traffic Light System
void drawTrafficLight(float x, float z) {
    glPushMatrix();
    glTranslatef(x, -1.0f, z);
    glColor3f(0.2f, 0.2f, 0.2f);
    glPushMatrix(); glScalef(0.1f, 1.8f, 0.1f); glutSolidCube(1.0); glPopMatrix();
    glTranslatef(0.0f, 1.0f, 0.0f);
    glPushMatrix(); glScalef(0.25f, 0.7f, 0.25f); glutSolidCube(1.0); glPopMatrix();

    glColor3f(trafficLightStatus == 0 ? 1.0f : 0.2f, 0.0f, 0.0f); glPushMatrix(); glTranslatef(0.0f, 0.2f, 0.13f); glutSolidSphere(0.07, 8, 8); glPopMatrix();
    glColor3f(trafficLightStatus == 1 ? 1.0f : 0.2f, trafficLightStatus == 1 ? 1.0f : 0.2f, 0.0f); glPushMatrix(); glTranslatef(0.0f, 0.0f, 0.13f); glutSolidSphere(0.07, 8, 8); glPopMatrix();
    glColor3f(0.0f, trafficLightStatus == 2 ? 1.0f : 0.2f, 0.0f); glPushMatrix(); glTranslatef(0.0f, -0.2f, 0.13f); glutSolidSphere(0.07, 8, 8); glPopMatrix();
    glPopMatrix();
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Camera 3D Orbit View Control
    glTranslatef(0.0f, -0.2f, -cam_distance);
    glRotatef(cam_angleX, 1.0f, 0.0f, 0.0f);
    glRotatef(cam_angleY, 0.0f, 1.0f, 0.0f);

    // ==========================================
    // ☀️ PERFECT SUN VISIBILITY LAYER
    // ==========================================
    glDepthMask(GL_FALSE);
    glPushMatrix();
    glTranslatef(1.6f, 2.9f, -2.5f);  // Sitting precisely on top of the background valley
    glColor3f(1.0f, 0.90f, 0.0f);
    glutSolidSphere(0.38, 32, 32);
    glPopMatrix();
    glDepthMask(GL_TRUE);

    // 1. Background: Mountains
    drawMountain(-1.5f, -4.5f, 3.0f, 2.5f);
    drawMountain(1.5f, -4.5f, 3.8f, 2.8f);  // Center peak
    drawMountain(4.5f, -4.5f, 2.6f, 2.2f);

    // Clouds
    drawCloud(-2.0f, 3.2f, -3.5f);
    drawCloud(0.5f, 3.4f, -3.8f);
    drawCloud(3.0f, 3.1f, -3.2f);

    // Flying Birds
    drawBird(bird_x, 2.4f, -2.0f);
    drawBird(bird_x - 1.2f, 2.7f, -2.5f);

    // Left Side Foreground: Bot Gach & Traffic Light
    drawBanyanTree(-4.8f, -1.0f);
    drawTrafficLight(-3.3f, 0.8f);

    // 2. Road Plane (Y-height at -1.0f, spans from Z = -1.8f to +1.8f)
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_QUADS);
        glVertex3f(-7.0f, -1.0f, -1.8f); glVertex3f(7.0f, -1.0f, -1.8f);
        glVertex3f(7.0f, -1.0f, 1.8f);  glVertex3f(-7.0f, -1.0f, 1.8f);
    glEnd();

    // Road Center Dividers
    glColor3f(1.0f, 1.0f, 1.0f);
    for(float i = -6.5f; i <= 6.5f; i += 2.0f) {
        glBegin(GL_QUADS);
            glVertex3f(i, -0.99f, -0.04f); glVertex3f(i + 1.0f, -0.99f, -0.04f);
            glVertex3f(i + 1.0f, -0.99f, 0.04f); glVertex3f(i, -0.99f, 0.04f);
        glEnd();
    }

    // Clear & Solid 3D Stop Line
    glColor3f(0.95f, 0.95f, 0.95f);
    glPushMatrix();
    glTranslatef(stopLineX, -0.96f, 0.8f); glScalef(0.18f, 0.03f, 1.4f); glutSolidCube(1.0);
    glPopMatrix();

    // 3. Cars Placement
    // Left Lane: Red Car (Z = 0.7f)
    glPushMatrix(); glTranslatef(car1_x, -0.7f, 0.7f); drawRedCarWithDoor(); glPopMatrix();

    // --- REPOSITIONED: Blue Car perfectly embedded inside the gray asphalt lane (Z = -0.7f) ---
    glPushMatrix();
    glTranslatef(car2_x, -0.72f, car2_z); // Smooth tire alignment on road height matrix
    glRotatef(car2_rotY, 0.0f, 1.0f, 0.0f);
    drawBlueCar();
    glPopMatrix();

    // 4. Storage Box
    glPushMatrix();
    glTranslatef(2.5f, -0.8f, 1.4f); glColor3f(0.55f, 0.35f, 0.15f);
    glPushMatrix(); glScalef(0.5f, 0.4f, 0.4f); glutSolidCube(1.0); glPopMatrix();
    glTranslatef(0.26f, 0.0f, 0.0f); if(isStorageDoorOpen) glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    glColor3f(0.7f, 0.45f, 0.2f); glPushMatrix(); glScalef(0.03f, 0.38f, 0.38f); glutSolidCube(1.0); glPopMatrix();
    glPopMatrix();

    glutSwapBuffers();
}

void idle() {
    // Automatic Red Car Braking
    float frontBumperX = car1_x + 0.7f;
    float stopEdgeX = stopLineX - 0.09f;

    if (trafficLightStatus == 0) {
        if (frontBumperX < stopEdgeX) {
            car1_x += car1_speed;
            if (car1_x + 0.7f > stopEdgeX) car1_x = stopEdgeX - 0.7f;
        }
    } else if (trafficLightStatus == 1) {
        car1_x += car1_speed * 0.25f;
    } else if (trafficLightStatus == 2) {
        car1_x += car1_speed;
    }

    if (car1_x > 7.0f) car1_x = -7.0f;

    // Door animation
    if (isCarDoorOpen && doorAngle < 75.0f) doorAngle += 5.0f;
    else if (!isCarDoorOpen && doorAngle > 0.0f) doorAngle -= 5.0f;

    // Bird animation
    bird_x += 0.015f;
    if (bird_x > 7.0f) bird_x = -7.0f;

    if (wingUp) {
        wingAngle += 4.0f;
        if (wingAngle > 35.0f) wingUp = false;
    } else {
        wingAngle -= 4.0f;
        if (wingAngle < -20.0f) wingUp = true;
    }

    glutPostRedisplay();
}

void keyboardKeys(unsigned char key, int x, int y) {
    switch (key) {
        case 'i': case 'I': cam_angleX -= 3.0f; break;
        case 'k': case 'K': cam_angleX += 3.0f; break;
        case 'j': case 'J': cam_angleY -= 3.0f; break;
        case 'l': case 'L': cam_angleY += 3.0f; break;
        case 't': case 'T': trafficLightStatus = (trafficLightStatus + 1) % 3; break;
        case 'o': case 'O': isCarDoorOpen = !isCarDoorOpen; break;
        case 'u': case 'U': isStorageDoorOpen = !isStorageDoorOpen; break;

        // Manual Blue Car Movement
        case 'a': case 'A': car2_rotY -= 5.0f; break;
        case 'd': case 'D': car2_rotY += 5.0f; break;
        case 'w': car2_x -= 0.15f; break; // Forward/Backward tracking inside road limits
        case 's': car2_x += 0.15f; break;
    }
    glutPostRedisplay();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)w / (double)h, 1.0, 30.0);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1000, 700);
    glutCreateWindow("Blue Car Perfectly Inside Road Frame");
    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboardKeys);
    glutIdleFunc(idle);
    glutMainLoop();
    return 0;
}
