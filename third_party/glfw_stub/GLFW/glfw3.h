/**
 * Minimal GLFW3 stub for offline / no-display builds.
 * Provides declarations only -- no implementations.
 * The real GLFW is fetched via FetchContent in normal builds.
 */
#pragma once

/* Opaque handle types */
typedef struct GLFWwindow  GLFWwindow;
typedef struct GLFWmonitor GLFWmonitor;

/* Error callback */
typedef void (*GLFWerrorfun)(int error_code, const char* description);
/* Window callbacks */
typedef void (*GLFWwindowsizefun)(GLFWwindow* window, int width, int height);
typedef void (*GLFWwindowclosefun)(GLFWwindow* window);
/* Proc address type */
typedef void (*GLFWglproc)(void);

/* Window hints */
#define GLFW_CONTEXT_VERSION_MAJOR  0x00022002
#define GLFW_CONTEXT_VERSION_MINOR  0x00022003
#define GLFW_OPENGL_PROFILE         0x00022008
#define GLFW_OPENGL_CORE_PROFILE    0x00032001
#define GLFW_OPENGL_FORWARD_COMPAT  0x00022006
#define GLFW_TRUE                   1
#define GLFW_FALSE                  0

#ifdef __cplusplus
extern "C" {
#endif

int            glfwInit(void);
void           glfwTerminate(void);
GLFWerrorfun   glfwSetErrorCallback(GLFWerrorfun callback);
void           glfwWindowHint(int hint, int value);
GLFWwindow*    glfwCreateWindow(int width, int height, const char* title,
                                GLFWmonitor* monitor, GLFWwindow* share);
void           glfwDestroyWindow(GLFWwindow* window);
void           glfwMakeContextCurrent(GLFWwindow* window);
void           glfwSwapInterval(int interval);
void           glfwSwapBuffers(GLFWwindow* window);
void           glfwPollEvents(void);
int            glfwWindowShouldClose(GLFWwindow* window);
void           glfwSetWindowTitle(GLFWwindow* window, const char* title);
void           glfwSetWindowUserPointer(GLFWwindow* window, void* pointer);
void*          glfwGetWindowUserPointer(GLFWwindow* window);
GLFWmonitor*   glfwGetPrimaryMonitor(void);
GLFWglproc     glfwGetProcAddress(const char* procname);

GLFWwindowsizefun  glfwSetWindowSizeCallback(GLFWwindow* window, GLFWwindowsizefun callback);
GLFWwindowclosefun glfwSetWindowCloseCallback(GLFWwindow* window, GLFWwindowclosefun callback);

#ifdef __cplusplus
}
#endif
