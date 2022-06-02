#include "window.hpp"

#include <iostream>

namespace
{

void glfw_error_callback(int error [[maybe_unused]], const char *description)
{
    std::cerr << description << std::endl;
}

} // namespace

Window::Window()
{
    glfwSetErrorCallback(glfw_error_callback);

    glfwInit();

    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(640, 480, "Vulkan engine", nullptr, nullptr);

    glfwSetWindowUserPointer(m_window, this);

    glfwSetCursorPosCallback(m_window, cursor_pos_callback);
    glfwSetMouseButtonCallback(m_window, mouse_button_callback);
    glfwSetScrollCallback(m_window, scroll_callback);
    glfwSetKeyCallback(m_window, key_callback);
    glfwSetFramebufferSizeCallback(m_window, framebuffer_size_callback);
}

Window::~Window()
{
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

std::vector<const char *> Window::get_required_vulkan_instance_extensions()
{
    std::uint32_t extension_count {};
    const auto extensions = glfwGetRequiredInstanceExtensions(&extension_count);
    return {extensions, extensions + extension_count};
}

void Window::run()
{
    int frames {};
    auto last_second = glfwGetTime();
    auto last_frame_time = last_second;

    while (!glfwWindowShouldClose(m_window))
    {
        glfwPollEvents();

        const auto now = glfwGetTime();
        const auto elapsed_seconds = now - last_frame_time;
        update(elapsed_seconds);
        last_frame_time = now;

        ++frames;

        if (now - last_second >= 1.0)
        {
            std::ostringstream title_stream;
            title_stream << "Vulkan engine - " << frames << " fps";
            glfwSetWindowTitle(m_window, title_stream.str().c_str());
            last_second = now;
            frames = 0;
        }
    }
}

void Window::update(double elapsed_seconds)
{
}

void Window::cursor_pos_callback(GLFWwindow *window, double xpos, double ypos)
{
}

void Window::mouse_button_callback(GLFWwindow *window,
                                   int button,
                                   int action,
                                   int mods)
{
}

void Window::scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
}

void Window::key_callback(
    GLFWwindow *window, int key, int scancode, int action, int mods)
{
}

void Window::framebuffer_size_callback(GLFWwindow *window,
                                       int width,
                                       int height)
{
}
