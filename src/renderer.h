#pragma once

#include <SDL3/SDL.h>
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
    void draw_filled_circle(float cx, float cy, float r);
    void draw_thick_line(Vec2 a, Vec2 b, float thickness);
};
