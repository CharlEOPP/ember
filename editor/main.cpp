#include "EditorApplication.h"
#include "ember/core/Log.h"

int main() {
    ember::Log::init();
    {
        ember::EditorApplication app;
        app.run();
    }
    ember::Log::shutdown();
    return 0;
}
