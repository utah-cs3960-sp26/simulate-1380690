#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <cstdint>

struct Vec2 {
    float x = 0, y = 0;
    Vec2() = default;
    Vec2(float x, float y) : x(x), y(y) {}
    Vec2 operator+(Vec2 o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(Vec2 o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    Vec2& operator+=(Vec2 o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(Vec2 o) { x -= o.x; y -= o.y; return *this; }
    float dot(Vec2 o) const { return x * o.x + y * o.y; }
    float length2() const { return x * x + y * y; }
    float length() const { return std::sqrt(length2()); }
    Vec2 normalized() const { float l = length(); return l > 0 ? Vec2{x / l, y / l} : Vec2{0, 0}; }
};

struct Ball {
    Vec2 pos;
    Vec2 vel;
    float radius;
    float mass;
    bool sleeping = false;
    int sleep_counter = 0;
    uint8_t color_r = 255, color_g = 255, color_b = 255;
};

struct WallSegment {
    Vec2 a, b;
    Vec2 normal;  // inward-facing normal (points toward legal region)
};

struct World {
    std::vector<Ball> balls;
    std::vector<WallSegment> walls;
    float restitution = 0.5f;
    bool paused = false;

    void init(int width, int height);
    void init_from_csv(const std::string& filename, int width, int height);
    void save_csv(const std::string& filename) const;
    void step(float dt);

private:
    static constexpr float GRAVITY = 980.0f;
    static constexpr float fixed_dt_ = 1.0f / 120.0f;
    static constexpr int MAX_SUBSTEPS = 8;
    static constexpr int SOLVER_ITERATIONS = 10;
    static constexpr float DAMPING = 0.995f;
    static constexpr float SLEEP_SPEED = 5.0f;
    static constexpr int SLEEP_FRAMES_REQUIRED = 20;
    static constexpr float REST_VELOCITY_CUTOFF = 15.0f;
    static constexpr float WAKE_IMPULSE_THRESHOLD = 5.0f;
    static constexpr float CELL_SIZE = 16.0f;

    float accumulator_ = 0.0f;
    int world_w_ = 0, world_h_ = 0;

    // Spatial hash
    std::vector<int> grid_counts_;
    std::vector<int> grid_starts_;
    std::vector<int> grid_data_;
    int grid_cols_ = 0, grid_rows_ = 0;

    void substep();
    void build_spatial_hash();
    void setup_walls(int width, int height);
    void resolve_ball_wall(bool apply_bounce);
    void resolve_ball_ball(bool apply_bounce);
};
