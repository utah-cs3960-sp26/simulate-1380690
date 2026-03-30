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

static constexpr float SLEEP_SPEED_THRESHOLD = 12.0f;
static constexpr int SLEEP_FRAMES_REQUIRED = 60;
static constexpr float DAMPING = 0.9998f;
static constexpr float WALL_FRICTION = 0.02f;
static constexpr float WALL_SLOP = 0.05f;
static constexpr float BALL_SLOP = 0.20f;
static constexpr float BALL_CORRECTION_PCT = 0.6f;
static constexpr float WAKE_SPEED_THRESHOLD = 20.0f;
static constexpr float BALL_BALL_FRICTION = 0.02f;

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

    // Funnel: two angled walls in the middle area
    float cx = width / 2.0f;
    float funnel_top = height * 0.55f;
    float funnel_half_gap = 40.0f; // total opening = 80px
    walls.push_back({{left + 40, funnel_top - 100}, {cx - funnel_half_gap, funnel_top}, {}});
    walls.push_back({{right - 40, funnel_top - 100}, {cx + funnel_half_gap, funnel_top}, {}});

    // Recompute wall normals (perpendicular to wall, pointing toward screen center)
    for (auto& w : walls) {
        Vec2 edge = w.b - w.a;
        Vec2 n1 = Vec2{-edge.y, edge.x}.normalized();
        Vec2 n2 = Vec2{edge.y, -edge.x}.normalized();
        Vec2 mid = (w.a + w.b) * 0.5f;
        Vec2 to_center = Vec2{(float)width / 2, (float)height / 2} - mid;
        w.normal = (n1.dot(to_center) > n2.dot(to_center)) ? n1 : n2;
    }

    // Spawn ~1000 balls ABOVE the funnel only
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> radius_dist(4.0f, 6.0f);
    std::uniform_real_distribution<float> jitter(-0.25f, 0.25f);
    std::uniform_int_distribution<int> color_dist(80, 255);

    const float max_r = 6.0f;
    const float spacing = 2.0f * max_r + 1.0f; // 13px, tight but no overlap
    float spawn_top = top + max_r + 2.0f;
    float funnel_highest = funnel_top - 100.0f; // highest point of funnel walls
    float spawn_bottom = funnel_highest - max_r - 2.0f;
    float spawn_left = left + max_r + 2.0f;
    float spawn_right = right - max_r - 2.0f;

    int cols = std::max(1, (int)((spawn_right - spawn_left) / spacing));
    int max_rows = std::max(1, (int)((spawn_bottom - spawn_top) / spacing));
    int target = std::min(1000, cols * max_rows);

    int count = 0;
    for (int row = 0; row < max_rows && count < target; ++row) {
        for (int col = 0; col < cols && count < target; ++col) {
            float r = radius_dist(rng);
            float px = spawn_left + col * spacing + jitter(rng);
            float py = spawn_top + row * spacing + jitter(rng);

            Ball b;
            b.pos = {px, py};
            b.prev_pos = b.pos;
            b.vel = {jitter(rng) * 1.0f, 0.0f};
            b.radius = r;
            b.mass = r * r;
            b.color_r = (uint8_t)color_dist(rng);
            b.color_g = (uint8_t)color_dist(rng);
            b.color_b = (uint8_t)color_dist(rng);
            balls.push_back(b);
            ++count;
        }
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
    float funnel_top = height * 0.55f;
    float funnel_half_gap = 40.0f;
    walls.push_back({{left + 40, funnel_top - 100}, {cx - funnel_half_gap, funnel_top}, {}});
    walls.push_back({{right - 40, funnel_top - 100}, {cx + funnel_half_gap, funnel_top}, {}});

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
        b.prev_pos = b.pos;
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
    float restitution_cutoff = std::max(20.0f, 2.0f * gravity * fixed_dt_);

    for (const auto& w : walls) {
        Vec2 edge = w.b - w.a;
        float edge_len2 = edge.length2();
        if (edge_len2 < 1e-8f) continue;

        // Closest point on segment to current position
        float t = clampf((ball.pos - w.a).dot(edge) / edge_len2, 0.0f, 1.0f);
        Vec2 cp = w.a + edge * t;

        // Use stable precomputed wall normal for interior contacts,
        // radial normal near endpoints
        Vec2 diff = ball.pos - cp;
        float diff_len = diff.length();
        Vec2 n;
        if (t > 1e-3f && t < 1.0f - 1e-3f) {
            n = w.normal;
        } else {
            n = (diff_len > 1e-6f) ? diff * (1.0f / diff_len) : w.normal;
        }

        float curr_sep = (ball.pos - cp).dot(n);

        // Swept crossing check
        Vec2 prev_cp = closest_point_on_segment(ball.prev_pos, w.a, w.b);
        float prev_sep = (ball.prev_pos - prev_cp).dot(n);

        bool overlapping = curr_sep < ball.radius;
        bool crossed = prev_sep >= ball.radius * 0.5f && curr_sep < ball.radius * 0.5f;

        if (!overlapping && !crossed) continue;

        ball.contact_count++;

        float penetration = ball.radius - curr_sep;
        if (crossed && penetration < 0.0f) penetration = ball.radius;

        // Positional correction — tight, with minimal slop
        float correction = std::max(penetration - WALL_SLOP, 0.0f);
        if (correction > 0.0f) ball.pos += n * correction;

        // Velocity reflection
        float vn = ball.vel.dot(n);
        if (vn < 0.0f) {
            if (ball.sleeping) {
                if (std::abs(vn) > WAKE_SPEED_THRESHOLD) {
                    ball.sleeping = false;
                    ball.sleep_counter = 0;
                } else {
                    ball.vel -= n * vn;
                    continue;
                }
            }
            float e = (std::abs(vn) < restitution_cutoff) ? 0.0f : restitution;
            ball.vel -= n * (vn * (1.0f + e));

            // Small tangential friction
            Vec2 tangent_vel = ball.vel - n * ball.vel.dot(n);
            ball.vel -= tangent_vel * WALL_FRICTION;
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
            a.contact_count++;
            b.contact_count++;
            return;
        }

        // Positional correction with slop
        float correction = std::max(penetration - BALL_SLOP, 0.0f) * BALL_CORRECTION_PCT;
        if (correction > 0.0f) {
            float total_mass = a.mass + b.mass;
            if (!a.sleeping) a.pos -= n * (correction * (b.mass / total_mass));
            if (!b.sleeping) b.pos += n * (correction * (a.mass / total_mass));
        }

        a.contact_count++;
        b.contact_count++;

        // Velocity response
        float rel_vn = (b.vel - a.vel).dot(n);
        if (rel_vn < 0) {
            // Wake sleeping balls only on significant collision
            if (std::abs(rel_vn) > WAKE_SPEED_THRESHOLD) {
                if (a.sleeping) { a.sleeping = false; a.sleep_counter = 0; }
                if (b.sleeping) { b.sleeping = false; b.sleep_counter = 0; }
            }

            float restitution_cutoff = std::max(20.0f, 2.0f * gravity * fixed_dt_);
            float e = (std::abs(rel_vn) < restitution_cutoff) ? 0.0f : restitution;
            float j = -(1.0f + e) * rel_vn / (1.0f / a.mass + 1.0f / b.mass);
            if (!a.sleeping) a.vel -= n * (j / a.mass);
            if (!b.sleeping) b.vel += n * (j / b.mass);

            // Tangential friction to dissipate lateral/rotational energy
            Vec2 rel2 = b.vel - a.vel;
            Vec2 vt = rel2 - n * rel2.dot(n);
            if (!a.sleeping) a.vel += vt * (0.5f * BALL_BALL_FRICTION);
            if (!b.sleeping) b.vel -= vt * (0.5f * BALL_BALL_FRICTION);
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

void World::collect_pairs() {
    pairs_.clear();

    for (int cy = 0; cy < grid_rows_; ++cy) {
        for (int cx = 0; cx < grid_cols_; ++cx) {
            int cell_idx = cy * grid_cols_ + cx;
            int cell_start = grid_starts_[cell_idx];
            int cell_count = grid_starts_[cell_idx + 1] - cell_start;
            if (cell_count == 0) continue;
            int* cell_data = &grid_data_[cell_start];

            // Intra-cell pairs
            for (int i = 0; i < cell_count; ++i) {
                for (int j = i + 1; j < cell_count; ++j) {
                    int ai = cell_data[i], bi = cell_data[j];
                    if (ai > bi) std::swap(ai, bi);
                    pairs_.push_back({ai, bi});
                }
            }

            // Neighbor-cell pairs (4 forward neighbors to avoid double-counting)
            static constexpr int neighbors[][2] = {{1,0},{0,1},{1,1},{-1,1}};
            for (auto& nb : neighbors) {
                int nx = cx + nb[0], ny = cy + nb[1];
                if (nx < 0 || nx >= grid_cols_ || ny < 0 || ny >= grid_rows_) continue;
                int nidx = ny * grid_cols_ + nx;
                int nstart = grid_starts_[nidx];
                int ncount = grid_starts_[nidx + 1] - nstart;
                if (ncount == 0) continue;
                int* ncell_data = &grid_data_[nstart];
                for (int i = 0; i < cell_count; ++i) {
                    for (int j = 0; j < ncount; ++j) {
                        int ai = cell_data[i], bi = ncell_data[j];
                        if (ai == bi) continue;
                        if (ai > bi) std::swap(ai, bi);
                        pairs_.push_back({ai, bi});
                    }
                }
            }
        }
    }

    // Remove duplicate pairs
    std::sort(pairs_.begin(), pairs_.end());
    pairs_.erase(std::unique(pairs_.begin(), pairs_.end()), pairs_.end());
}

void World::step(float dt) {
    if (paused) return;

    const float fixed_dt = 1.0f / 120.0f;
    accumulator_ += dt;

    // Cap accumulator to prevent spiral of death
    float max_accum = fixed_dt * 8.0f;
    if (accumulator_ > max_accum) accumulator_ = max_accum;

    while (accumulator_ >= fixed_dt) {
        accumulator_ -= fixed_dt;

        apply_gravity(fixed_dt);

        // Save prev positions and reset contact counts
        for (auto& b : balls) {
            b.prev_pos = b.pos;
            b.contact_count = 0;
        }

        // Integrate positions (skip sleeping balls)
        for (auto& b : balls) {
            if (b.sleeping) continue;
            b.vel = b.vel * DAMPING;
            b.pos += b.vel * fixed_dt;
        }

        // Build spatial hash once per substep and collect unique pairs
        build_spatial_hash();
        collect_pairs();

        // Solver iterations
        for (int iter = 0; iter < solver_iterations; ++iter) {
            for (auto& b : balls) {
                resolve_ball_wall(b);
            }

            for (auto& [ai, bi] : pairs_) {
                resolve_ball_ball(balls[ai], balls[bi]);
            }
        }

        // Sleep system: contact-based — only sleep if resting on something
        for (auto& b : balls) {
            float speed = b.vel.length();

            if (b.sleeping) {
                // Hysteretic wake: require several unsupported substeps
                if (b.contact_count == 0) {
                    if (++b.unsupported_counter >= 3) {
                        b.sleeping = false;
                        b.sleep_counter = 0;
                        b.unsupported_counter = 0;
                    }
                } else {
                    b.unsupported_counter = 0;
                }
                continue;
            }

            if (b.contact_count > 0 && speed < SLEEP_SPEED_THRESHOLD) {
                b.sleep_counter++;
                if (b.sleep_counter >= SLEEP_FRAMES_REQUIRED) {
                    b.sleeping = true;
                    b.vel = {0, 0};
                    b.prev_pos = b.pos;
                }
            } else {
                b.sleep_counter = 0;
            }
        }
    }
}
