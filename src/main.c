#define _CRT_SECURE_NO_WARNINGS
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <SDL2/SDL.h>

#include "common.c"

void secc(int code)
{
    if (code < 0) {
        fprintf(stderr, "SDL pooped itself: %s\n", SDL_GetError());
        abort();
    }
}

void *secp(void *ptr)
{
    if (ptr == NULL) {
        fprintf(stderr, "SDL pooped itself: %s\n", SDL_GetError());
        abort();
    }

    return ptr;
}

SDL_Surface *load_png_file_as_surface(uint32_t *data, size_t width, size_t height)
{
    SDL_Surface* image_surface =
        secp(SDL_CreateRGBSurfaceFrom(
                 data,
                 (int) width,
                 (int) height,
                 32,
                 (int) width * 4,
                 0x000000FF,
                 0x0000FF00,
                 0x00FF0000,
                 0xFF000000));
    return image_surface;
}

SDL_Texture *load_digits_png_file_as_texture(SDL_Renderer *renderer)
{
    SDL_Surface *image_surface = load_png_file_as_surface(digits_data, digits_width, digits_height);
    return secp(SDL_CreateTextureFromSurface(renderer, image_surface));
}

#ifdef PENGER
SDL_Texture *load_penger_png_file_as_texture(SDL_Renderer *renderer)
{
    SDL_Surface *image_surface = load_png_file_as_surface(penger_data, penger_width, penger_height);
    return secp(SDL_CreateTextureFromSurface(renderer, image_surface));
}
#endif

void render_digit_at(SDL_Renderer *renderer, SDL_Texture *digits, size_t digit_index,
                     size_t wiggle_index, int *pen_x, int *pen_y, float user_scale, float fit_scale)
{
    const int effective_digit_width = (int) floorf((float) CHAR_WIDTH * user_scale * fit_scale);
    const int effective_digit_height = (int) floorf((float) CHAR_HEIGHT * user_scale * fit_scale);

    const SDL_Rect src_rect = {
        (int) (digit_index * SPRITE_CHAR_WIDTH),
        (int) (wiggle_index * SPRITE_CHAR_HEIGHT),
        SPRITE_CHAR_WIDTH,
        SPRITE_CHAR_HEIGHT
    };
    const SDL_Rect dst_rect = {
        *pen_x,
        *pen_y,
        effective_digit_width,
        effective_digit_height
    };
    SDL_RenderCopy(renderer, digits, &src_rect, &dst_rect);
    *pen_x += effective_digit_width;
}

#ifdef PENGER
void render_penger_at(SDL_Renderer *renderer, SDL_Texture *penger, float time, int flipped, SDL_Window *window)
{
    int window_width, window_height;
    SDL_GetWindowSize(window, &window_width, &window_height);

    int sps  = PENGER_STEPS_PER_SECOND;

    int step = (int)(time*sps)%(60*sps); //step index [0,60*sps-1]

    float progress  = step/(60.0*sps); // [0,1]

    int frame_index = step%2;

    float penger_drawn_width = ((float)penger_width / 2) / PENGER_SCALE;

    float penger_walk_width = window_width + penger_drawn_width;

    const SDL_Rect src_rect = {
        (int) (penger_width / 2) * frame_index,
        0,
        (int) penger_width / 2,
        (int) penger_height
    };

    SDL_Rect dst_rect = {
        floorf((float)penger_walk_width * progress - penger_drawn_width),
        window_height - (penger_height / PENGER_SCALE),
        (int) (penger_width / 2) / PENGER_SCALE,
        (int) penger_height / PENGER_SCALE
    };

    SDL_RenderCopyEx(renderer, penger, &src_rect, &dst_rect, 0, NULL, flipped);
}
#endif

int main(int argc, char **argv)
{
    State state = {0};
    parse_state_from_args(&state, argc, argv);

    secc(SDL_Init(SDL_INIT_VIDEO));

    SDL_Window *window =
        secp(SDL_CreateWindow(
                 "sowon",
                 0, 0,
                 TEXT_WIDTH, TEXT_HEIGHT*2,
                 SDL_WINDOW_RESIZABLE));

    SDL_Renderer *renderer =
        secp(SDL_CreateRenderer(
                 window, -1,
                 SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED));

    secc(SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear"));

    SDL_Texture *digits = load_digits_png_file_as_texture(renderer);

    #ifdef PENGER
    SDL_Texture *penger = load_penger_png_file_as_texture(renderer);
    #endif

    secc(SDL_SetTextureColorMod(digits, MAIN_COLOR_R, MAIN_COLOR_G, MAIN_COLOR_B));

    if (state.paused) {
        secc(SDL_SetTextureColorMod(digits, PAUSE_COLOR_R, PAUSE_COLOR_G, PAUSE_COLOR_B));
    } else {
        secc(SDL_SetTextureColorMod(digits, MAIN_COLOR_R, MAIN_COLOR_G, MAIN_COLOR_B));
    }

    uint64_t last_time = SDL_GetPerformanceCounter();
    while (!state.quit) {
        uint64_t now = SDL_GetPerformanceCounter();
        float dt = (float)(now - last_time)/SDL_GetPerformanceFrequency();
        last_time = now;

        // INPUT BEGIN //////////////////////////////
        SDL_Event event = {0};
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT: {
                state.quit = 1;
            } break;

            case SDL_KEYDOWN: {
                switch (event.key.keysym.sym) {
                case SDLK_SPACE: {
                    state.paused = !state.paused;
                    if (state.paused) {
                        secc(SDL_SetTextureColorMod(digits, PAUSE_COLOR_R, PAUSE_COLOR_G, PAUSE_COLOR_B));
                    } else {
                        secc(SDL_SetTextureColorMod(digits, MAIN_COLOR_R, MAIN_COLOR_G, MAIN_COLOR_B));
                    }
                } break;

                case SDLK_KP_PLUS:
                case SDLK_EQUALS: {
                    state.user_scale += SCALE_FACTOR * state.user_scale;
                } break;

                case SDLK_KP_MINUS:
                case SDLK_MINUS: {
                    state.user_scale -= SCALE_FACTOR * state.user_scale;
                } break;

                case SDLK_KP_0:
                case SDLK_0: {
                    state.user_scale = 1.0f;
                } break;

                case SDLK_F5: {
                    parse_state_from_args(&state, argc, argv);
                    if (state.paused) {
                        secc(SDL_SetTextureColorMod(digits, PAUSE_COLOR_R, PAUSE_COLOR_G, PAUSE_COLOR_B));
                    } else {
                        secc(SDL_SetTextureColorMod(digits, MAIN_COLOR_R, MAIN_COLOR_G, MAIN_COLOR_B));
                    }
                } break;

                case SDLK_F11: {
                    Uint32 window_flags;
                    secc(window_flags = SDL_GetWindowFlags(window));
                    if(window_flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
                        secc(SDL_SetWindowFullscreen(window, 0));
                    } else {
                        secc(SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP));
                    }
                } break;
                }
            } break;

            case SDL_MOUSEWHEEL: {
                if (SDL_GetModState() & KMOD_CTRL) {
                    if (event.wheel.y > 0) {
                        state.user_scale += SCALE_FACTOR * state.user_scale;
                    } else if (event.wheel.y < 0) {
                        state.user_scale -= SCALE_FACTOR * state.user_scale;
                    }
                }
            } break;

            default: {}
            }
        }
        // INPUT END //////////////////////////////

        // RENDER BEGIN //////////////////////////////
        SDL_SetRenderDrawColor(renderer, BACKGROUND_COLOR_R, BACKGROUND_COLOR_G, BACKGROUND_COLOR_B, 255);
        SDL_RenderClear(renderer);
        {
            const size_t t = (size_t) floorf(fmaxf(state.displayed_time, 0.0f));
            // PENGER BEGIN //////////////////////////////

            #ifdef PENGER
            render_penger_at(renderer, penger, state.displayed_time, state.mode==MODE_COUNTDOWN, window);
            #endif

            // PENGER END //////////////////////////////

            // DIGITS BEGIN //////////////////////////////
            int pen_x, pen_y;
            float fit_scale = 1.0;
            int w, h;
            SDL_GetWindowSize(window, &w, &h);
            initial_pen(w, h, &pen_x, &pen_y, state.user_scale, &fit_scale);

            // TODO: support amount of hours >99
            const size_t hours = t / 60 / 60;
            render_digit_at(renderer, digits, hours / 10,   state.wiggle_index      % WIGGLE_COUNT, &pen_x, &pen_y, state.user_scale, fit_scale);
            render_digit_at(renderer, digits, hours % 10,  (state.wiggle_index + 1) % WIGGLE_COUNT, &pen_x, &pen_y, state.user_scale, fit_scale);
            render_digit_at(renderer, digits, COLON_INDEX,  state.wiggle_index      % WIGGLE_COUNT, &pen_x, &pen_y, state.user_scale, fit_scale);

            const size_t minutes = t / 60 % 60;
            render_digit_at(renderer, digits, minutes / 10, (state.wiggle_index + 2) % WIGGLE_COUNT, &pen_x, &pen_y, state.user_scale, fit_scale);
            render_digit_at(renderer, digits, minutes % 10, (state.wiggle_index + 3) % WIGGLE_COUNT, &pen_x, &pen_y, state.user_scale, fit_scale);
            render_digit_at(renderer, digits, COLON_INDEX,  (state.wiggle_index + 1) % WIGGLE_COUNT, &pen_x, &pen_y, state.user_scale, fit_scale);

            const size_t seconds = t % 60;
            render_digit_at(renderer, digits, seconds / 10, (state.wiggle_index + 4) % WIGGLE_COUNT, &pen_x, &pen_y, state.user_scale, fit_scale);
            render_digit_at(renderer, digits, seconds % 10, (state.wiggle_index + 5) % WIGGLE_COUNT, &pen_x, &pen_y, state.user_scale, fit_scale);

            char title[TITLE_CAP];
            snprintf(title, sizeof(title), "%02zu:%02zu:%02zu - sowon", hours, minutes, seconds);
            if (strcmp(state.prev_title, title) != 0) {
                SDL_SetWindowTitle(window, title);
            }
            memcpy(title, state.prev_title, TITLE_CAP);
            // DIGITS END //////////////////////////////
        }
        SDL_RenderPresent(renderer);
        // RENDER END //////////////////////////////

        // UPDATE BEGIN //////////////////////////////
        state_update(&state, dt);
        // UPDATE END //////////////////////////////

        now = SDL_GetPerformanceCounter();
        Uint32 frame_time = (Uint32)(((now - last_time)*1000.0f)/SDL_GetPerformanceFrequency());
        Uint32 frame_cap = 1000/FPS;
        if (frame_time < frame_cap) SDL_Delay(frame_cap - frame_time);
    }

    SDL_Quit();

    return 0;
}
