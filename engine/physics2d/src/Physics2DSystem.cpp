#include "ember/physics2d/Physics2DSystem.h"
#include "ember/physics2d/RigidBody2D.h"
#include "ember/physics2d/Colliders2D.h"
#include "ember/scripting/ScriptSystem.h"   // dispatchCollision + ContactPhase
#include "ember/ecs/World.h"
#include "ember/ecs/Components.h"   // Transform

#include <algorithm>
#include <cmath>

namespace ember {

namespace {

// Flattened collider used during a step / query.
struct Col {
    Entity     e         = NULL_ENTITY;
    bool       isCircle  = false;
    glm::vec2  center{0.0f};
    glm::vec2  halfExtents{0.0f};   // box
    f32        radius    = 0.0f;    // circle
    f32        invMass   = 0.0f;
    glm::vec2* velocity  = nullptr; // into RigidBody2D, null if static-only
    glm::vec3* position  = nullptr; // into Transform, for write-back
    f32        restitution = 0.0f;
    f32        friction    = 0.0f;
    bool       isTrigger   = false;
};

struct Manifold { bool hit = false; glm::vec2 normal{0.0f}; f32 depth = 0.0f; };

glm::vec2 xy(const glm::vec3& v) { return {v.x, v.y}; }

// Min/max AABB of a collider (for broadphase reject + box math).
void aabb(const Col& c, glm::vec2& mn, glm::vec2& mx) {
    if (c.isCircle) { mn = c.center - glm::vec2(c.radius); mx = c.center + glm::vec2(c.radius); }
    else            { mn = c.center - c.halfExtents;       mx = c.center + c.halfExtents; }
}

Manifold collideBoxBox(const Col& a, const Col& b) {
    const glm::vec2 d = b.center - a.center;
    const glm::vec2 overlap = (a.halfExtents + b.halfExtents) - glm::abs(d);
    Manifold m;
    if (overlap.x <= 0.0f || overlap.y <= 0.0f) return m;
    m.hit = true;
    if (overlap.x < overlap.y) { m.normal = {d.x < 0 ? -1.0f : 1.0f, 0.0f}; m.depth = overlap.x; }
    else                       { m.normal = {0.0f, d.y < 0 ? -1.0f : 1.0f}; m.depth = overlap.y; }
    return m;
}

Manifold collideCircleCircle(const Col& a, const Col& b) {
    const glm::vec2 d = b.center - a.center;
    const f32 r = a.radius + b.radius;
    const f32 d2 = glm::dot(d, d);
    Manifold m;
    if (d2 >= r * r) return m;
    const f32 dist = std::sqrt(d2);
    m.hit = true;
    m.normal = dist > 1e-6f ? d / dist : glm::vec2(1.0f, 0.0f);
    m.depth  = r - dist;
    return m;
}

// Circle (a) vs Box (b); normal points from box toward circle, then flipped to A->B.
Manifold collideCircleBox(const Col& circle, const Col& box) {
    const glm::vec2 d = circle.center - box.center;
    const glm::vec2 closest = box.center + glm::clamp(d, -box.halfExtents, box.halfExtents);
    const glm::vec2 diff = circle.center - closest;
    const f32 d2 = glm::dot(diff, diff);
    Manifold m;
    if (d2 >= circle.radius * circle.radius) return m;
    const f32 dist = std::sqrt(d2);
    m.hit = true;
    glm::vec2 n = dist > 1e-6f ? diff / dist : glm::vec2(0.0f, 1.0f);  // box -> circle
    m.normal = -n;                                                     // A(circle) -> B(box)
    m.depth  = circle.radius - dist;
    return m;
}

// Manifold with normal pointing A -> B.
Manifold narrowphase(const Col& a, const Col& b) {
    if (!a.isCircle && !b.isCircle) return collideBoxBox(a, b);
    if (a.isCircle && b.isCircle)   return collideCircleCircle(a, b);
    if (a.isCircle && !b.isCircle)  return collideCircleBox(a, b);          // A->B already
    // box(a) vs circle(b): compute circle->box then flip to A(box)->B(circle)
    Manifold m = collideCircleBox(b, a);  // normal B->A
    m.normal = -m.normal;                 // A->B
    return m;
}

void buildColliders(World& world, std::vector<Col>& out) {
    auto add = [&](Entity e, Transform& t, bool circle, const glm::vec2& he, f32 rad,
                   const glm::vec2& offset, const PhysicsMaterial2D& mat, bool trigger) {
        Col c;
        c.e = e; c.isCircle = circle;
        c.center = xy(t.position) + offset;
        c.halfExtents = he * glm::vec2(t.scale.x, t.scale.y);
        c.radius = rad * std::max(t.scale.x, t.scale.y);
        c.position = &t.position;
        if (auto* rb = world.tryGet<RigidBody2D>(e)) {
            c.invMass = rb->invMass();
            c.velocity = &rb->velocity;
        }
        c.restitution = mat.restitution;
        c.friction    = mat.friction;
        c.isTrigger   = trigger;
        out.push_back(c);
    };
    for (auto [e, t, b] : world.view<Transform, BoxCollider2D>())
        add(e, t, false, b.halfExtents, 0.0f, b.offset, b.material, b.isTrigger);
    for (auto [e, t, c] : world.view<Transform, CircleCollider2D>())
        add(e, t, true, glm::vec2(0.0f), c.radius, c.offset, c.material, c.isTrigger);
}

void resolve(Col& a, Col& b, const Manifold& m) {
    const f32 invSum = a.invMass + b.invMass;
    if (invSum <= 0.0f) return;   // both immovable

    const glm::vec2 va = a.velocity ? *a.velocity : glm::vec2(0.0f);
    const glm::vec2 vb = b.velocity ? *b.velocity : glm::vec2(0.0f);
    const glm::vec2 rv = vb - va;
    const f32 vn = glm::dot(rv, m.normal);

    if (vn < 0.0f) {   // approaching
        const f32 e = std::min(a.restitution, b.restitution);
        const f32 j = -(1.0f + e) * vn / invSum;
        const glm::vec2 impulse = j * m.normal;
        if (a.velocity) *a.velocity -= impulse * a.invMass;
        if (b.velocity) *b.velocity += impulse * b.invMass;

        // Coulomb friction along the tangent.
        const glm::vec2 va2 = a.velocity ? *a.velocity : glm::vec2(0.0f);
        const glm::vec2 vb2 = b.velocity ? *b.velocity : glm::vec2(0.0f);
        glm::vec2 t = (vb2 - va2) - glm::dot(vb2 - va2, m.normal) * m.normal;
        const f32 tl = glm::length(t);
        if (tl > 1e-5f) {
            t /= tl;
            f32 jt = -glm::dot(vb2 - va2, t) / invSum;
            const f32 mu = std::sqrt(a.friction * b.friction);
            jt = std::clamp(jt, -j * mu, j * mu);
            const glm::vec2 fImp = jt * t;
            if (a.velocity) *a.velocity -= fImp * a.invMass;
            if (b.velocity) *b.velocity += fImp * b.invMass;
        }
    }

    // Baumgarte positional correction to remove penetration.
    const f32 slop = 0.005f, percent = 0.8f;
    const f32 corr = std::max(m.depth - slop, 0.0f) / invSum * percent;
    const glm::vec2 c = corr * m.normal;
    if (a.position) { a.position->x -= c.x * a.invMass; a.position->y -= c.y * a.invMass; }
    if (b.position) { b.position->x += c.x * b.invMass; b.position->y += c.y * b.invMass; }
}

} // namespace

void Physics2DSystem::step(World& world, f32 h) {
    // 1. Integrate.
    for (auto [e, t, rb] : world.view<Transform, RigidBody2D>()) {
        if (rb.type == BodyType2D::Static) { rb.force = glm::vec2(0.0f); continue; }
        if (rb.type == BodyType2D::Dynamic) {
            rb.velocity += m_gravity * rb.gravityScale * h;
            rb.velocity += rb.force * rb.invMass() * h;
            rb.velocity *= 1.0f / (1.0f + rb.linearDamping * h);
        }
        t.position.x += rb.velocity.x * h;
        t.position.y += rb.velocity.y * h;
        rb.force = glm::vec2(0.0f);
    }

    // 2. Collisions (O(n^2) broadphase with AABB reject; spatial hash is a future opt).
    std::vector<Col> cols;
    buildColliders(world, cols);
    std::map<std::pair<u64, u64>, ContactInfo> current;   // this step's contacts
    for (usize i = 0; i < cols.size(); ++i) {
        glm::vec2 amn, amx; aabb(cols[i], amn, amx);
        for (usize j = i + 1; j < cols.size(); ++j) {
            glm::vec2 bmn, bmx; aabb(cols[j], bmn, bmx);
            if (amx.x < bmn.x || bmx.x < amn.x || amx.y < bmn.y || bmx.y < amn.y) continue;
            const Manifold m = narrowphase(cols[i], cols[j]);
            if (!m.hit) continue;
            const bool trigger = cols[i].isTrigger || cols[j].isTrigger;
            if (m_scripts) {
                const u64 ia = static_cast<u64>(entt::to_integral(cols[i].e));
                const u64 ib = static_cast<u64>(entt::to_integral(cols[j].e));
                const auto key = ia < ib ? std::make_pair(ia, ib) : std::make_pair(ib, ia);
                current[key] = ContactInfo{ cols[i].e, cols[j].e, trigger };
            }
            if (trigger) continue;   // triggers: events only, no physical response
            resolve(cols[i], cols[j], m);
        }
    }

    // 3. Derive enter/stay/exit vs the previous step and fire script callbacks.
    if (m_scripts) {
        for (const auto& [k, c] : current) {
            const ContactPhase ph = m_prevContacts.count(k) ? ContactPhase::Stay : ContactPhase::Enter;
            m_scripts->dispatchCollision(c.a, c.b, ph, c.trigger);
            m_scripts->dispatchCollision(c.b, c.a, ph, c.trigger);
        }
        for (const auto& [k, c] : m_prevContacts) {
            if (current.count(k)) continue;
            m_scripts->dispatchCollision(c.a, c.b, ContactPhase::Exit, c.trigger);
            m_scripts->dispatchCollision(c.b, c.a, ContactPhase::Exit, c.trigger);
        }
        m_prevContacts = std::move(current);
    }
}

void Physics2DSystem::update(World& world, f32 dt) {
    m_accum += dt;
    int guard = 0;
    while (m_accum >= m_fixedDelta && guard++ < 8) {   // clamp catch-up to avoid spirals
        step(world, m_fixedDelta);
        m_accum -= m_fixedDelta;
    }
}

RaycastHit2D Physics2DSystem::raycast(World& world, const glm::vec2& origin,
                                      const glm::vec2& dir, f32 maxDist) const {
    std::vector<Col> cols;
    buildColliders(world, cols);
    const glm::vec2 d = glm::length(dir) > 1e-6f ? glm::normalize(dir) : glm::vec2(1.0f, 0.0f);
    RaycastHit2D best;
    best.distance = maxDist;
    for (const Col& c : cols) {
        f32 tHit;
        glm::vec2 n;
        bool hit = false;
        if (c.isCircle) {
            const glm::vec2 m = origin - c.center;
            const f32 b = glm::dot(m, d);
            const f32 cc = glm::dot(m, m) - c.radius * c.radius;
            const f32 disc = b * b - cc;
            if (disc >= 0.0f) { const f32 t = -b - std::sqrt(disc); if (t >= 0.0f) { tHit = t; n = glm::normalize(origin + d * t - c.center); hit = true; } }
        } else {
            glm::vec2 mn, mx; aabb(c, mn, mx);
            f32 tmin = 0.0f, tmax = maxDist; n = {0, 0};
            bool ok = true;
            for (int a = 0; a < 2; ++a) {
                const f32 o = origin[a], dd = d[a], lo = mn[a], hi = mx[a];
                if (std::abs(dd) < 1e-8f) { if (o < lo || o > hi) { ok = false; break; } }
                else {
                    f32 t1 = (lo - o) / dd, t2 = (hi - o) / dd; f32 sign = -1.0f;
                    if (t1 > t2) { std::swap(t1, t2); sign = 1.0f; }
                    if (t1 > tmin) { tmin = t1; n = {0, 0}; n[a] = sign; }
                    tmax = std::min(tmax, t2);
                    if (tmin > tmax) { ok = false; break; }
                }
            }
            if (ok) { tHit = tmin; hit = true; }
        }
        if (hit && tHit >= 0.0f && tHit <= best.distance) {
            best.hit = true; best.entity = c.e; best.distance = tHit;
            best.point = origin + d * tHit; best.normal = n;
        }
    }
    return best;
}

std::vector<Entity> Physics2DSystem::overlapBox(World& world, const glm::vec2& center,
                                                const glm::vec2& halfExtents) const {
    Col q; q.isCircle = false; q.center = center; q.halfExtents = halfExtents;
    std::vector<Col> cols; buildColliders(world, cols);
    std::vector<Entity> out;
    for (const Col& c : cols) if (narrowphase(q, c).hit) out.push_back(c.e);
    return out;
}

std::vector<Entity> Physics2DSystem::overlapCircle(World& world, const glm::vec2& center,
                                                   f32 radius) const {
    Col q; q.isCircle = true; q.center = center; q.radius = radius;
    std::vector<Col> cols; buildColliders(world, cols);
    std::vector<Entity> out;
    for (const Col& c : cols) if (narrowphase(q, c).hit) out.push_back(c.e);
    return out;
}

} // namespace ember
