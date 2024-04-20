#pragma once

#include "Fyrion/Common.hpp"
#include "Fyrion/Core/Array.hpp"
#include "Fyrion/Core/HashMap.hpp"
#include "Fyrion/Core/HashSet.hpp"
#include "Fyrion/Core/String.hpp"
#include "Fyrion/Core/UniquePtr.hpp"
#include "Fyrion/Core/UUID.hpp"
#include "Fyrion/Resource/ResourceTypes.hpp"

namespace Fyrion
{
    struct SceneObjectNode
    {
        RID                     rid{};
        UUID                    uuid{};
        String                  name{};
        SceneObjectNode*        parent{};
        Array<SceneObjectNode*> children{};
        bool                    selected{};
        u64                     order{U64_MAX};
    };

    class FY_API SceneEditor final
    {
    public:
        SceneEditor();
        ~SceneEditor();
        SceneEditor(const SceneEditor& other) = delete;
        SceneEditor(SceneEditor&& other) noexcept = default;

        void             LoadScene(RID rid);
        bool             IsLoaded() const;
        SceneObjectNode* GetRootNode() const;
        SceneObjectNode* FindNodeByRID(RID rid) const;
        void             CreateObject();
        void             DestroySelectedObjects();
        void             ClearSelection();
        void             SelectObject(SceneObjectNode* node);
        bool             IsParentOfSelected(SceneObjectNode* node) const;
        bool             IsSimulating();
        SceneObjectNode* GetLastSelectedObject() const;
    private:
        SceneObjectNode*                         m_rootNode{};
        HashMap<RID, UniquePtr<SceneObjectNode>> m_nodes{};
        HashSet<RID>                             m_selectedObjects{};
        SceneObjectNode*                         m_lastSelectedObject{};
        u64                                      m_count{};

        SceneObjectNode* LoadSceneObjectAsset(RID rid);
        void UpdateSceneObjectNode(SceneObjectNode& node, ResourceObject& object, bool updateChildren = true);
        static void SceneObjectAssetChanged(VoidPtr userData, ResourceEventType eventType, ResourceObject& oldObject, ResourceObject& newObject);
    };
}