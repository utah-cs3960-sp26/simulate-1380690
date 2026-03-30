#include "simulation.h"
#include <algorithm>
#include <random>
#include <cmath>
#include <fstream>
#include <sstream>
#include <cstdio>

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

static constexpr float SLEEP_SPEED_THRESHOLD = 8.0f;
static constexpr int SLEEP_FRAMES_REQUIRED = 60;
static constexpr float DAMPING = 0.998f;
static constexpr float RESTITUTION_CUTOFF_VEL = 2.0f;

void World::init(int width, int height) {
    world_w = width;
    world_h = height;
    accumulator_ = 0.0f;
    balls.clear();
    walls.clear();

    float margin = 30.0f;
    float left = margin, right = width - margin;
    float top = margin, bottom = height - margin;

    // 4 container walls — normals point inward
    walls.push_back({{left, bottom}, {right, bottom}, {0, -1}});
    walls.push_back({{left, top}, {right, top}, {0, 1}});
    walls.push_back({{left, top}, {left, bottom}, {1, 0}});
    walls.push_back({{right, top}, {right, bottom}, {-1, 0}});

    // Funnel: two angled walls in the upper-middle area
    float cx = width / 2.0f;
    float funnel_top = height * 0.35f;
    float funnel_gap = 60.0f;
    walls.push_back({{left + 40, funnel_top - 80}, {cx - funnel_gap, funnel_top}, {}});
    walls.push_back({{right - 40, funnel_top - 80}, {cx + funnel_gap, funnel_top}, {}});

    // Recompute wall normals (perpendicular to wall, pointing toward screen center)
    for (auto& w : walls) {
        Vec2 edge = w.b - w.a;
        Vec2 n1 = Vec2{-edge.y, edge.x}.normalized();
        Vec2 n2 = Vec2{edge.y, -edge.x}.normalized();
        Vec2 mid = (w.a + w.b) * 0.5f;
        Vec2 to_center = Vec2{(float)width / 2, (float)height / 2} - mid;
        w.normal = (n1.dot(to_center) > n2.dot(to_center)) ? n1 : n2;
    }

    // Spawn ~1000 balls in a grid above the funnel
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> radius_dist(4.0f, 6.0f);
    std::uniform_real_distribution<float> jitter(-0.5f, 0.5f);
    std::uniform_int_distribution<int> color_dist(80, 255);

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
            b.mass = r * r;
            b.color_r = (uint8_t)color_dist(rng);
            b.color_g = (uint8_t)color_dist(rng);
            b.color_b = (uint8_t)color_dist(rng);
            balls.push_back(b);
            ++count;
        }
        if (top + 20 + (row + 1) * spacing > bottom - 20) break;
    }
}

void World::init_from_csv(const std::string& filename, int width, int height) {
    world_w = width;
    world_h = height;
    accumulator_ = 0.0f;
    balls.clear();
    walls.clear();

    float margin = 30.0f;
    float left = margin, right = width - margin;
    float top = margin, bottom = height - margin;

    walls.push_back({{left, bottom}, {right, bottom}, {0, -1}});
    walls.push_back({{left, top}, {right, top}, {0, 1}});
    walls.push_back({{left, top}, {left, bottom}, {1, 0}});
    walls.push_back({{right, top}, {right, bottom}, {-1, 0}});

    float cx = width / 2.0f;
    float funnel_top = height * 0.35f;
    float funnel_gap = 60.0f;
    walls.push_back({{left + 40, funnel_top - 80}, {cx - funnel_gap, funnel_top}, {}});
    walls.push_back({{right - 40, funnel_top - 80}, {cx + funnel_gap, funnel_top}, {}});

    for (auto& w : walls) {
        Vec2 edge = w.b - w.a;
        Vec2 n1 = Vec2{-edge.y, edge.x}.normalized();
        Vec2 n2 = Vec2{edge.y, -edge.x}.normalized();
        Vec2 mid = (w.a + w.b) * 0.5f;
        Vec2 to_center = Vec2{(float)width / 2, (float)height / 2} - mid;
        w.normal = (n1.dot(to_center) > n2.dot(to_center)) ? n1 : n2;
    }

    std::ifstream file(filename);
    if (!file.is_open()) {
        fprintf(stderr, "Failed to open CSV: %s, falling back to default scene\n", filename.c_str());
        init(width, height);
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        std::string token;
        std::vector<float> values;
        while (std::getline(ss, token, ',')) {
            values.push_back(std::stof(token));
        }
        if (values.size() < 2) continue;

        Ball b;
        b.pos.x = values[0];
        b.pos.y = values[1];
        b.radius = (values.size() >= 3) ? values[2] : 5.0f;
        b.mass = b.radius * b.radius;
        b.vel = {0, 0};
        b.color_r = (values.size() >= 4) ? (uint8_t)values[3] : 77;
        b.color_g = (values.size() >= 5) ? (uint8_t)values[4] : 153;
        b.color_b = (values.size() >= 6) ? (uint8_t)values[5] : 255;
        balls.push_back(b);
    }
    fprintf(stderr, "Loaded %d balls from CSV: %s\n", (int)balls.size(), filename.c_str());
}

void World::save_csv(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        fprintf(stderr, "Failed to save CSV: %s\n", filename.c_str());
        return;
    }
    for (auto& b : balls) {
        file << b.pos.x << "," << b.pos.y << "," << b.radius << ","
             << (int)b.color_r << "," << (int)b.color_g << "," << (int)b.color_b << "\n";
    }
    fprintf(stderr, "Saved %d ball positions to %s\n", (int)balls.size(), filename.c_str());
}

void World::apply_gravity(float dt) {
    for (auto& b : balls) {
        if (b.sleeping) continue;
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

            // Positional correction with slop
            float slop = 0.3f;
            float correction = std::max(penetration - slop, 0.0f) + std::min(penetration, slop) * 0.2f;
            ball.pos += n * correction;

            // Velocity reflection
            float vn = ball.vel.dot(n);
            if (vn < 0) {
                if (ball.sleeping) {
                    // Only wake on significant incoming velocity (not gravity settling)
                    if (std::abs(vn) > 5.0f) {
                        ball.sleeping = false;
                        ball.sleep_counter = 0;
                    } else {
                        // Just zero out the penetrating velocity component
                        ball.vel -= n * vn;
                        continue;
                    }
                }
                float e = (std::abs(vn) < RESTITUTION_CUTOFF_VEL) ? 0.0f : restitution;
                ball.vel -= n * (vn * (1.0f + e));

                // Tangential friction to help settling
                Vec2 tangent_vel = ball.vel - n * ball.vel.dot(n);
                float friction = 0.3f;
                ball.vel -= tangent_vel * friction;
            }
        } else if (dist < 1e-6f) {
            ball.pos += w.normal * ball.radius;
            float vn = ball.vel.dot(w.normal);
            if (vn < 0) {
                if (ball.sleeping) {
                    ball.vel -= w.normal * vn;
                    continue;
                }
                float e = (std::abs(vn) < RESTITUTION_CUTOFF_VEL) ? 0.0f : restitution;
                ball.vel -= w.normal * (vn * (1.0f + e));
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

        // If both sleeping, only apply gentle positional correction if deep overlap
        if (a.sleeping && b.sleeping) {
            if (penetration > 1.0f) {
                float total_mass = a.mass + b.mass;
                float correction = penetration * 0.1f;
                a.pos -= n * (correction * (b.mass / total_mass));
                b.pos += n * (correction * (a.mass / total_mass));
            }
            return;
        }

        // Positional correction with slop
        float slop = 0.5f;
        float correction = std::max(penetration - slop, 0.0f) * 0.4f;
        if (correction > 0.0f) {
            float total_mass = a.mass + b.mass;
            if (!a.sleeping) a.pos -= n * (correction * (b.mass / total_mass));
            if (!b.sleeping) b.pos += n * (correction * (a.mass / total_mass));
        }

        // Velocity response
        float rel_vn = (b.vel - a.vel).dot(n);
        if (rel_vn < 0) {
            // Wake sleeping balls only on significant collision
            float wake_threshold = 5.0f;
            if (std::abs(rel_vn) > wake_threshold) {
                if (a.sleeping) { a.sleeping = false; a.sleep_counter = 0; }
                if (b.sleeping) { b.sleeping = false; b.sleep_counter = 0; }
            }

            float e = (std::abs(rel_vn) < RESTITUTION_CUTOFF_VEL) ? 0.0f : restitution;
            float j = -(1.0f + e) * rel_vn / (1.0f / a.mass + 1.0f / b.mass);
            if (!a.sleeping) a.vel -= n * (j / a.mass);
            if (!b.sleeping) b.vel += n * (j / b.mass);
        }
    }
}

void World::build_spatial_hash() {
    grid_cols_ = (int)(world_w / CELL_SIZE) + 1;
    grid_rows_ = (int)(world_h / CELL_SIZE) + 1;
    int total = grid_cols_ * grid_rows_;
    grid_counts_.assign(total, 0);

    for (int i = 0; i < (int)balls.size(); ++i) {
        auto& b = balls[i];
        int cx0 = std::max(0, (int)((b.pos.x - b.radius) / CELL_SIZE));
        int cy0 = std::max(0, (int)((b.pos.y - b.radius) / CELL_SIZE));
        int cx1 = std::min(grid_cols_ - 1, (int)((b.pos.x + b.radius) / CELL_SIZE));
        int cy1 = std::min(grid_rows_ - 1, (int)((b.pos.y + b.radius) / CELL_SIZE));
        for (int cy = cy0; cy <= cy1; ++cy)
            for (int cx = cx0; cx <= cx1; ++cx)
                ++grid_counts_[cy * grid_cols_ + cx];
    }

    grid_starts_.resize(total + 1);
    grid_starts_[0] = 0;
    for (int i = 0; i < total; ++i)
        grid_starts_[i + 1] = grid_starts_[i] + grid_counts_[i];

    int total_entries = grid_starts_[total];
    grid_data_.resize(total_entries);
    std::fill(grid_counts_.begin(), grid_counts_.end(), 0);
    for (int i = 0; i < (int)balls.size(); ++i) {
        auto& b = balls[i];
        int cx0 = std::max(0, (int)((b.pos.x - b.radius) / CELL_SIZE));
        int cy0 = std::max(0, (int)((b.pos.y - b.radius) / CELL_SIZE));
        int cx1 = std::min(grid_cols_ - 1, (int)((b.pos.x + b.radius) / CELL_SIZE));
        int cy1 = std::min(grid_rows_ - 1, (int)((b.pos.y + b.radius) / CELL_SIZE));
        for (int cy = cy0; cy <= cy1; ++cy)
            for (int cx = cx0; cx <= cx1; ++cx) {
                int idx = cy * grid_cols_ + cx;
                grid_data_[grid_starts_[idx] + grid_counts_[idx]] = i;
                ++grid_counts_[idx];
            }
    }
}

void World::step(float dt) {
    if (paused) return;

    const float fixed_dt = 1.0f / 120.0f;
    accumulator_ += dt;

    // Cap accumulator to prevent spiral of death (max 8 substeps per frame)
    float max_accum = fixed_dt * 8.0f;
    if (accumulator_ > max_accum) accumulator_ = max_accum;

    while (accumulator_ >= fixed_dt) {
        accumulator_ -= fixed_dt;

        apply_gravity(fixed_dt);

        // Integrate positions (skip sleeping balls)
        for (auto& b : balls) {
            if (b.sleeping) continue;
            b.vel = b.vel * DAMPING;
            b.pos += b.vel * fixed_dt;
        }

        // Solver iterations
        for (int iter = 0; iter < solver_iterations; ++iter) {
            for (auto& b : balls) {
                resolve_ball_wall(b);
            }

            // Only rebuild spatial hash once per substep (first iteration),
            // positions don't change much between solver iterations
            if (iter == 0) build_spatial_hash();

            for (int cy = 0; cy < grid_rows_; ++cy) {
                for (int cx = 0; cx < grid_cols_; ++cx) {
                    int cell_start = grid_starts_[cy * grid_cols_ + cx];
                    int cell_count = grid_starts_[cy * grid_cols_ + cx + 1] - cell_start;
                    if (cell_count == 0) continue;
                    int* cell_data = &grid_data_[cell_start];

                    for (int i = 0; i < cell_count; ++i) {
                        for (int j = i + 1; j < cell_count; ++j) {
                            resolve_ball_ball(balls[cell_data[i]], balls[cell_data[j]]);
                        }
                    }
                    static constexpr int neighbors[][2] = {{1,0},{0,1},{1,1},{-1,1}};
                    for (auto& nb : neighbors) {
                        int nx = cx + nb[0], ny = cy + nb[1];
                        if (nx < 0 || nx >= grid_cols_ || ny < 0 || ny >= grid_rows_) continue;
                        int nstart = grid_starts_[ny * grid_cols_ + nx];
                        int ncount = grid_starts_[ny * grid_cols_ + nx + 1] - nstart;
                        if (ncount == 0) continue;
                        int* ncell_data = &grid_data_[nstart];
                        for (int i = 0; i < cell_count; ++i) {
                            for (int j = 0; j < ncount; ++j) {
                                int ai = cell_data[i], bi = ncell_data[j];
                                if (ai < bi)
                                    resolve_ball_ball(balls[ai], balls[bi]);
                                else if (bi < ai)
                                    resolve_ball_ball(balls[bi], balls[ai]);
                            }
                        }
                    }
                }
            }
        }

        // Sleep system: track how long balls have been slow
        for (auto& b : balls) {
            if (b.sleeping) continue;
            float speed = b.vel.length();
            if (speed < SLEEP_SPEED_THRESHOLD) {
                b.sleep_counter++;
                // Progressive damping as ball approaches sleep
                float progress = (float)b.sleep_counter / (float)SLEEP_FRAMES_REQUIRED;
                float extra_damp = 1.0f - progress * 0.05f; // ramps from 1.0 to 0.95
                b.vel = b.vel * extra_damp;

                if (b.sleep_counter >= SLEEP_FRAMES_REQUIRED) {
                    b.sleeping = true;
                    b.vel = {0, 0};
                }
            } else {
                b.sleep_counter = 0;
            }
        }
    }
}
