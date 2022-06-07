#include "application.hpp"

#include <iostream>

namespace
{

void glfw_error_callback(int error [[maybe_unused]], const char *description)
{
    std::cerr << description << std::endl;
}

} // namespace

Application::Application()
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

    int width {};
    int height {};
    glfwGetFramebufferSize(m_window, &width, &height);

    m_renderer = std::make_unique<Renderer>(m_window, width, height);
}

Application::~Application()
{
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void Application::run()
{
    int frames {};
    auto last_second = glfwGetTime();
    auto last_frame_time = last_second;

    while (!glfwWindowShouldClose(m_window))
    {
        glfwPollEvents();

        const auto now = glfwGetTime();
        const auto elapsed_seconds = now - last_frame_time;
        last_frame_time = now;

        update(elapsed_seconds);

        m_renderer->draw_frame();

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

void Application::update(double elapsed_seconds)
{
}

void Application::cursor_pos_callback(GLFWwindow *window,
                                      double xpos,
                                      double ypos)
{
}

void Application::mouse_button_callback(GLFWwindow *window,
                                        int button,
                                        int action,
                                        int mods)
{
}

void Application::scroll_callback(GLFWwindow *window,
                                  double xoffset,
                                  double yoffset)
{
}

void Application::key_callback(
    GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS && key == GLFW_KEY_F)
    {
        // Toggle fullscreen

        auto *const monitor = glfwGetPrimaryMonitor();
        const auto *const video_mode = glfwGetVideoMode(monitor);
        if (glfwGetWindowMonitor(window) != nullptr)
        {
            constexpr int width {640};
            constexpr int height {480};
            const auto pos_x = std::max((video_mode->width - width) / 2, 0);
            const auto pos_y = std::max((video_mode->height - height) / 2, 0);
            glfwSetWindowMonitor(
                window, nullptr, pos_x, pos_y, width, height, GLFW_DONT_CARE);
        }
        else
        {
            glfwSetWindowMonitor(window,
                                 monitor,
                                 0,
                                 0,
                                 video_mode->width,
                                 video_mode->height,
                                 GLFW_DONT_CARE);
        }
    }
}

void Application::framebuffer_size_callback(GLFWwindow *window,
                                            int width,
                                            int height)
{
    auto app = static_cast<Application *>(glfwGetWindowUserPointer(window));
    app->m_renderer->resize_framebuffer(static_cast<std::uint32_t>(width),
                                        static_cast<std::uint32_t>(height));
}
