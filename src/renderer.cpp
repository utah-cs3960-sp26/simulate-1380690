#include "renderer.h"
#include <cmath>
#include <cstdio>
#include <cstring>

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

void Renderer::draw_thick_line(Vec2 a, Vec2 b, float thickness) {
    Vec2 d = b - a;
    float len = d.length();
    if (len < 1e-6f) return;
    Vec2 n = Vec2{-d.y, d.x} * (1.0f / len) * (thickness * 0.5f);

    SDL_FPoint pts[4] = {
        {a.x + n.x, a.y + n.y},
        {a.x - n.x, a.y - n.y},
        {b.x - n.x, b.y - n.y},
        {b.x + n.x, b.y + n.y}
    };
    // Draw as two triangles
    SDL_Vertex verts[6];
    SDL_FColor color = {0.8f, 0.8f, 0.8f, 1.0f};
    int indices[] = {0, 1, 2, 0, 2, 3};
    for (int i = 0; i < 6; ++i) {
        verts[i].position = pts[indices[i]];
        verts[i].color = color;
        verts[i].tex_coord = {0, 0};
    }
    SDL_RenderGeometry(renderer, nullptr, verts, 6, nullptr, 0);
}

void Renderer::draw(const World& world, float fps) {
    SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);
    SDL_RenderClear(renderer);

    // Draw walls
    for (auto& w : world.walls) {
        draw_thick_line(w.a, w.b, 3.0f);
    }

    // Draw balls — batch all scanlines into one array per draw color change
    // Pre-reserve space to avoid reallocations
    circle_rects_.clear();
    circle_rects_.reserve(world.balls.size() * 12);

    // Sort balls by color for better batching would be costly, so just
    // batch per-ball and flush per color change
    uint8_t last_r = 0, last_g = 0, last_b = 0;
    bool first = true;
    for (auto& b : world.balls) {
        // Flush batch on color change
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

    int sleeping = 0;
    for (auto& b : world.balls) if (b.sleeping) ++sleeping;
    snprintf(buf, sizeof(buf), "Sleeping: %d", sleeping);
    SDL_RenderDebugText(renderer, 10, 58, buf);

    if (world.paused) {
        SDL_RenderDebugText(renderer, 10, 74, "PAUSED");
    }

    SDL_RenderPresent(renderer);
}
