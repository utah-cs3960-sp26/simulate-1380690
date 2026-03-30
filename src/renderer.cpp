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

void Renderer::draw(const World& world, float fps) {
    SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);
    SDL_RenderClear(renderer);

    // Draw container walls
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_FRect wall_rect = {
        world.wall_left, world.wall_top,
        world.wall_right - world.wall_left,
        world.wall_bottom - world.wall_top
    };
    SDL_RenderRect(renderer, &wall_rect);

    // Draw balls — batch rects by color
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
