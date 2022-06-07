#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "old_renderer.hpp"

#include <GLFW/glfw3.h>

#include <memory>
#include <vector>

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
    void update(double elapsed_seconds);

    static void
    cursor_pos_callback(GLFWwindow *window, double xpos, double ypos);
    static void
    mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
    static void
    scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
    static void key_callback(
        GLFWwindow *window, int key, int scancode, int action, int mods);
    static void
    framebuffer_size_callback(GLFWwindow *window, int width, int height);

    GLFWwindow *m_window;
    std::unique_ptr<Renderer> m_renderer;
};

#endif // APPLICATION_HPP
