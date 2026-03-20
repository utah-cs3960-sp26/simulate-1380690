#pragma once

#include <vector>
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
};

struct Wall {
    Vec2 a, b;
    Vec2 normal; // outward normal (points into the playable area)
};

struct World {
    std::vector<Ball> balls;
    std::vector<Wall> walls;
    float gravity = 500.0f;       // pixels/s^2
    float restitution = 0.5f;
    int solver_iterations = 4;
    bool paused = false;

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
    int world_w = 0, world_h = 0;
    float accumulator_ = 0.0f;
    void apply_gravity(float dt);
    void resolve_ball_wall(Ball& ball);
    void resolve_ball_ball(Ball& a, Ball& b);
    void build_spatial_hash();

    // Flat spatial hash grid (counting sort)
    std::vector<int> grid_counts_;  // per-cell count
    std::vector<int> grid_starts_;  // prefix sum (size = total_cells + 1)
    std::vector<int> grid_data_;    // ball indices packed contiguously
    int grid_cols_ = 0, grid_rows_ = 0;
};
