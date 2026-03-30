#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "simulation.h"
#include "renderer.h"
#include <algorithm>
#include <string>
#include <cstring>

static constexpr int WIDTH = 1280;
static constexpr int HEIGHT = 720;

int main(int argc, char* argv[]) {
    std::string scene_file;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--scene") == 0 && i + 1 < argc) {
            scene_file = argv[++i];
        }
    }

    Renderer ren;
    if (!ren.init(WIDTH, HEIGHT)) return 1;

    World world;
    if (!scene_file.empty()) {
        world.init_from_csv(scene_file, WIDTH, HEIGHT);
    } else {
        world.init(WIDTH, HEIGHT);
        world.save_csv("initial.csv");
    }

    Uint64 last_time = SDL_GetPerformanceCounter();
    Uint64 freq = SDL_GetPerformanceFrequency();
    float fps = 60.0f;
    float fps_smooth = 0.95f;

    bool running = true;
    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (ev.type == SDL_EVENT_KEY_DOWN) {
                switch (ev.key.key) {
                    case SDLK_Q:
                    case SDLK_ESCAPE:
                        running = false;
                        break;
                    case SDLK_R:
                        if (!scene_file.empty())
                            world.init_from_csv(scene_file, WIDTH, HEIGHT);
                        else
                            world.init(WIDTH, HEIGHT);
                        break;
                    case SDLK_SPACE:
                        world.paused = !world.paused;
                        break;
                    case SDLK_EQUALS:
                        world.restitution = std::min(1.0f, world.restitution + 0.05f);
                        break;
                    case SDLK_MINUS:
                        world.restitution = std::max(0.0f, world.restitution - 0.05f);
                        break;
                    case SDLK_S:
                        world.save_csv("final_positions.csv");
                        break;
                    default: break;
                }
            }
        }

        Uint64 now = SDL_GetPerformanceCounter();
        float dt = (float)(now - last_time) / (float)freq;
        last_time = now;

        if (dt > 1.0f / 15.0f) dt = 1.0f / 15.0f;

        float instant_fps = (dt > 0) ? 1.0f / dt : 60.0f;
        fps = fps * fps_smooth + instant_fps * (1.0f - fps_smooth);

        world.step(dt);
        ren.draw(world, fps);
    }

    ren.shutdown();
    return 0;
}
