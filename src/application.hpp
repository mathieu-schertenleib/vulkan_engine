#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "renderer.hpp"

#include <GLFW/glfw3.h>

#include <memory>
#include <vector>

struct Glfw_context
{
    [[nodiscard]] Glfw_context();
    ~Glfw_context();
};

struct ImGui_context
{
    [[nodiscard]] ImGui_context();
    ~ImGui_context();
};

class Application
{
public:
    [[nodiscard]] Application();
    ~Application();

    Application(const Application &) = delete;
    Application &operator=(const Application &) = delete;

    Application(Application &&) = default;
    Application &operator=(Application &&) = default;

    void run();

private:
    static void key_callback(
        GLFWwindow *window, int key, int scancode, int action, int mods);
    static void
    framebuffer_size_callback(GLFWwindow *window, int width, int height);

    [[nodiscard]] bool is_fullscreen();
    void set_fullscreen();
    void set_windowed();

    Glfw_context m_glfw_context;
    GLFWwindow *m_window;
    ImGui_context m_imgui_context;
    std::unique_ptr<Renderer> m_renderer;
};

#endif // APPLICATION_HPP
