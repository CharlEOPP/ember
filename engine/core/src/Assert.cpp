#include "ember/core/Assert.h"
#include <cstdlib>

namespace ember::Assert {

static Handler s_handler;

static void defaultHandler([[maybe_unused]] const char* expr,
                           [[maybe_unused]] const char* file,
                           [[maybe_unused]] int         line,
                           [[maybe_unused]] const char* msg)
{
#if !defined(NDEBUG)
    EMBER_DEBUGBREAK();
#else
    std::terminate();
#endif
}

void setHandler(Handler handler) { s_handler = std::move(handler); }
void resetHandler()               { s_handler = {}; }

void invoke(const char* expr, const char* file, int line, const char* msg) {
    if (s_handler)
        s_handler(expr, file, line, msg);
    else
        defaultHandler(expr, file, line, msg);
}

} // namespace ember::Assert
