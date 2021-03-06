#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#endif

#include "shader.h"
#include "camera.h"
#include "model.h"
#include "world.h"
#include "time.h"
#include "skymap.h"
#include "history/world_info.h"
#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

unsigned int loadTexture(const char* path);
unsigned int loadCubemap(std::vector<std::string> faces);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
//Camera camera(glm::vec3(0.0f, 6.0f, 5.0f));
//Camera camera(glm::vec3(0.0f, 0.0f, 155.0f));
//Camera camera(glm::vec3(0.0f, 0.0f, 60.0f));//LIGHTING test
Camera camera(glm::vec3(7.9019, 29.3491, 18.9233), glm::vec3(0.0f, 1.0f, 0.0f), -89.2999, -71.7001);//looking at the whole World

bool draw_world_info;

float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;

float current_mouse_x = lastX;
float current_mouse_y = lastY;

bool firstMouse = true;

world* World;
world_info* world_data;
timing* Time;
skymap* sky;
float* deltaTime;

int main() {

    std::cout << "For those who wish for the dawn of the second sun, would also shatter a risen moon." << std::endl;

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_SAMPLES, 4);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Dawn Of the Second Sun", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    glCullFace(GL_BACK);
    //glEnable(GL_MULTISAMPLE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
    glm::mat4 view = camera.GetViewMatrix();

    Time = new timing(false);
    deltaTime = Time->get_time_change_static();

    bool drawsky = false;

    sky = new skymap();
    sky->set_cam(view);
    sky->set_projection(projection);
    sky->init();

    draw_world_info = false;//determins which part of the program to create
    if (!draw_world_info) {
        World = new world();
        World->set_time(Time);
        World->set_projection(projection);
        World->init();
    }
    else {
        world_data = new world_info();
        world_data->set_projection(projection);
        world_data->init();
    }


    while (!glfwWindowShouldClose(window))
    {
        Time->update_time();
        processInput(window);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        view = camera.GetViewMatrix();


        if (!draw_world_info) {
            glEnable(GL_CULL_FACE);
            World->set_cam(view);
            World->draw();
            // World->draw_selection();

            if (drawsky) {
                sky->set_cam(view);
                sky->set_projection(projection);
                sky->draw();
            }

            World->update();
        }
        else {
            world_data->set_cam(view);
            world_data->draw();
        }


        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}


void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        // camera.print_stats();
        world_data->regen_map();
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, *deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, *deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, *deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, *deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, *deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, *deltaTime);
    //timimng changes
    if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS)
        Time->set_time_multipler(0);
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
        Time->set_time_multipler(1);
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
        Time->set_time_multipler(2);
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
        Time->set_time_multipler(3);
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
        Time->set_time_multipler(4);
    if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS)
        Time->toggle_frame_rates();
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
    if (World != NULL) {
       float zoom = camera.ProcessMouseScroll(yoffset);
       World->change_projection(glm::perspective(glm::radians(zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f));
    }
}

// glfw: whenever the mouse buttons are clicked, this callback is called
// ----------------------------------------------------------------------
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        std::cout << "clicked left button" << std::endl;
        World->process_mouse_action(current_mouse_x, current_mouse_y);
    }
    else {
        //std::cout << "clicked " << button<< std::endl;
    }
       
}