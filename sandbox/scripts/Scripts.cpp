// Registers all sandbox scripts with the engine (ScriptRegistry + serializer).
// This is the one sandbox TU that pulls in yaml-cpp (via RegisterScript.h).
#include "ember/scripting/RegisterScript.h"

#include "scripts/Spinner.h"
#include "scripts/Lifetime.h"
#include "scripts/PlayerController.h"
#include "scripts/CameraFollow.h"

using namespace game;

EMBER_REGISTER_SCRIPT(Spinner)
EMBER_REGISTER_SCRIPT(Lifetime)
EMBER_REGISTER_SCRIPT(PlayerController)
EMBER_REGISTER_SCRIPT(CameraFollow)

// Player logic should run before the camera that follows it.
EMBER_SCRIPT_ORDER(PlayerController, -10)
EMBER_SCRIPT_ORDER(CameraFollow, 10)
