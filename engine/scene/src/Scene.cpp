#include "ember/scene/Scene.h"
#include "ember/core/Assert.h"
#include "ember/scene/TransformSystem.h"   // resolveWorldTransform (now public)

#include <algorithm>
#include <utility>

namespace ember {

Entity Scene::create(std::string name) {
    Entity e = m_world.create();
    Tag& tag = m_world.emplace<Tag>(e);
    tag.name = std::move(name);
    m_world.emplace<Transform>(e);
    m_world.emplace<WorldTransform>(e);
    return e;
}

void Scene::destroy(Entity e) {
    // Detach from parent's child list.
    if (Parent* p = m_world.tryGet<Parent>(e); p && p->parent != NULL_ENTITY) {
        if (Children* siblings = m_world.tryGet<Children>(p->parent))
            std::erase(siblings->children, e);
    }
    // Orphan any children.
    if (Children* ch = m_world.tryGet<Children>(e)) {
        for (Entity c : ch->children)
            if (Parent* cp = m_world.tryGet<Parent>(c)) cp->parent = NULL_ENTITY;
    }
    m_world.destroy(e);
}

Entity Scene::findByName(std::string_view name) {
    for (auto [e, tag] : m_world.view<Tag>())
        if (tag.name == name) return e;
    return NULL_ENTITY;
}

std::vector<Entity> Scene::findByTag(std::string_view layer) {
    std::vector<Entity> out;
    for (auto [e, tag] : m_world.view<Tag>())
        if (tag.layer == layer) out.push_back(e);
    return out;
}

bool Scene::isAncestor(Entity ancestor, Entity of) {
    Entity cur = getParent(of);
    while (cur != NULL_ENTITY) {
        if (cur == ancestor) return true;
        cur = getParent(cur);
    }
    return false;
}

Entity Scene::getParent(Entity child) {
    const Parent* p = m_world.tryGet<Parent>(child);
    return p ? p->parent : NULL_ENTITY;
}

void Scene::setParent(Entity child, Entity parent) {
    EMBER_ASSERT(!isAncestor(child, parent), "Scene::setParent() would create a cycle");
    if (isAncestor(child, parent)) return;

    // Remove from previous parent's child list.
    const Entity old = getParent(child);
    if (old != NULL_ENTITY) {
        if (Children* ch = m_world.tryGet<Children>(old))
            std::erase(ch->children, child);
    }

    // Set the new parent.
    Parent* p = m_world.tryGet<Parent>(child);
    if (!p) p = &m_world.emplace<Parent>(child);
    p->parent = parent;

    // Add to new parent's child list.
    if (parent != NULL_ENTITY) {
        Children* ch = m_world.tryGet<Children>(parent);
        if (!ch) ch = &m_world.emplace<Children>(parent);
        ch->children.push_back(child);
    }

    markTransformDirty(child);
}

void Scene::markTransformDirty(Entity e) {
    if (WorldTransform* wt = m_world.tryGet<WorldTransform>(e)) wt->dirty = true;
    if (Children* ch = m_world.tryGet<Children>(e))
        for (Entity c : ch->children) markTransformDirty(c);
}

glm::mat4 Scene::getWorldTransform(Entity e) {
    return resolveWorldTransform(m_world, e);
}

} // namespace ember
