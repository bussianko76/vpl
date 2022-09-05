#include "VPLRender.hpp"
#define EXIT std::exit(EXIT_FAILURE)

extern bool b_pause_play;
extern bool b_seekable;
extern std::size_t idx;
extern double frame_per_sec;

static const char* vertex_shader_src =
        "#version 330 core\n"
        "layout(location=0) in vec3 aPos;\n"
        "layout(location=1) in vec2 aCoord;\n"
        "out vec2 coord;\n"
        "void main()\n"
        "{\n"
        "       gl_Position = vec4(aPos, 1.0);\n"
        "       coord = vec2(aCoord.x, 1.0 - aCoord.y);\n"
        "};\n";

static const char* fragment_shader_src =
        "#version 330 core\n"
        "in vec2 coord;\n"
        "out vec4 texture;\n"
        "uniform sampler2D image;\n"
        "void main()\n"
        "{\n"
        "       texture = texture2D(image, coord);\n"
        "};\n";

bool VPLRender::init_shader()
{
    try
    {
        int ret{0}, len{0};
        uint v_id = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(v_id, 1, &vertex_shader_src, nullptr);
        glCompileShader(v_id);
        glGetShaderiv(v_id, GL_COMPILE_STATUS, &ret);
        if(ret != GLFW_TRUE)
        {
            glGetShaderiv(v_id, GL_INFO_LOG_LENGTH, &len);
            throw shader_error{v_id, GL_VERTEX_SHADER, len};
        }

        uint f_id = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(f_id, 1, &fragment_shader_src, nullptr);
        glCompileShader(f_id);
        glGetShaderiv(f_id, GL_COMPILE_STATUS, &ret);
        if(ret != GLFW_TRUE)
        {
            glGetShaderiv(f_id, GL_INFO_LOG_LENGTH, &len);
            throw shader_error{f_id, GL_FRAGMENT_SHADER, len};
        }

        m_obj[5] = glCreateProgram();
        glAttachShader(m_obj[5], v_id);
        glAttachShader(m_obj[5], f_id);
        glLinkProgram(m_obj[5]);
        glGetProgramiv(m_obj[5], GL_LINK_STATUS, &ret);
        if(ret != GL_TRUE)
        {
            glGetProgramiv(m_obj[5], GL_INFO_LOG_LENGTH, &len);
            throw shader_error{m_obj[5], GL_PROGRAM, len};
        }
        glDetachShader(m_obj[5], v_id);
        glDetachShader(m_obj[5], f_id);
        glDeleteShader(v_id);
        glDeleteShader(f_id);
    }
    catch(shader_error& e)
    {
        std::cerr << e.what() << "\n";
        return false;
    }
    return true;
}

std::string shader_error::what()
{
    return base;
}

void VPLRender::init_gl_obj()
{
    auto& vao = m_obj[0];
    auto& vbo = m_obj[1];
    auto& tbo = m_obj[2];
    auto& ebo = m_obj[3];
    auto& txt = m_obj[4];

    static const int vertices[12]{1, 1, 0, 1, -1, 0, -1, -1, 0, -1, 1, 0};
    static const int coords[8]{1, 1, 1, 0, 0, 0, 0, 1};
    static const int indices[6]{0, 1, 3, 3, 2, 1};

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &tbo);
    glGenBuffers(1, &ebo);
    glGenTextures(1, &txt);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_INT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, tbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(coords), coords, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_INT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glBindVertexArray(0);

    glBindTexture(GL_TEXTURE_2D, txt);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void VPLRender::review(int width, int height)
{
    double sar = static_cast<double>(width) / static_cast<double>(height);
    int w, h, img_w{0}, img_h{0};
    glfwGetFramebufferSize(m_wnd, &w, &h);
    if(width > w)
    {
        img_w = w;
        img_h = static_cast<int>(w / sar);
    }
    else if(width <= w)
    {
        img_w = width;
        img_h = height;
    }
    if(img_h > h)
    {
        img_h = h;
        img_w = static_cast<int>(h * sar);
    }
    glViewport((w - img_w)/2, (h - img_h)/2, img_w, img_h);
}

void VPLRender::keyfunc(GLFWwindow *wnd, int key, int scancode, int action, int mode)
{
    switch(key)
    {
    case GLFW_KEY_ESCAPE:
    {
        if(action == GLFW_PRESS && action != GLFW_REPEAT)
        {
            b_pause_play = false;
            glfwSetWindowShouldClose(wnd, GLFW_TRUE);
        }
    }break;
    case GLFW_KEY_F:
    {
        static bool key_f{false};
        static int old_w{0}, old_h{0};
        const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        if(action == GLFW_PRESS && action != GLFW_REPEAT)
        {
            key_f = !key_f;
            if(key_f)
            {
                glfwGetFramebufferSize(wnd, &old_w, &old_h);
                glfwSetWindowMonitor(wnd, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, mode->refreshRate);
            }
            else if(!key_f)
            {
                glfwSetWindowMonitor(wnd, nullptr, (mode->width - old_w)/2, (mode->height - old_h)/2, old_w, old_h, mode->refreshRate);
            }
        }
    }break;
    case GLFW_KEY_K:
    case GLFW_KEY_SPACE:
    {
        if(action == GLFW_PRESS && action != GLFW_REPEAT)
        {
            b_pause_play = !b_pause_play;
        }
    }break;
    case GLFW_KEY_L:
    {
        if(action == GLFW_PRESS) b_pause_play = false;
        else if(action == GLFW_RELEASE) b_pause_play = true;
    }break;
    case GLFW_KEY_UP:
    {
        if(action == GLFW_PRESS && action != GLFW_REPEAT)
        {
            b_seekable = true;
            idx += (frame_per_sec * 60);
        }
    }break;

    case GLFW_KEY_DOWN:
    {
        if(action == GLFW_PRESS && action != GLFW_REPEAT)
        {
            b_seekable = true;
            if(idx >= (frame_per_sec * 60)) idx -= (frame_per_sec * 60);
            else idx = 0;
        }
    }break;

    case GLFW_KEY_RIGHT:
    {
        if(action == GLFW_PRESS && action != GLFW_REPEAT)
        {
            b_seekable = true;
            idx += (frame_per_sec * 30);
        }
    }break;
    case GLFW_KEY_LEFT:
    {
        if(action == GLFW_PRESS && action != GLFW_REPEAT)
        {
            b_seekable = true;
            if(idx >= (frame_per_sec * 30)) idx -= (frame_per_sec * 30);
            else idx = 0;
        }
    }break;
    default: break;
    }
}

VPLRender::VPLRender(const std::string &title, int width, int height)
{
    if(!glfwInit())
    {
        std::cerr << "Couldn't initialize GLFW library." <<"\n";
        EXIT;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_wnd = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if(!m_wnd)
    {
        std::cerr << "Couldn't create main window." << "\n";
        EXIT;
    }
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    glfwSetWindowPos(m_wnd, (mode->width - width)/2, (mode->height - height)/2);
    glfwMakeContextCurrent(m_wnd);
    glfwSetInputMode(m_wnd, GLFW_STICKY_KEYS, GLFW_TRUE);
    glfwSetKeyCallback(m_wnd, keyfunc);

    glewExperimental = true;
    if(glewInit() != GLEW_OK)
    {
        std::cerr << "Couldn't init GLEW library." << std::endl;
        EXIT;
    }

    if(init_shader()) init_gl_obj();

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
}

VPLRender::~VPLRender()
{
    glDeleteProgram(m_obj[5]);
    glDeleteTextures(1, &m_obj[4]);
    glDeleteBuffers(1, &m_obj[3]);
    glDeleteBuffers(1, &m_obj[2]);
    glDeleteBuffers(1, &m_obj[1]);
    glDeleteVertexArrays(1, &m_obj[0]);
    if(m_wnd)
    {
        glfwDestroyWindow(m_wnd);
    }
    glfwTerminate();
}

GLFWwindow *VPLRender::window()
{
    if(m_wnd) return m_wnd;
    return  nullptr;
}

void VPLRender::paint(unsigned char *_data, int image_w, int image_h)
{
    glBindTexture(GL_TEXTURE_2D, m_obj[4]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_w, image_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, _data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glUseProgram(m_obj[5]);

    review(image_w, image_h);

    glBindVertexArray(m_obj[0]);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glBindTexture(GL_TEXTURE_2D, 0);
}

shader_error::shader_error(GLuint obj, int shader_type, int len)
{
    switch (shader_type) {
    case GL_VERTEX_SHADER:
    {
        char emsg[len + 1];
        glGetShaderInfoLog(obj, len, nullptr, &emsg[0]);
        base = "ERROR!SHADER::VERTEX::COMPILE_FAILED: " + std::string(emsg);
    }break;
    case GL_FRAGMENT_SHADER:
    {
        char emsg[len + 1];
        glGetShaderInfoLog(obj, len, nullptr, &emsg[0]);
        base = "ERROR!SHADER::FRAGMENT::COMPILE_FAILED: " + std::string(emsg);
    }break;
    case GL_PROGRAM:
    {
        char emsg[len + 1];
        glGetProgramInfoLog(obj, len, nullptr, &emsg[0]);
        base = "ERROR!SHADER::PROGRAM::LINK_FAILED: " + std::string(emsg);
    }break;
    default: break;
    }
}
