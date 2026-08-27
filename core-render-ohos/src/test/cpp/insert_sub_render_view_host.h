#ifndef CORE_RENDER_OHOS_TEST_INSERT_SUB_RENDER_VIEW_HOST_H
#define CORE_RENDER_OHOS_TEST_INSERT_SUB_RENDER_VIEW_HOST_H

#include <memory>
#include <unordered_map>

// Extracted leftover KRRenderLayerHandler::InsertSubRenderView +
// KRRenderView::AddContentView (root path).
//
// Leftover:
//   view_registry_[child_tag] default-inserts nullptr
//   Root (parent_tag==-1) calls AddContentView with no child_view check
//   AddContentView only checks root_node_, then contentView->GetNode()
//
// Production (no ArkUI / Harmony):
//   find() not operator[] for child and parent
//   !child_view => skip (root and non-root)
//   AddContentView(nullptr) returns before GetNode()
//
// Header-only so host leftover tests compile without Harmony.

struct InsertHostView {
    int tag = 0;
};

using InsertHostMap = std::unordered_map<int, std::shared_ptr<InsertHostView>>;

struct InsertResult {
    bool called_add_content = false;
    bool called_insert_sub = false;
    std::shared_ptr<InsertHostView> added;
};

inline InsertResult InsertSubRenderView(InsertHostMap &view_registry, int parent_tag, int child_tag) {
    InsertResult r;
    auto child_it = view_registry.find(child_tag);
    if (child_it == view_registry.end() || child_it->second == nullptr) {
        return r;
    }
    auto &child_view = child_it->second;
    if (parent_tag == -1) {
        r.called_add_content = true;
        r.added = child_view;
        return r;
    }
    auto parent_it = view_registry.find(parent_tag);
    if (parent_it == view_registry.end() || parent_it->second == nullptr) {
        return r;
    }
    r.called_insert_sub = true;
    r.added = child_view;
    return r;
}

inline bool AddContentView(void *root_node, const std::shared_ptr<InsertHostView> &contentView) {
    if (root_node == nullptr || contentView == nullptr) {
        return false;
    }
    return true;
}

#endif  // CORE_RENDER_OHOS_TEST_INSERT_SUB_RENDER_VIEW_HOST_H
