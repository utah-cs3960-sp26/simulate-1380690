#include "simulation.h"
#include <algorithm>
#include <random>
#include <fstream>
#include <sstream>
#include <cstdio>

// Closest point on segment ab to point p
static Vec2 closest_point_on_segment(Vec2 p, Vec2 a, Vec2 b) {
    Vec2 ab = b - a;
    float len2 = ab.length2();
    if (len2 < 1e-8f) return a;
    float t = (p - a).dot(ab) / len2;
    t = std::max(0.0f, std::min(1.0f, t));
    return a + ab * t;
}

void World::setup_walls(int width, int height) {
    walls.clear();

    // Funnel top: two angled walls forming a V
    // Funnel mouth spans most of the top, narrows to ~120px opening
    float funnel_top_y = 50.0f;
    float funnel_bottom_y = 250.0f;
    float funnel_top_left = 100.0f;
    float funnel_top_right = (float)width - 100.0f;
    float funnel_gap = 60.0f;  // half-width of opening
    float center_x = (float)width / 2.0f;

    // Left funnel wall (top-left to center-left)
    walls.push_back({Vec2(funnel_top_left, funnel_top_y), Vec2(center_x - funnel_gap, funnel_bottom_y)});
    // Right funnel wall (top-right to center-right)
    walls.push_back({Vec2(funnel_top_right, funnel_top_y), Vec2(center_x + funnel_gap, funnel_bottom_y)});

    // Box below funnel
    float box_left = center_x - 400.0f;
    float box_right = center_x + 400.0f;
    float box_top = funnel_bottom_y;
    float box_bottom = (float)height - 30.0f;

    // Left wall of box (connects funnel to box)
    walls.push_back({Vec2(center_x - funnel_gap, funnel_bottom_y), Vec2(box_left, funnel_bottom_y)});
    // Left side of box
    walls.push_back({Vec2(box_left, box_top), Vec2(box_left, box_bottom)});
    // Bottom of box
    walls.push_back({Vec2(box_left, box_bottom), Vec2(box_right, box_bottom)});
    // Right side of box
    walls.push_back({Vec2(box_right, box_bottom), Vec2(box_right, box_top)});
    // Right wall of box (connects funnel to box)
    walls.push_back({Vec2(box_right, funnel_bottom_y), Vec2(center_x + funnel_gap, funnel_bottom_y)});

    // Extended funnel side walls down to box edges
    walls.push_back({Vec2(funnel_top_left, funnel_top_y), Vec2(box_left, funnel_top_y)});
    walls.push_back({Vec2(box_left, funnel_top_y), Vec2(box_left, box_top)});
    walls.push_back({Vec2(funnel_top_right, funnel_top_y), Vec2(box_right, funnel_top_y)});
    walls.push_back({Vec2(box_right, funnel_top_y), Vec2(box_right, box_top)});
}

void World::init(int width, int height) {
    world_w_ = width;
    world_h_ = height;
    accumulator_ = 0.0f;
    balls.clear();
    setup_walls(width, height);

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> radius_dist(4.0f, 6.0f);
    std::uniform_real_distribution<float> jitter(-0.2f, 0.2f);
    std::uniform_int_distribution<int> color_dist(150, 255);

    // Spawn balls above the funnel mouth in a grid
    float spacing = 13.0f;
    float spawn_left = 120.0f;
    float spawn_right = (float)width - 120.0f;
    float spawn_top = -800.0f;  // Above screen, above funnel mouth

    int cols = (int)((spawn_right - spawn_left) / spacing);
    int target = 1000;
    int count = 0;

    for (int row = 0; count < target; ++row) {
        for (int col = 0; col < cols && count < target; ++col) {
            Ball b;
            b.radius = radius_dist(rng);
            b.pos.x = spawn_left + col * spacing + jitter(rng);
            b.pos.y = spawn_top + row * spacing + jitter(rng);
            b.vel = {0, 0};
            b.mass = b.radius * b.radius;
            b.color_r = (uint8_t)color_dist(rng);
            b.color_g = (uint8_t)color_dist(rng);
            b.color_b = (uint8_t)color_dist(rng);
            balls.push_back(b);
            ++count;
        }
    }
}

void World::init_from_csv(const std::string& filename, int width, int height) {
    world_w_ = width;
    world_h_ = height;
    accumulator_ = 0.0f;
    balls.clear();
    setup_walls(width, height);

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
        b.color_r = (values.size() >= 4) ? (uint8_t)values[3] : 255;
        b.color_g = (values.size() >= 5) ? (uint8_t)values[4] : 255;
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

void World::build_spatial_hash() {
    grid_cols_ = (int)(world_w_ / CELL_SIZE) + 1;
    grid_rows_ = (int)((world_h_ + 1000) / CELL_SIZE) + 1;  // Extra rows for balls above screen
    int total = grid_cols_ * grid_rows_;
    grid_counts_.assign(total, 0);

    for (int i = 0; i < (int)balls.size(); ++i) {
        int cx = std::max(0, std::min(grid_cols_ - 1, (int)(balls[i].pos.x / CELL_SIZE)));
        int cy = std::max(0, std::min(grid_rows_ - 1, (int)((balls[i].pos.y + 1000.0f) / CELL_SIZE)));
        ++grid_counts_[cy * grid_cols_ + cx];
    }

    grid_starts_.resize(total + 1);
    grid_starts_[0] = 0;
    for (int i = 0; i < total; ++i)
        grid_starts_[i + 1] = grid_starts_[i] + grid_counts_[i];

    grid_data_.resize(grid_starts_[total]);
    std::fill(grid_counts_.begin(), grid_counts_.end(), 0);

    for (int i = 0; i < (int)balls.size(); ++i) {
        int cx = std::max(0, std::min(grid_cols_ - 1, (int)(balls[i].pos.x / CELL_SIZE)));
        int cy = std::max(0, std::min(grid_rows_ - 1, (int)((balls[i].pos.y + 1000.0f) / CELL_SIZE)));
        int idx = cy * grid_cols_ + cx;
        grid_data_[grid_starts_[idx] + grid_counts_[idx]] = i;
        ++grid_counts_[idx];
    }
}

void World::substep() {
    float dt = fixed_dt_;
    int n = (int)balls.size();

    // 1. Apply gravity and integrate position
    for (int i = 0; i < n; ++i) {
        if (balls[i].sleeping) continue;
        balls[i].vel.y += GRAVITY * dt;
        balls[i].pos += balls[i].vel * dt;
    }

    // 2. Resolve collisions (multiple iterations for stable stacking)
    for (int iter = 0; iter < SOLVER_ITERATIONS; ++iter) {
        build_spatial_hash();

        // Ball-wall segment collisions
        for (int i = 0; i < n; ++i) {
            Ball& ball = balls[i];
            for (auto& wall : walls) {
                Vec2 closest = closest_point_on_segment(ball.pos, wall.a, wall.b);
                Vec2 diff = ball.pos - closest;
                float dist2 = diff.length2();
                if (dist2 >= ball.radius * ball.radius || dist2 < 1e-8f) continue;

                float dist = std::sqrt(dist2);
                Vec2 normal = diff * (1.0f / dist);
                float penetration = ball.radius - dist;

                // Push out along normal
                ball.pos += normal * penetration;

                // Reflect velocity component along normal
                float vn = ball.vel.dot(normal);
                if (vn < 0) {
                    float e = (std::abs(vn) < REST_VELOCITY_CUTOFF) ? 0.0f : restitution;
                    ball.vel -= normal * ((1.0f + e) * vn);
                    if (ball.sleeping) {
                        ball.sleeping = false;
                    }
                }
            }
        }

        // Ball-ball collisions via spatial hash
        for (int cy = 0; cy < grid_rows_; ++cy) {
            for (int cx = 0; cx < grid_cols_; ++cx) {
                int cell_idx = cy * grid_cols_ + cx;
                int cell_start = grid_starts_[cell_idx];
                int cell_count = grid_starts_[cell_idx + 1] - cell_start;
                if (cell_count == 0) continue;

                static constexpr int neighbors[][2] = {{0,0},{1,0},{0,1},{1,1},{-1,1}};
                for (auto& nb : neighbors) {
                    int nx = cx + nb[0], ny = cy + nb[1];
                    if (nx < 0 || nx >= grid_cols_ || ny < 0 || ny >= grid_rows_) continue;
                    int nidx = ny * grid_cols_ + nx;
                    int nstart = grid_starts_[nidx];
                    int ncount = grid_starts_[nidx + 1] - nstart;
                    if (ncount == 0) continue;

                    bool same_cell = (nb[0] == 0 && nb[1] == 0);

                    for (int i = 0; i < cell_count; ++i) {
                        int ai = grid_data_[cell_start + i];
                        int j_start = same_cell ? i + 1 : 0;
                        for (int j = j_start; j < ncount; ++j) {
                            int bi = grid_data_[nstart + j];
                            if (ai == bi) continue;

                            Ball& a = balls[ai];
                            Ball& b = balls[bi];
                            Vec2 diff = b.pos - a.pos;
                            float dist2 = diff.length2();
                            float min_dist = a.radius + b.radius;

                            if (dist2 >= min_dist * min_dist || dist2 < 1e-8f) continue;

                            float dist = std::sqrt(dist2);
                            Vec2 normal = diff * (1.0f / dist);
                            float overlap = min_dist - dist;

                            // Push apart — mass-weighted
                            float total_mass = a.mass + b.mass;
                            float a_share = b.mass / total_mass;
                            float b_share = a.mass / total_mass;
                            a.pos -= normal * (overlap * a_share);
                            b.pos += normal * (overlap * b_share);

                            // Velocity impulse
                            float rel_vn = (b.vel - a.vel).dot(normal);
                            if (rel_vn < 0) {
                                float e = (std::abs(rel_vn) < REST_VELOCITY_CUTOFF) ? 0.0f : restitution;
                                float j_imp = -(1.0f + e) * rel_vn / (1.0f / a.mass + 1.0f / b.mass);
                                a.vel -= normal * (j_imp / a.mass);
                                b.vel += normal * (j_imp / b.mass);

                                // Wake sleeping balls on collision
                                if (a.sleeping) a.sleeping = false;
                                if (b.sleeping) b.sleeping = false;
                            }
                        }
                    }
                }
            }
        }
    }

    // 3. Apply damping
    for (int i = 0; i < n; ++i) {
        if (!balls[i].sleeping) {
            balls[i].vel = balls[i].vel * DAMPING;
        }
    }

    // 4. Sleep: if speed < threshold, zero velocity
    for (int i = 0; i < n; ++i) {
        if (!balls[i].sleeping && balls[i].vel.length2() < SLEEP_SPEED * SLEEP_SPEED) {
            balls[i].vel = {0, 0};
            balls[i].sleeping = true;
        }
    }
}

void World::step(float dt) {
    if (paused) return;

    accumulator_ += dt;
    float max_accum = fixed_dt_ * MAX_SUBSTEPS;
    if (accumulator_ > max_accum) accumulator_ = max_accum;

    while (accumulator_ >= fixed_dt_) {
        accumulator_ -= fixed_dt_;
        substep();
    }
}
