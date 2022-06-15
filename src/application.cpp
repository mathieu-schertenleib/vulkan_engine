#include "application.hpp"

#include "imgui.h"

#include <iostream>

namespace
{

void glfw_error_callback(int error [[maybe_unused]], const char *description)
{
    std::cerr << "GLFW error " << error << ": " << description << std::endl;
}

} // namespace

Glfw_context::Glfw_context()
{
    glfwSetErrorCallback(glfw_error_callback);
    glfwInit();
}

Glfw_context::~Glfw_context()
{
    glfwTerminate();
}

ImGui_context::ImGui_context()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
}

ImGui_context::~ImGui_context()
{
    ImGui::DestroyContext();
}

Application::Application()
{
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(1280, 720, "Vulkan engine", nullptr, nullptr);

    glfwSetWindowUserPointer(m_window, this);

    glfwSetKeyCallback(m_window, key_callback);
    glfwSetFramebufferSizeCallback(m_window, framebuffer_size_callback);

    int width {};
    int height {};
    glfwGetFramebufferSize(m_window, &width, &height);

    m_renderer = std::make_unique<Renderer>(m_window,
                                            static_cast<std::uint32_t>(width),
                                            static_cast<std::uint32_t>(height));
}

Application::~Application()
{
    glfwDestroyWindow(m_window);
}

void Application::run()
{
    int frames {};
    const auto start_time = glfwGetTime();
    auto last_frame_time = start_time;

    while (!glfwWindowShouldClose(m_window))
    {
        glfwPollEvents();

        const auto now = glfwGetTime();
        const auto delta_time = now - last_frame_time;
        last_frame_time = now;
        const auto total_time = now - start_time;

        double x, y;
        glfwGetCursorPos(m_window, &x, &y);

        m_renderer->draw_frame(static_cast<float>(total_time),
                               static_cast<float>(delta_time),
                               {static_cast<float>(x), static_cast<float>(y)});

        ++frames;
    }
}

void Application::key_callback(
    GLFWwindow *window, int key, int /*scancode*/, int action, int /*mods*/)
{
    if (action == GLFW_PRESS && key == GLFW_KEY_F)
    {
        auto app = static_cast<Application *>(glfwGetWindowUserPointer(window));
        if (app->is_fullscreen())
            app->set_windowed();
        else
            app->set_fullscreen();
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

bool Application::is_fullscreen()
{
    return glfwGetWindowMonitor(m_window) != nullptr;
}

void Application::set_fullscreen()
{
    auto *const monitor = glfwGetPrimaryMonitor();
    const auto *const video_mode = glfwGetVideoMode(monitor);
    glfwSetWindowMonitor(m_window,
                         monitor,
                         0,
                         0,
                         video_mode->width,
                         video_mode->height,
                         GLFW_DONT_CARE);
}

void Application::set_windowed()
{
    auto *const monitor = glfwGetPrimaryMonitor();
    const auto *const video_mode = glfwGetVideoMode(monitor);
    constexpr int width {1280};
    constexpr int height {720};
    glfwSetWindowMonitor(m_window,
                         nullptr,
                         (video_mode->width - width) / 2,
                         (video_mode->height - height) / 2,
                         width,
                         height,
                         GLFW_DONT_CARE);
}