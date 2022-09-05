#pragma once
#include <iostream>
#include <string>
#include <exception>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

class shader_error: private std::exception
{
private:
    std::string base;
public:
    shader_error(GLuint obj, int shader_type, int len);
    std::string what();
};

class VPLRender
{
private:
    GLFWwindow* m_wnd{nullptr};
    GLuint m_obj[6];
    bool init_shader();
    void init_gl_obj();
    void review(int width, int height);
    static void keyfunc(GLFWwindow* wnd, int key, int scancode, int action, int mode);
public:
    VPLRender(const std::string& title = "VPL", int width = 1024, int height = 768);
    ~VPLRender();
    GLFWwindow* window();
    void paint(unsigned char* _data, int image_w, int image_h);
};

