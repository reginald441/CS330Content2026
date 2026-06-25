///////////////////////////////////////////////////////////////////////////////
// 2D Game Program
// ===============
// This program implements a simple 2D game using OpenGL and GLFW. The game
// involves circular objects (referred to as "balls") that move within a window 
// and interact with static rectangular bricks. The primary features of the game
// include:
// 
// - **Bricks**: Represented as either reflective or destructible. Reflective 
//   bricks change the direction of the balls upon collision, while destructible
//   bricks disappear when hit.
// - **Balls**: Circular objects that move randomly within the window. Their 
//   motion is influenced by collisions with bricks and window boundaries.
// - **Input Handling**: Players can spawn new balls by pressing the spacebar or 
//   close the game window by pressing the ESC key.
//
// Key Features:
// -------------
// - **Brick Types**:
//   - Reflective bricks alter the direction of the balls.
//   - Destructible bricks disappear upon collision.
// - **Ball Motion**:
//   - Balls move in random directions and bounce upon hitting boundaries.
//   - Collisions with bricks are handled dynamically.
// - **User Input**:
//   - Spacebar spawns a new ball at the center with random colors.
//   - ESC closes the application.
// 
// Dependencies:
// -------------
// - GLFW for window creation and input handling.
// - OpenGL for rendering graphics.
// 
// Author: [Your Name]
// Date: [Today's Date]
// Assignment: 8-2 Assignment - CS-330 Computational Graphics and Visualization
///////////////////////////////////////////////////////////////////////////////

#include <GLFW/glfw3.h>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <vector>
#include <windows.h>

using namespace std;

const float DEG2RAD = 3.14159f / 180.0f;

void processInput(GLFWwindow* window);

enum BRICKTYPE { REFLECTIVE, DESTRUCTABLE };
enum ONOFF { ON, OFF };

class Brick
{
public:
    float red, green, blue;
    float x, y, width, height;
    BRICKTYPE type;
    ONOFF alive;
    int hitsLeft;

    Brick(BRICKTYPE bt, float xx, float yy, float ww, float hh, float rr, float gg, float bb)
    {
        type = bt;
        x = xx;
        y = yy;
        width = ww;
        height = hh;
        red = rr;
        green = gg;
        blue = bb;
        alive = ON;

        if (type == DESTRUCTABLE)
            hitsLeft = 2;
        else
            hitsLeft = 99;
    }

    void drawBrick()
    {
        if (alive == OFF)
            return;

        glColor3f(red, green, blue);

        glBegin(GL_POLYGON);
        glVertex2f(x - width / 2, y - height / 2);
        glVertex2f(x + width / 2, y - height / 2);
        glVertex2f(x + width / 2, y + height / 2);
        glVertex2f(x - width / 2, y + height / 2);
        glEnd();

        glColor3f(0.0f, 0.0f, 0.0f);
        glLineWidth(2.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(x - width / 2, y - height / 2);
        glVertex2f(x + width / 2, y - height / 2);
        glVertex2f(x + width / 2, y + height / 2);
        glVertex2f(x - width / 2, y + height / 2);
        glEnd();
    }
};

class Circle
{
public:
    float red, green, blue;
    float radius;
    float x;
    float y;
    float speed;
    int direction;

    Circle(float xx, float yy, float rr, float s, int dir, float r, float g, float b)
    {
        x = xx;
        y = yy;
        radius = rr;
        speed = s;
        direction = dir;
        red = r;
        green = g;
        blue = b;
    }

    void CheckCollision(Brick* brick)
    {
        if (brick->alive == OFF)
            return;

        float closestX = max(brick->x - brick->width / 2, min(x, brick->x + brick->width / 2));
        float closestY = max(brick->y - brick->height / 2, min(y, brick->y + brick->height / 2));

        float distanceX = x - closestX;
        float distanceY = y - closestY;

        float distanceSquared = (distanceX * distanceX) + (distanceY * distanceY);

        if (distanceSquared < (radius * radius))
        {
            if (brick->type == DESTRUCTABLE)
            {
                brick->hitsLeft--;

                if (brick->hitsLeft == 1)
                {
                    brick->red = 1.0f;
                    brick->green = 0.7f;
                    brick->blue = 0.0f;
                }
                else if (brick->hitsLeft <= 0)
                {
                    brick->alive = OFF;
                }
            }

            direction = GetBounceDirection();
            speed += 0.0004f;

            if (speed > 0.018f)
                speed = 0.018f;
        }
    }

    int GetBounceDirection()
    {
        return (rand() % 8) + 1;
    }

    void MoveOneStep()
    {
        if (direction == 1 || direction == 5 || direction == 6)
            y += speed;

        if (direction == 2 || direction == 5 || direction == 7)
            x += speed;

        if (direction == 3 || direction == 7 || direction == 8)
            y -= speed;

        if (direction == 4 || direction == 6 || direction == 8)
            x -= speed;

        if (x > 1 - radius)
        {
            x = 1 - radius;
            direction = GetBounceDirection();
        }

        if (x < -1 + radius)
        {
            x = -1 + radius;
            direction = GetBounceDirection();
        }

        if (y > 1 - radius)
        {
            y = 1 - radius;
            direction = GetBounceDirection();
        }

        if (y < -1 + radius)
        {
            y = -1 + radius;
            direction = GetBounceDirection();
        }
    }

    void DrawCircle()
    {
        glColor3f(red, green, blue);

        glBegin(GL_POLYGON);
        for (int i = 0; i < 360; i++)
        {
            float degInRad = i * DEG2RAD;
            glVertex2f(cos(degInRad) * radius + x, sin(degInRad) * radius + y);
        }
        glEnd();
    }
};

vector<Circle> world;
vector<Brick> bricks;

float RandomFloat()
{
    return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

void AddBall()
{
    float r = RandomFloat();
    float g = RandomFloat();
    float b = RandomFloat();

    int dir = (rand() % 8) + 1;

    world.push_back(Circle(0.0f, -0.55f, 0.045f, 0.0025f, dir, r, g, b));
}

void CreateBrickWall()
{
    bricks.clear();

    bricks.push_back(Brick(DESTRUCTABLE, -0.75f, 0.65f, 0.25f, 0.12f, 0.9f, 0.1f, 0.1f));
    bricks.push_back(Brick(DESTRUCTABLE, -0.45f, 0.65f, 0.25f, 0.12f, 0.1f, 0.7f, 1.0f));
    bricks.push_back(Brick(REFLECTIVE, -0.15f, 0.65f, 0.25f, 0.12f, 0.2f, 0.9f, 0.3f));
    bricks.push_back(Brick(DESTRUCTABLE, 0.15f, 0.65f, 0.25f, 0.12f, 1.0f, 0.5f, 0.0f));
    bricks.push_back(Brick(DESTRUCTABLE, 0.45f, 0.65f, 0.25f, 0.12f, 0.8f, 0.2f, 0.9f));
    bricks.push_back(Brick(REFLECTIVE, 0.75f, 0.65f, 0.25f, 0.12f, 1.0f, 1.0f, 0.2f));

    bricks.push_back(Brick(DESTRUCTABLE, -0.60f, 0.45f, 0.28f, 0.12f, 0.0f, 0.8f, 0.8f));
    bricks.push_back(Brick(REFLECTIVE, -0.25f, 0.45f, 0.28f, 0.12f, 0.9f, 0.4f, 0.4f));
    bricks.push_back(Brick(DESTRUCTABLE, 0.10f, 0.45f, 0.28f, 0.12f, 0.4f, 0.7f, 1.0f));
    bricks.push_back(Brick(DESTRUCTABLE, 0.45f, 0.45f, 0.28f, 0.12f, 0.7f, 1.0f, 0.4f));

    bricks.push_back(Brick(REFLECTIVE, -0.40f, 0.25f, 0.35f, 0.12f, 0.7f, 0.7f, 0.7f));
    bricks.push_back(Brick(DESTRUCTABLE, 0.00f, 0.25f, 0.35f, 0.12f, 1.0f, 0.2f, 0.6f));
    bricks.push_back(Brick(REFLECTIVE, 0.40f, 0.25f, 0.35f, 0.12f, 0.2f, 1.0f, 0.6f));

    // Bottom paddle
    bricks.push_back(Brick(REFLECTIVE, 0.0f, -0.85f, 0.45f, 0.08f, 1.0f, 1.0f, 1.0f));
}

void CheckCircleCollisions()
{
    for (int i = 0; i < world.size(); i++)
    {
        for (int j = i + 1; j < world.size(); j++)
        {
            float dx = world[i].x - world[j].x;
            float dy = world[i].y - world[j].y;
            float distance = sqrt(dx * dx + dy * dy);

            if (distance < world[i].radius + world[j].radius)
            {
                world[i].direction = world[i].GetBounceDirection();
                world[j].direction = world[j].GetBounceDirection();

                world[i].red = RandomFloat();
                world[i].green = RandomFloat();
                world[i].blue = RandomFloat();

                world[j].red = RandomFloat();
                world[j].green = RandomFloat();
                world[j].blue = RandomFloat();
            }
        }
    }
}

int main(void)
{
    srand(static_cast<unsigned int>(time(NULL)));

    if (!glfwInit())
        return -1;

    GLFWwindow* window = glfwCreateWindow(1000, 800, "8-2 Assignment - Brick Collision Animation", NULL, NULL);

    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    CreateBrickWall();

    world.push_back(Circle(-0.45f, -0.50f, 0.05f, 0.007f, 2, 1.0f, 0.2f, 0.2f));
    world.push_back(Circle(0.45f, -0.45f, 0.05f, 0.006f, 6, 0.2f, 0.6f, 1.0f));

    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        for (int i = 0; i < bricks.size(); i++)
        {
            bricks[i].drawBrick();
        }

        for (int i = 0; i < world.size(); i++)
        {
            for (int j = 0; j < bricks.size(); j++)
            {
                world[i].CheckCollision(&bricks[j]);
            }

            world[i].MoveOneStep();
            world[i].DrawCircle();
        }

        CheckCircleCollisions();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window)
{
    static bool spacePressed = false;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

    // Move paddle left
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
    {
        bricks.back().x -= 0.015f;
    }

    // Move paddle right
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
    {
        bricks.back().x += 0.015f;
    }

    // Keep paddle on screen
    if (bricks.back().x < -0.75f)
        bricks.back().x = -0.75f;

    if (bricks.back().x > 0.75f)
        bricks.back().x = 0.75f;

    // Add one ball when Space is pressed
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !spacePressed)
    {
        AddBall();
        spacePressed = true;
    }

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
    {
        spacePressed = false;
    }
}