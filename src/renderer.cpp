#include "renderer.h"
#include <cmath>
#include <cstdio>

bool Renderer::init(int width, int height) {
    width_ = width;
    height_ = height;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow("Physics Simulator", width, height, 0);
    if (!window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        return false;
    }

    renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        return false;
    }

    SDL_SetRenderVSync(renderer, 1);
    return true;
}

void Renderer::shutdown() {
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

static void draw_thick_line(SDL_Renderer* r, float x1, float y1, float x2, float y2, float thickness) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 1e-6f) return;
    float nx = -dy / len * thickness * 0.5f;
    float ny = dx / len * thickness * 0.5f;

    SDL_Vertex verts[4];
    SDL_FColor color = {0.78f, 0.78f, 0.78f, 1.0f};  // light gray
    verts[0] = {{x1 + nx, y1 + ny}, color, {0, 0}};
    verts[1] = {{x1 - nx, y1 - ny}, color, {0, 0}};
    verts[2] = {{x2 - nx, y2 - ny}, color, {0, 0}};
    verts[3] = {{x2 + nx, y2 + ny}, color, {0, 0}};

    int indices[6] = {0, 1, 2, 0, 2, 3};
    SDL_RenderGeometry(r, nullptr, verts, 4, indices, 6);
}

void Renderer::draw(const World& world, float fps) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Draw wall segments (thick lines, 3px)
    for (auto& wall : world.walls) {
        draw_thick_line(renderer, wall.a.x, wall.a.y, wall.b.x, wall.b.y, 3.0f);
    }

    // Draw balls as filled circles
    circle_rects_.clear();
    circle_rects_.reserve(world.balls.size() * 12);

    uint8_t last_r = 0, last_g = 0, last_b = 0;
    bool first = true;
    for (auto& b : world.balls) {
        if (first || b.color_r != last_r || b.color_g != last_g || b.color_b != last_b) {
            if (!first && !circle_rects_.empty()) {
                SDL_RenderFillRects(renderer, circle_rects_.data(), (int)circle_rects_.size());
                circle_rects_.clear();
            }
            SDL_SetRenderDrawColor(renderer, b.color_r, b.color_g, b.color_b, 255);
            last_r = b.color_r; last_g = b.color_g; last_b = b.color_b;
            first = false;
        }
        int ir = (int)b.radius;
        for (int dy = -ir; dy <= ir; ++dy) {
            float half_w = std::sqrt(b.radius * b.radius - (float)(dy * dy));
            circle_rects_.push_back({b.pos.x - half_w, b.pos.y + dy, half_w * 2.0f, 1.0f});
        }
    }
    if (!circle_rects_.empty()) {
        SDL_RenderFillRects(renderer, circle_rects_.data(), (int)circle_rects_.size());
    }

    // HUD
    char buf[128];
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    snprintf(buf, sizeof(buf), "FPS: %.0f", fps);
    SDL_RenderDebugText(renderer, 10, 10, buf);

    snprintf(buf, sizeof(buf), "Balls: %d", (int)world.balls.size());
    SDL_RenderDebugText(renderer, 10, 26, buf);

    snprintf(buf, sizeof(buf), "Restitution: %.2f", world.restitution);
    SDL_RenderDebugText(renderer, 10, 42, buf);

    if (world.paused) {
        SDL_RenderDebugText(renderer, 10, 58, "PAUSED");
    }

    SDL_RenderPresent(renderer);
}
