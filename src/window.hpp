#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <GLFW/glfw3.h>

class Window
{
public:
    [[nodiscard]] Window();
    ~Window();

    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;

    Window(Window &&) = default;
    Window &operator=(Window &&) = default;

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

    GLFWwindow *m_window;
};

#endif // WINDOW_HPP
