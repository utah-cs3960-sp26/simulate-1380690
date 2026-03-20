#include "simulation.h"
#include <algorithm>
#include <random>
#include <cmath>

static float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

// Point-to-segment closest point
static Vec2 closest_point_on_segment(Vec2 p, Vec2 a, Vec2 b) {
    Vec2 ab = b - a;
    float t = ab.dot(ab);
    if (t < 1e-8f) return a;
    t = clampf((p - a).dot(ab) / t, 0.0f, 1.0f);
    return a + ab * t;
}

void World::init(int width, int height) {
    world_w = width;
    world_h = height;
    balls.clear();
    walls.clear();

    float margin = 30.0f;
    float left = margin, right = width - margin;
    float top = margin, bottom = height - margin;

    // 4 container walls — normals point inward
    // Bottom
    walls.push_back({{left, bottom}, {right, bottom}, {0, -1}});
    // Top
    walls.push_back({{left, top}, {right, top}, {0, 1}});
    // Left
    walls.push_back({{left, top}, {left, bottom}, {1, 0}});
    // Right
    walls.push_back({{right, top}, {right, bottom}, {-1, 0}});

    // Funnel: two angled walls in the upper-middle area
    float cx = width / 2.0f;
    float funnel_top = height * 0.35f;
    float funnel_gap = 60.0f;
    walls.push_back({{left + 40, funnel_top - 80}, {cx - funnel_gap, funnel_top}, {0.4f, -0.916f}}); // left funnel
    walls.push_back({{right - 40, funnel_top - 80}, {cx + funnel_gap, funnel_top}, {-0.4f, -0.916f}}); // right funnel

    // Recompute wall normals properly (perpendicular to wall, pointing "up" or "inward")
    for (auto& w : walls) {
        Vec2 edge = w.b - w.a;
        // Two candidate normals
        Vec2 n1 = Vec2{-edge.y, edge.x}.normalized();
        Vec2 n2 = Vec2{edge.y, -edge.x}.normalized();
        // Pick the one pointing toward center of screen
        Vec2 mid = (w.a + w.b) * 0.5f;
        Vec2 to_center = Vec2{(float)width / 2, (float)height / 2} - mid;
        w.normal = (n1.dot(to_center) > n2.dot(to_center)) ? n1 : n2;
    }

    // Spawn ~1000 balls in a grid above the funnel
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> radius_dist(4.0f, 7.0f);
    std::uniform_real_distribution<float> jitter(-0.5f, 0.5f);

    float spawn_left = left + 20;
    float spawn_right = right - 20;
    float spacing = 12.0f;
    int cols = (int)((spawn_right - spawn_left) / spacing);

    int count = 0;
    int target = 1000;
    for (int row = 0; count < target; ++row) {
        for (int col = 0; col < cols && count < target; ++col) {
            float r = radius_dist(rng);
            float px = spawn_left + col * spacing + jitter(rng);
            float py = top + 20 + row * spacing + jitter(rng);
            if (py > bottom - 20) break;
            Ball b;
            b.pos = {px, py};
            b.vel = {jitter(rng) * 2.0f, 0.0f};
            b.radius = r;
            b.mass = r * r; // proportional to area
            balls.push_back(b);
            ++count;
        }
        if (top + 20 + (row + 1) * spacing > bottom - 20) break;
    }
}

void World::apply_gravity(float dt) {
    for (auto& b : balls) {
        b.vel.y += gravity * dt;
    }
}

void World::resolve_ball_wall(Ball& ball) {
    for (auto& w : walls) {
        Vec2 cp = closest_point_on_segment(ball.pos, w.a, w.b);
        Vec2 diff = ball.pos - cp;
        float dist = diff.length();
        if (dist < ball.radius && dist > 1e-6f) {
            Vec2 n = diff.normalized();
            float penetration = ball.radius - dist;

            // Positional correction
            ball.pos += n * penetration;

            // Velocity reflection
            float vn = ball.vel.dot(n);
            if (vn < 0) {
                // Restitution bias: kill bounce for very low velocities
                float e = (std::abs(vn) < 5.0f) ? 0.0f : restitution;
                ball.vel -= n * (vn * (1.0f + e));
            }
        } else if (dist < 1e-6f) {
            // Ball center is exactly on the wall — use wall normal
            ball.pos += w.normal * ball.radius;
            float vn = ball.vel.dot(w.normal);
            if (vn < 0) {
                ball.vel -= w.normal * (vn * (1.0f + restitution));
            }
        }
    }
}

void World::resolve_ball_ball(Ball& a, Ball& b) {
    Vec2 diff = b.pos - a.pos;
    float dist2 = diff.length2();
    float min_dist = a.radius + b.radius;
    if (dist2 < min_dist * min_dist && dist2 > 1e-8f) {
        float dist = std::sqrt(dist2);
        Vec2 n = diff * (1.0f / dist);
        float penetration = min_dist - dist;

        // Positional correction with slop
        float slop = 0.2f;
        float correction = std::max(penetration - slop, 0.0f);
        float total_mass = a.mass + b.mass;
        a.pos -= n * (correction * (b.mass / total_mass));
        b.pos += n * (correction * (a.mass / total_mass));

        // Velocity response
        float rel_vn = (b.vel - a.vel).dot(n);
        if (rel_vn < 0) {
            float e = (std::abs(rel_vn) < 5.0f) ? 0.0f : restitution;
            float j = -(1.0f + e) * rel_vn / (1.0f / a.mass + 1.0f / b.mass);
            a.vel -= n * (j / a.mass);
            b.vel += n * (j / b.mass);
        }
    }
}

void World::build_spatial_hash() {
    grid_cols_ = (int)(world_w / CELL_SIZE) + 1;
    grid_rows_ = (int)(world_h / CELL_SIZE) + 1;
    int total = grid_cols_ * grid_rows_;
    grid_.resize(total);
    for (auto& cell : grid_) cell.clear();

    for (int i = 0; i < (int)balls.size(); ++i) {
        auto& b = balls[i];
        int cx0 = std::max(0, (int)((b.pos.x - b.radius) / CELL_SIZE));
        int cy0 = std::max(0, (int)((b.pos.y - b.radius) / CELL_SIZE));
        int cx1 = std::min(grid_cols_ - 1, (int)((b.pos.x + b.radius) / CELL_SIZE));
        int cy1 = std::min(grid_rows_ - 1, (int)((b.pos.y + b.radius) / CELL_SIZE));
        for (int cy = cy0; cy <= cy1; ++cy)
            for (int cx = cx0; cx <= cx1; ++cx)
                grid_[cell_index(cx, cy)].push_back(i);
    }
}

void World::step(float dt) {
    if (paused) return;

    // Fixed timestep substeps
    const float fixed_dt = 1.0f / 120.0f;
    float accum = dt;
    while (accum > 0.0f) {
        float sub_dt = std::min(accum, fixed_dt);
        accum -= sub_dt;

        apply_gravity(sub_dt);

        // Integrate positions
        for (auto& b : balls) {
            b.pos += b.vel * sub_dt;
        }

        // Solver iterations
        for (int iter = 0; iter < solver_iterations; ++iter) {
            // Ball-wall
            for (auto& b : balls) {
                resolve_ball_wall(b);
            }

            // Ball-ball with spatial hash
            build_spatial_hash();
            for (int cy = 0; cy < grid_rows_; ++cy) {
                for (int cx = 0; cx < grid_cols_; ++cx) {
                    auto& cell = grid_[cell_index(cx, cy)];
                    // Check within cell
                    for (int i = 0; i < (int)cell.size(); ++i) {
                        for (int j = i + 1; j < (int)cell.size(); ++j) {
                            resolve_ball_ball(balls[cell[i]], balls[cell[j]]);
                        }
                    }
                    // Check neighbor cells (right, below, below-right, below-left)
                    int neighbors[][2] = {{1,0},{0,1},{1,1},{-1,1}};
                    for (auto& nb : neighbors) {
                        int nx = cx + nb[0], ny = cy + nb[1];
                        if (nx < 0 || nx >= grid_cols_ || ny < 0 || ny >= grid_rows_) continue;
                        auto& ncell = grid_[cell_index(nx, ny)];
                        for (int i : cell) {
                            for (int j : ncell) {
                                if (i < j) resolve_ball_ball(balls[i], balls[j]);
                            }
                        }
                    }
                }
            }
        }

        // Damping: sleep very slow balls
        for (auto& b : balls) {
            float speed2 = b.vel.length2();
            if (speed2 < 1.0f) {
                b.vel = b.vel * 0.9f;
            }
        }
    }
}
