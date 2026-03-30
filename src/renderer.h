#pragma once

#include <SDL3/SDL.h>
#include <vector>
#include "simulation.h"

class Renderer {
public:
    bool init(int width, int height);
    void shutdown();
    void draw(const World& world, float fps);

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    int width_ = 0, height_ = 0;

private:
    std::vector<SDL_FRect> circle_rects_;
};
