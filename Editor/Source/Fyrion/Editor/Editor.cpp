#include "Editor.hpp"
#include "Fyrion/Core/Event.hpp"
#include "Fyrion/Engine.hpp"
#include "Fyrion/ImGui/ImGui.hpp"
#include "Fyrion/ImGui/Lib/imgui_internal.h"

namespace Fyrion
{

    namespace
    {
//        Array<EditorWindow>         EditorWindows;
//        Array<OpenWindowStorage>    OpenWindows;
//        EventHandler<OnEditorStart> OnEditorStartEvent{};
//        MenuItemContext* MenuContext{};

        bool dockInitialized = false;
        u32  dockSpaceId{10000};
        u32  centerSpaceId{10000};
        u32  topRightDockId{};
        u32  bottomRightDockId{};
        u32  bottomDockId{};
        u32  leftDockId{};
        u32  idCounter{100000};
        bool showImGuiDemo = false;
    }

    void InitProjectBrowser();
    void InitDockSpace();
    void InitProject();
    void InitWorldView();
    void InitEntityTreeWindow();
    void InitPropertiesWindow();

    void EditorUpdate(f64 deltaTime)
    {
        ImGuiStyle& style = ImGui::GetStyle();
        ImGui::CreateDockSpace(dockSpaceId);
        InitDockSpace();
//        DrawOpenWindows();
//
//        if (ShowImGuiDemo)
//        {
//            ImGui::ShowDemoWindow(&ShowImGuiDemo);
//        }
//
//        DrawMenu();
        ImGui::End();
    }


    void InitDockSpace()
    {
        if (!dockInitialized)
        {
            dockInitialized = true;
            ImGui::DockBuilderReset(dockSpaceId);

            //create default windows
            centerSpaceId     = dockSpaceId;
            topRightDockId    = ImGui::DockBuilderSplitNode(centerSpaceId, ImGuiDir_Right, 0.15f, nullptr, &centerSpaceId);
            bottomRightDockId = ImGui::DockBuilderSplitNode(topRightDockId, ImGuiDir_Down, 0.50f, nullptr, &topRightDockId);
            bottomDockId      = ImGui::DockBuilderSplitNode(centerSpaceId, ImGuiDir_Down, 0.20f, nullptr, &centerSpaceId);
            leftDockId        = ImGui::DockBuilderSplitNode(centerSpaceId, ImGuiDir_Left, 0.12f, nullptr, &centerSpaceId);

//            for (const auto& windowType: EditorWindows)
//            {
//                auto p = GetDockId(windowType.InitialDockPosition);
//                if (p != U32_MAX)
//                {
//                    ImGui::DockBuilderDockWindow(CreateWindow(windowType, nullptr), p);
//                }
//            }
        }
    }


    void Editor::Init()
    {
        Event::Bind<OnUpdate, &EditorUpdate>();
    }
}
