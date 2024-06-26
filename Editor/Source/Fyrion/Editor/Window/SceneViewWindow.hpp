#pragma once
#include "Fyrion/Core/Registry.hpp"
#include "Fyrion/Editor/EditorTypes.hpp"
#include "Fyrion/Graphics/RenderGraph.hpp"

namespace Fyrion
{
    struct MenuItemEventData;
    class SceneEditor;

    class SceneViewWindow : public EditorWindow
    {
    public:
        SceneViewWindow();

        void Draw(u32 id, bool& open) override;

        static void RegisterType(NativeTypeHandler<SceneViewWindow>& type);
    private:
        SceneEditor&     m_sceneEditor;
        u32              m_guizmoOperation{};
        bool             m_windowStartedSimulation{};
        bool             m_movingScene{};
        RenderGraph*      m_renderGraph{};

        static void OpenSceneView(const MenuItemEventData& eventData);
    };
}
