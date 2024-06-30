#pragma once

#include "Fyrion/Common.hpp"

namespace Fyrion
{
    class SceneObject;
}

namespace Fyrion::SceneManager
{
    FY_API void         Destroy(SceneObject* sceneObject);
    FY_API void         SetActiveObject(SceneObject* sceneObject);
    FY_API SceneObject* CreateObject();
}
