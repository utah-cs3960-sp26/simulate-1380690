#include "simulation.h"
#include <algorithm>
#include <random>
#include <fstream>
#include <sstream>
#include <cstdio>

void World::setup_walls(int width, int height) {
    float margin = 30.0f;
    wall_left   = margin;
    wall_right  = width - margin;
    wall_top    = margin;
    wall_bottom = height - margin;
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
    std::uniform_int_distribution<int> color_dist(80, 255);

    float spacing = 13.0f;
    float spawn_left = wall_left + 8.0f;
    float spawn_right = wall_right - 8.0f;
    float spawn_top = wall_top + 8.0f;

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

void World::build_spatial_hash() {
    grid_cols_ = (int)(world_w_ / CELL_SIZE) + 1;
    grid_rows_ = (int)(world_h_ / CELL_SIZE) + 1;
    int total = grid_cols_ * grid_rows_;
    grid_counts_.assign(total, 0);

    for (int i = 0; i < (int)balls.size(); ++i) {
        int cx = std::max(0, std::min(grid_cols_ - 1, (int)(balls[i].pos.x / CELL_SIZE)));
        int cy = std::max(0, std::min(grid_rows_ - 1, (int)(balls[i].pos.y / CELL_SIZE)));
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
        int cy = std::max(0, std::min(grid_rows_ - 1, (int)(balls[i].pos.y / CELL_SIZE)));
        int idx = cy * grid_cols_ + cx;
        grid_data_[grid_starts_[idx] + grid_counts_[idx]] = i;
        ++grid_counts_[idx];
    }
}

void World::substep() {
    float dt = fixed_dt_;

    // 1. Apply gravity
    for (auto& b : balls) {
        b.vel.y += GRAVITY * dt;
    }

    // 2. Apply damping
    for (auto& b : balls) {
        b.vel = b.vel * DAMPING;
    }

    // 3. Move all balls
    for (auto& b : balls) {
        b.pos += b.vel * dt;
    }

    // 4-5. Resolve collisions (multiple iterations for stacking)
    for (int iter = 0; iter < SOLVER_ITERATIONS; ++iter) {
        // Ball-wall collisions
        for (auto& b : balls) {
            // Floor
            if (b.pos.y + b.radius > wall_bottom) {
                b.pos.y = wall_bottom - b.radius;
                if (b.vel.y > 0) {
                    float e = (std::abs(b.vel.y) < REST_VELOCITY_CUTOFF) ? 0.0f : restitution;
                    b.vel.y = -b.vel.y * e;
                }
            }
            // Ceiling
            if (b.pos.y - b.radius < wall_top) {
                b.pos.y = wall_top + b.radius;
                if (b.vel.y < 0) {
                    float e = (std::abs(b.vel.y) < REST_VELOCITY_CUTOFF) ? 0.0f : restitution;
                    b.vel.y = -b.vel.y * e;
                }
            }
            // Left wall
            if (b.pos.x - b.radius < wall_left) {
                b.pos.x = wall_left + b.radius;
                if (b.vel.x < 0) {
                    float e = (std::abs(b.vel.x) < REST_VELOCITY_CUTOFF) ? 0.0f : restitution;
                    b.vel.x = -b.vel.x * e;
                }
            }
            // Right wall
            if (b.pos.x + b.radius > wall_right) {
                b.pos.x = wall_right - b.radius;
                if (b.vel.x > 0) {
                    float e = (std::abs(b.vel.x) < REST_VELOCITY_CUTOFF) ? 0.0f : restitution;
                    b.vel.x = -b.vel.x * e;
                }
            }
        }

        // Build spatial hash (only on first iteration)
        if (iter == 0) {
            build_spatial_hash();
        }

        // Ball-ball collisions via spatial hash
        for (int cy = 0; cy < grid_rows_; ++cy) {
            for (int cx = 0; cx < grid_cols_; ++cx) {
                int cell_idx = cy * grid_cols_ + cx;
                int cell_start = grid_starts_[cell_idx];
                int cell_count = grid_starts_[cell_idx + 1] - cell_start;
                if (cell_count == 0) continue;

                // Check this cell + 4 forward neighbors
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
                            Vec2 n = diff * (1.0f / dist);
                            float overlap = min_dist - dist;

                            // Push apart (equal weight by inverse mass)
                            float total_mass = a.mass + b.mass;
                            a.pos -= n * (overlap * 0.5f * (b.mass / total_mass));
                            b.pos += n * (overlap * 0.5f * (a.mass / total_mass));

                            // Velocity response
                            float rel_vn = (b.vel - a.vel).dot(n);
                            if (rel_vn < 0) {
                                float e = (std::abs(rel_vn) < REST_VELOCITY_CUTOFF) ? 0.0f : restitution;
                                float j_imp = -(1.0f + e) * rel_vn / (1.0f / a.mass + 1.0f / b.mass);
                                a.vel -= n * (j_imp / a.mass);
                                b.vel += n * (j_imp / b.mass);
                            }
                        }
                    }
                }
            }
        }
    }

    // 6. Sleep: if speed < threshold, zero velocity
    for (auto& b : balls) {
        if (b.vel.length() < SLEEP_SPEED) {
            b.vel = {0, 0};
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
