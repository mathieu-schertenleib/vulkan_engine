#include "application.hpp"

#include "imgui.h"

#include <iostream>
#include <stdexcept>

namespace
{

[[noreturn]] void glfw_fatal(int error, const char *description)
{
    std::ostringstream oss;
    oss << "GLFW error " << error << ": " << description << '\n';
    throw std::runtime_error(oss.str());
}

} // namespace

Application::Glfw_context::Glfw_context()
{
    glfwSetErrorCallback(&glfw_fatal);
    glfwInit();
}

Application::Glfw_context::~Glfw_context()
{
    glfwTerminate();
}

Application::ImGui_context::ImGui_context()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
}

Application::ImGui_context::~ImGui_context()
{
    ImGui::DestroyContext();
}

Application::Application()
{
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)>(
        glfwCreateWindow(1280, 720, "Vulkan engine", nullptr, nullptr),
        &glfwDestroyWindow);

    glfwSetWindowUserPointer(m_window.get(), this);

    glfwSetKeyCallback(m_window.get(), key_callback);
    glfwSetFramebufferSizeCallback(m_window.get(), framebuffer_size_callback);

    int width {};
    int height {};
    glfwGetFramebufferSize(m_window.get(), &width, &height);

    m_renderer = std::make_unique<Renderer>(m_window.get(),
                                            static_cast<std::uint32_t>(width),
                                            static_cast<std::uint32_t>(height));
}

void Application::run()
{
    const auto start_time = glfwGetTime();

    while (!glfwWindowShouldClose(m_window.get()))
    {
        glfwPollEvents();

        const auto now = glfwGetTime();
        const auto total_time = now - start_time;

        double x, y;
        glfwGetCursorPos(m_window.get(), &x, &y);

        m_renderer->draw_frame(static_cast<float>(total_time),
                               {static_cast<float>(x), static_cast<float>(y)});
    }
}

void Application::key_callback(
    GLFWwindow *window, int key, int /*scancode*/, int action, int /*mods*/)
{
    if (action == GLFW_PRESS && key == GLFW_KEY_F)
    {
        const auto app =
            static_cast<Application *>(glfwGetWindowUserPointer(window));
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
    const auto app =
        static_cast<Application *>(glfwGetWindowUserPointer(window));
    app->m_renderer->resize_framebuffer(static_cast<std::uint32_t>(width),
                                        static_cast<std::uint32_t>(height));
}

bool Application::is_fullscreen()
{
    return glfwGetWindowMonitor(m_window.get()) != nullptr;
}

void Application::set_fullscreen()
{
    const auto monitor = glfwGetPrimaryMonitor();
    const auto video_mode = glfwGetVideoMode(monitor);
    glfwSetWindowMonitor(m_window.get(),
                         monitor,
                         0,
                         0,
                         video_mode->width,
                         video_mode->height,
                         GLFW_DONT_CARE);
}

void Application::set_windowed()
{
    const auto monitor = glfwGetPrimaryMonitor();
    const auto video_mode = glfwGetVideoMode(monitor);
    constexpr int width {1280};
    constexpr int height {720};
    glfwSetWindowMonitor(m_window.get(),
                         nullptr,
                         (video_mode->width - width) / 2,
                         (video_mode->height - height) / 2,
                         width,
                         height,
                         GLFW_DONT_CARE);
}