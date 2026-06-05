#include "ImGuiLayer.h"
#include "ember/platform/Window.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#ifdef EMBER_HAS_IMGUIZMO
#include <ImGuizmo.h>
#endif
#include <GLFW/glfw3.h>

#include <filesystem>

namespace ember {

void ImGuiLayer::init(Window& window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.IniFilename  = "editor/layout.ini";   // persisted dockspace layout

    // Scale the UI to the monitor's DPI so text stays readable at high resolution
    // / fullscreen. Font is rasterized at the scaled size for crispness; widget
    // metrics are scaled to match.
    float xscale = 1.0f, yscale = 1.0f;
    glfwGetWindowContentScale(window.getNativeHandle(), &xscale, &yscale);
    const float dpi = (xscale > 0.0f) ? xscale : 1.0f;
    const float fontSize = 16.0f * dpi;   // base 16px (up from 14) × DPI

    if (std::filesystem::exists("assets/fonts/JetBrainsMono-Regular.ttf"))
        io.Fonts->AddFontFromFileTTF("assets/fonts/JetBrainsMono-Regular.ttf", fontSize);
    else
        io.FontGlobalScale = dpi;   // built-in font: scale up instead

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();

    // Light cleanup pass: rounded corners + a bit more breathing room.
    style.FrameRounding     = 4.0f;
    style.GrabRounding      = 4.0f;
    style.TabRounding       = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.WindowRounding    = 6.0f;
    style.FramePadding      = ImVec2(6.0f, 4.0f);
    style.ItemSpacing       = ImVec2(8.0f, 6.0f);
    style.WindowPadding     = ImVec2(8.0f, 8.0f);
    style.ScaleAllSizes(dpi);   // DPI-scale all of the above

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding              = 0.0f;   // OS-window children need square corners
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplGlfw_InitForOpenGL(window.getNativeHandle(), true);
    ImGui_ImplOpenGL3_Init("#version 410");
    m_initialized = true;
}

void ImGuiLayer::setUiScale(float scale) {
    ImGui::GetIO().FontGlobalScale = (scale > 0.0f) ? scale : 1.0f;
}

void ImGuiLayer::begin() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
#ifdef EMBER_HAS_IMGUIZMO
    ImGuizmo::BeginFrame();
#endif
}

void ImGuiLayer::end(Window& window) {
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(window.getWidth()),
                            static_cast<float>(window.getHeight()));

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backup = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup);
    }
}

void ImGuiLayer::applyTheme(int theme) {
    if (theme == 1)      ImGui::StyleColorsLight();
    else if (theme == 2) ImGui::StyleColorsClassic();
    else                 ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding              = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
}

void ImGuiLayer::shutdown() {
    if (!m_initialized) return;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    m_initialized = false;
}

} // namespace ember
