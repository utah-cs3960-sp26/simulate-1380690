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

// Create a wall segment with a precomputed inward normal
static WallSegment make_wall(Vec2 a, Vec2 b, Vec2 interior_point) {
    Vec2 ab = b - a;
    // Perpendicular: (-dy, dx)
    Vec2 n(-ab.y, ab.x);
    float len = n.length();
    if (len > 1e-8f) n = n * (1.0f / len);
    // Make sure normal points toward the interior
    if ((interior_point - a).dot(n) < 0) {
        n = n * -1.0f;
    }
    return {a, b, n};
}

void World::setup_walls(int width, int height) {
    walls.clear();

    float center_x = (float)width / 2.0f;

    // Funnel parameters
    float funnel_top_y = 50.0f;
    float funnel_bottom_y = 250.0f;
    float funnel_top_left = 100.0f;
    float funnel_top_right = (float)width - 100.0f;
    float funnel_gap = 60.0f;

    // Box parameters
    float box_left = center_x - 400.0f;
    float box_right = center_x + 400.0f;
    float box_top = funnel_bottom_y;
    float box_bottom = (float)height - 30.0f;

    // Interior point for normal computation (center of box)
    Vec2 interior(center_x, (box_top + box_bottom) / 2.0f);
    // Interior point for funnel (center of funnel)
    Vec2 funnel_interior(center_x, (funnel_top_y + funnel_bottom_y) / 2.0f);

    // Left funnel wall
    walls.push_back(make_wall(
        Vec2(funnel_top_left, funnel_top_y),
        Vec2(center_x - funnel_gap, funnel_bottom_y),
        funnel_interior));
    // Right funnel wall
    walls.push_back(make_wall(
        Vec2(funnel_top_right, funnel_top_y),
        Vec2(center_x + funnel_gap, funnel_bottom_y),
        funnel_interior));

    // Horizontal ledge connecting funnel to box (left side)
    walls.push_back(make_wall(
        Vec2(center_x - funnel_gap, funnel_bottom_y),
        Vec2(box_left, funnel_bottom_y),
        interior));
    // Horizontal ledge connecting funnel to box (right side)
    walls.push_back(make_wall(
        Vec2(box_right, funnel_bottom_y),
        Vec2(center_x + funnel_gap, funnel_bottom_y),
        interior));

    // Left side of box
    walls.push_back(make_wall(
        Vec2(box_left, box_top),
        Vec2(box_left, box_bottom),
        interior));
    // Bottom of box
    walls.push_back(make_wall(
        Vec2(box_left, box_bottom),
        Vec2(box_right, box_bottom),
        interior));
    // Right side of box
    walls.push_back(make_wall(
        Vec2(box_right, box_bottom),
        Vec2(box_right, box_top),
        interior));

    // Extended funnel side walls
    Vec2 upper_interior(center_x, funnel_top_y + 10.0f);
    walls.push_back(make_wall(
        Vec2(funnel_top_left, funnel_top_y),
        Vec2(box_left, funnel_top_y),
        upper_interior));
    walls.push_back(make_wall(
        Vec2(box_left, funnel_top_y),
        Vec2(box_left, box_top),
        interior));
    walls.push_back(make_wall(
        Vec2(funnel_top_right, funnel_top_y),
        Vec2(box_right, funnel_top_y),
        upper_interior));
    walls.push_back(make_wall(
        Vec2(box_right, funnel_top_y),
        Vec2(box_right, box_top),
        interior));
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

    float spacing = 13.0f;
    float spawn_left = 120.0f;
    float spawn_right = (float)width - 120.0f;
    float spawn_top = -800.0f;

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
    grid_rows_ = (int)((world_h_ + 1000) / CELL_SIZE) + 1;
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

void World::resolve_ball_wall(bool apply_bounce) {
    int n = (int)balls.size();
    for (int i = 0; i < n; ++i) {
        Ball& ball = balls[i];
        for (auto& wall : walls) {
            Vec2 closest = closest_point_on_segment(ball.pos, wall.a, wall.b);
            Vec2 diff = ball.pos - closest;
            float dist2 = diff.length2();
            if (dist2 >= ball.radius * ball.radius) continue;

            // Use the wall's fixed inward normal
            Vec2 normal = wall.normal;

            // Check if ball center is on the wrong side of the wall
            float signed_dist = (ball.pos - wall.a).dot(normal);
            float dist = std::sqrt(dist2);

            float penetration;
            if (signed_dist < 0) {
                // Ball center is on the wrong side — push it fully back
                penetration = ball.radius + std::abs(signed_dist);
                // Use wall normal directly since we're on wrong side
            } else if (dist < ball.radius) {
                penetration = ball.radius - dist;
                // Use actual direction if on correct side
                if (dist > 1e-4f) {
                    normal = diff * (1.0f / dist);
                }
            } else {
                continue;
            }

            ball.pos += normal * penetration;

            // Velocity: remove inward component (and optionally bounce)
            float vn = ball.vel.dot(normal);
            if (vn < 0) {
                if (apply_bounce) {
                    float e = (std::abs(vn) < REST_VELOCITY_CUTOFF) ? 0.0f : restitution;
                    ball.vel -= normal * ((1.0f + e) * vn);
                } else {
                    // Position-only: just remove the penetrating velocity component
                    ball.vel -= normal * vn;
                }
                if (ball.sleeping) {
                    ball.sleeping = false;
                    ball.sleep_counter = 0;
                }
            }
        }
    }
}

void World::resolve_ball_ball(bool apply_bounce) {
    static constexpr int neighbors[][2] = {{0,0},{1,0},{0,1},{1,1},{-1,1}};

    for (int cy = 0; cy < grid_rows_; ++cy) {
        for (int cx = 0; cx < grid_cols_; ++cx) {
            int cell_idx = cy * grid_cols_ + cx;
            int cell_start = grid_starts_[cell_idx];
            int cell_count = grid_starts_[cell_idx + 1] - cell_start;
            if (cell_count == 0) continue;

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

                        if (dist2 >= min_dist * min_dist) continue;

                        float dist;
                        Vec2 normal;
                        if (dist2 < 1e-8f) {
                            // Centers coincide — use arbitrary separation direction
                            normal = Vec2(0, 1);
                            dist = 0.0f;
                        } else {
                            dist = std::sqrt(dist2);
                            normal = diff * (1.0f / dist);
                        }
                        float overlap = min_dist - dist;

                        // Position correction: treat sleeping balls as immovable
                        bool a_sleep = a.sleeping;
                        bool b_sleep = b.sleeping;

                        if (a_sleep && b_sleep) {
                            // Both sleeping — split equally but don't wake
                            a.pos -= normal * (overlap * 0.5f);
                            b.pos += normal * (overlap * 0.5f);
                            continue;
                        }

                        if (a_sleep) {
                            // Only move b
                            b.pos += normal * overlap;
                        } else if (b_sleep) {
                            // Only move a
                            a.pos -= normal * overlap;
                        } else {
                            // Both awake — mass-weighted
                            float total_mass = a.mass + b.mass;
                            float a_share = b.mass / total_mass;
                            float b_share = a.mass / total_mass;
                            a.pos -= normal * (overlap * a_share);
                            b.pos += normal * (overlap * b_share);
                        }

                        // Velocity impulse
                        float rel_vn = (b.vel - a.vel).dot(normal);
                        if (rel_vn < 0) {
                            float e;
                            if (apply_bounce) {
                                e = (std::abs(rel_vn) < REST_VELOCITY_CUTOFF) ? 0.0f : restitution;
                            } else {
                                e = 0.0f;
                            }

                            // Effective masses (sleeping = infinite mass)
                            float inv_mass_a = a_sleep ? 0.0f : (1.0f / a.mass);
                            float inv_mass_b = b_sleep ? 0.0f : (1.0f / b.mass);
                            float inv_mass_sum = inv_mass_a + inv_mass_b;
                            if (inv_mass_sum < 1e-8f) continue;

                            float j_imp = -(1.0f + e) * rel_vn / inv_mass_sum;
                            a.vel -= normal * (j_imp * inv_mass_a);
                            b.vel += normal * (j_imp * inv_mass_b);

                            // Wake sleeping balls only on significant impact
                            if (std::abs(rel_vn) > WAKE_IMPULSE_THRESHOLD) {
                                if (a_sleep) { a.sleeping = false; a.sleep_counter = 0; }
                                if (b_sleep) { b.sleeping = false; b.sleep_counter = 0; }
                            }
                        }
                    }
                }
            }
        }
    }
}

void World::substep() {
    float dt = fixed_dt_;
    int n = (int)balls.size();

    // 1. Apply gravity and integrate position (skip sleeping balls)
    for (int i = 0; i < n; ++i) {
        if (balls[i].sleeping) continue;
        balls[i].vel.y += GRAVITY * dt;
        balls[i].pos += balls[i].vel * dt;
    }

    // 2. Collision resolution iterations
    // First iteration: apply bounce impulses
    // Subsequent iterations: position correction only (no bounce to avoid energy injection)
    for (int iter = 0; iter < SOLVER_ITERATIONS; ++iter) {
        build_spatial_hash();
        bool apply_bounce = (iter == 0);
        // Ball-ball first, then ball-wall (so walls always win)
        resolve_ball_ball(apply_bounce);
        resolve_ball_wall(apply_bounce);
    }

    // 3. Final wall clamp — ensure no ball is inside a wall after all resolution
    resolve_ball_wall(false);

    // 4. Apply damping
    for (int i = 0; i < n; ++i) {
        if (!balls[i].sleeping) {
            balls[i].vel = balls[i].vel * DAMPING;
        }
    }

    // 5. Sleep: require multiple consecutive slow substeps before sleeping
    for (int i = 0; i < n; ++i) {
        if (balls[i].sleeping) continue;
        if (balls[i].vel.length2() < SLEEP_SPEED * SLEEP_SPEED) {
            balls[i].sleep_counter++;
            if (balls[i].sleep_counter >= SLEEP_FRAMES_REQUIRED) {
                balls[i].vel = {0, 0};
                balls[i].sleeping = true;
            }
        } else {
            balls[i].sleep_counter = 0;
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
