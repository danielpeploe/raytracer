/*==================================================================================
* Ray Tracer for COSC363 Assignment 2
*
* Daniel Peploe
*===================================================================================
*/
#include <iostream>
#include <cmath>
#include <vector>
#include <glm/glm.hpp>
#include "Sphere.h"
#include "SceneObject.h"
#include "Ray.h"
#include "Plane.h"
#include "TextureBMP.h"
#include <GL/freeglut.h>

using namespace std;

const float EDIST = 40.0;
const int NUMDIV = 500;
const int MAX_STEPS = 5;
const float XMIN = -10.0;
const float XMAX = 10.0;
const float YMIN = -10.0;
const float YMAX = 10.0;

TextureBMP texture;
vector<SceneObject*> sceneObjects;


//----------------------------------------------------------------------------------
//   Computes the colour value obtained by tracing a ray and finding its
//     closest point of intersection with objects in the scene.
//----------------------------------------------------------------------------------
glm::vec3 trace(Ray ray, int step)
{
    const glm::vec3 backgroundCol(0.0f); // Background colour = (0,0,0)
    const glm::vec3 lightPos(10, 14, -3); // Light's position
    glm::vec3 color(0.0f);
    SceneObject* obj;

    ray.closestPt(sceneObjects); // Compare the ray with all objects in the scene
    if (ray.index == -1) return backgroundCol; // No intersection
    obj = sceneObjects[ray.index]; // Object on which the closest point of intersection is found

    if (ray.index == 3)
    {
        // Stripe pattern
        const int stripeWidth = 5;
        const int iz = static_cast<int>(ray.hit.z) / stripeWidth;
        const int kz = iz % 2; // 2 colors
        const int ix = static_cast<int>(ray.hit.x) / stripeWidth;
        const int kx = ix % 2; // 2 colors

        if (ray.hit.x < 0)
        {
            color = ((kz == kx) || (!kz == !kx)) ? glm::vec3(1, 1, 0.5) : glm::vec3(0, 1, 0);
        }
        else
        {
            color = ((kz == kx) || (!kz == !kx)) ? glm::vec3(0, 1, 0) : glm::vec3(1, 1, 0.5);
        }

        obj->setColor(color);
    }

    if (ray.index == 0) // Sphere 1
    {
        const glm::vec3 center(-12.0, -10, -90.0);
        const glm::vec3 normalised = glm::normalize(ray.hit - center);
        const float s = (0.5f - atan2(normalised.z, normalised.x) + M_PI) / (2 * M_PI);
        const float t = 0.5f + asin(normalised.y) / M_PI;
        color = texture.getColorAt(s, t);
        obj->setColor(color);
    }

    // Calculate lighting
    color = obj->lighting(lightPos, -ray.dir, ray.hit);

    // Shadows
    const glm::vec3 lightVec = lightPos - ray.hit;
    Ray shadowRay(ray.hit, lightVec); // Shadow ray
    shadowRay.closestPt(sceneObjects); // Find closest point intersection on shadow ray
    const float lightDist = glm::length(lightVec);

    if ((shadowRay.index > -1) && (shadowRay.dist < lightDist))
    {
        color *= (shadowRay.index == 1 || shadowRay.index == 2) ? 0.3f : 0.1f; // Apply shadow
    }

    // Handle reflection, transparency and refraction
    if (obj->isReflective() && step < MAX_STEPS)
    {
        const float rho = obj->getReflectionCoeff();
        const glm::vec3 normalVec = obj->normal(ray.hit);
        const glm::vec3 reflectedDir = glm::reflect(ray.dir, normalVec);
        Ray reflectedRay(ray.hit, reflectedDir);
        const glm::vec3 reflectedColor = trace(reflectedRay, step + 1); // Recursive
        color += (rho * reflectedColor);
    }

    if (obj->isTransparent() && step < MAX_STEPS)
    {
        Ray refractedRay(ray.hit, ray.dir);
        refractedRay.closestPt(sceneObjects);
        Ray secondaryRay(refractedRay.hit, ray.dir);
        const glm::vec3 refractedColor = trace(secondaryRay, step + 1); // Recursive
        color = refractedColor * obj->getTransparencyCoeff() + (1 - obj->getTransparencyCoeff()) * color;
    }

    if (obj->isRefractive() && step < MAX_STEPS)
    {
        const float eta = 1 / obj->getRefractiveIndex();
        const glm::vec3 normalVec = obj->normal(ray.hit);
        const glm::vec3 refractedDir = glm::refract(ray.dir, normalVec, eta);
        Ray refractedRay(ray.hit, refractedDir);
        refractedRay.closestPt(sceneObjects);
        const glm::vec3 m = obj->normal(refractedRay.hit);
        const glm::vec3 h = glm::refract(refractedDir, -m, 1.0f / eta);
        Ray secondaryRay(refractedRay.hit, h);
        const glm::vec3 refractedColor = trace(secondaryRay, step + 1); // Recursive
        color = refractedColor * obj->getRefractionCoeff() + (1 - obj->getRefractionCoeff()) * color;
    }

    // Apply fog effect
    const float t = ray.hit.z / -195;
    color = (1 - t) * color + t * glm::vec3(1, 1, 1);

    return color;
}

//---------------------------------------------------------------------------------------
// Displays the ray traced image by drawing each cell as a quad.
//---------------------------------------------------------------------------------------
void display()
{
    const float cellX = (XMAX - XMIN) / NUMDIV;  // Cell width
    const float cellY = (YMAX - YMIN) / NUMDIV;  // Cell height
    glm::vec3 eye(0.0f);

    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glBegin(GL_QUADS);  // Each cell is a tiny quad.

    for (int i = 0; i < NUMDIV; i++)    // Scan every cell of the image plane
    {
        const float xp = XMIN + i * cellX;
        for (int j = 0; j < NUMDIV; j++)
        {
            const float yp = YMIN + j * cellY;
            const glm::vec3 dir(xp + 0.5f * cellX, yp + 0.5f * cellY, -EDIST);  // Direction of the primary ray

            Ray ray = Ray(eye, dir);

            const glm::vec3 col = trace(ray, 1); // Trace the primary ray and get the colour value
            glColor3f(col.r, col.g, col.b);
            glVertex2f(xp, yp);             // Draw each cell with its color value
            glVertex2f(xp + cellX, yp);
            glVertex2f(xp + cellX, yp + cellY);
            glVertex2f(xp, yp + cellY);
        }
    }

    glEnd();
    glFlush();
}

//----------------------------------------------------------------------------------
//   Creates scene objects and add them to the list of scene objects. It also
//     initializes the OpenGL 2D orthographc projection matrix for drawing the
//     the ray traced image.
//----------------------------------------------------------------------------------
void initialize()
{
    texture = TextureBMP("jabulani.bmp");

    glMatrixMode(GL_PROJECTION);
    gluOrtho2D(XMIN, XMAX, YMIN, YMAX);

    glClearColor(0, 0, 0, 1);

    Sphere *sphere1 = new Sphere(glm::vec3(-12.0, -10, -90.0), 5.0);
    sphere1->setColor(glm::vec3(0, 0, 1));
    sceneObjects.push_back(sphere1);

    Sphere *sphere2 = new Sphere(glm::vec3(0.0, -10, -90.0), 5.0);
    sphere2->setColor(glm::vec3(0, 1, 1));
    sceneObjects.push_back(sphere2);
    sphere2->setRefractivity(true, 0.8, 1.5);

    Sphere *sphere3 = new Sphere(glm::vec3(12.0, -10, -90.0), 5.0);
    sphere3->setColor(glm::vec3(1, 0, 0));
    sceneObjects.push_back(sphere3);
    sphere3->setTransparency(true, 0.8);

    const struct PlaneData {
        glm::vec3 pointA;
        glm::vec3 pointB;
        glm::vec3 pointC;
        glm::vec3 pointD;
        glm::vec3 color;
        bool isReflective;
        bool hasSpecularity;
    } planes[] = {
        {{-20., -15, 5}, {20., -15, 5}, {20., -15, -200}, {-20., -15, -200}, {0.8, 0.8, 0}, false, false}, // Floor
        {{-20., -15, -200}, {20., -15, -200}, {20., 15, -200}, {-20., 15, -200}, {0.11, 0.46, 0.98}, false, false}, // Back Wall
        {{-20., 15, -200}, {20., 15, -200}, {20., 15, 5}, {-20., 15, 5}, {0.0, 1, 0.0}, false, false}, // Roof
        {{20., -15, -200}, {20., -15, 5}, {20., 15, 5}, {20., 15, -200}, {0.96, 0.44, 0.62}, false, false}, // Right Wall
        {{-20., -15, 5}, {-20., -15, -200}, {-20., 15, -200}, {-20., 15, 5}, {1, 1, 0.0}, false, false}, // Left Wall
        {{-15., -10, -150}, {15., -10, -150}, {15., 10, -150}, {-15., 10, -150}, {0, 0, 0}, true, false}, // Mirror Plane
        {{-20., -10, 0}, {20., -10, 0}, {20., 10, 0}, {-20., 10, 0}, {1, 1, 1}, true, false} // Mirror Behind Camera
    };

    // Create Planes
    for (const auto& planeData : planes) {
        Plane *plane = new Plane(planeData.pointA, planeData.pointB, planeData.pointC, planeData.pointD);
        plane->setColor(planeData.color);
        sceneObjects.push_back(plane);
        plane->setReflectivity(planeData.isReflective, 1);
        plane->setSpecularity(planeData.hasSpecularity);
    }

}

int main(int argc, char *argv[]) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB );
    glutInitWindowSize(500, 500);
    glutInitWindowPosition(20, 20);
    glutCreateWindow("Raytracer");

    glutDisplayFunc(display);
    initialize();

    glutMainLoop();
    return 0;
}
