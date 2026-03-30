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
    uint8_t color_r = 77, color_g = 153, color_b = 255; // default blue
    bool sleeping = false;
    int sleep_counter = 0;
};

struct Wall {
    Vec2 a, b;
    Vec2 normal; // outward normal (points into the playable area)
};

struct World {
    std::vector<Ball> balls;
    std::vector<Wall> walls;
    float gravity = 1200.0f;      // pixels/s^2
    float restitution = 0.5f;
    int solver_iterations = 8;
    bool paused = false;

    float interpolation_alpha() const { return accumulator_ / fixed_dt_; }

    void init_from_csv(const std::string& filename, int width, int height);
    void save_csv(const std::string& filename) const;

    // Spatial hash
    static constexpr float CELL_SIZE = 20.0f;
    struct CellKey { int cx, cy; };
    struct CellKeyHash {
        std::size_t operator()(CellKey k) const {
            return std::hash<int64_t>()(((int64_t)k.cx << 32) | (uint32_t)k.cy);
        }
    };
    struct CellKeyEqual {
        bool operator()(CellKey a, CellKey b) const { return a.cx == b.cx && a.cy == b.cy; }
    };

    void init(int width, int height);
    void step(float dt);

private:
    static constexpr float fixed_dt_ = 1.0f / 120.0f;
    static constexpr int max_substeps_ = 4;

    int world_w = 0, world_h = 0;
    float accumulator_ = 0.0f;
    void apply_gravity(float dt);
    void resolve_ball_wall(Ball& ball);
    void resolve_ball_ball(Ball& a, Ball& b);
    void build_spatial_hash();

    // Flat spatial hash grid (counting sort, center-cell only)
    std::vector<int> grid_counts_;
    std::vector<int> grid_starts_;
    std::vector<int> grid_data_;
    int grid_cols_ = 0, grid_rows_ = 0;

    // Cached collision pairs from broad phase
    std::vector<std::pair<int,int>> pairs_;
};
