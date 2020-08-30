#include <stdio.h>
#include <stdlib.h>

#include <SDL.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define FPS 60
#define DELTA_TIME (1.0f / FPS)
#define SPRITE_DIGIT_WIDTH (300 / 2)
#define SPRITE_DIGIT_HEIGHT (380 / 2)
#define DIGIT_WIDTH (300 / 2)
#define DIGIT_HEIGHT (380 / 2)
#define DIGITS_COUNT 8
#define TEXT_WIDTH (DIGIT_WIDTH * DIGITS_COUNT)
#define TEXT_HEIGHT (DIGIT_HEIGHT)
#define WIGGLE_COUNT 3
#define WIGGLE_DURATION (0.40 / WIGGLE_COUNT)
#define COLON_INDEX 10
#define MAIN_COLOR_R 220
#define MAIN_COLOR_G 220
#define MAIN_COLOR_B 220
#define PAUSE_COLOR_R 220
#define PAUSE_COLOR_G 120
#define PAUSE_COLOR_B 120
#define BACKGROUND_COLOR_R 24
#define BACKGROUND_COLOR_G 24
#define BACKGROUND_COLOR_B 24
#define SCALE_FACTOR 0.15f

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

SDL_Surface *load_png_file_as_surface(const char *image_filename)
{
    int width, height;
    uint32_t *image_pixels = (uint32_t *) stbi_load(image_filename, &width, &height, NULL, 4);
    if (image_pixels == NULL) {
        fprintf(stderr, "[ERROR] Could not load `%s` as PNG\n", image_filename);
        abort();
    }

    SDL_Surface* image_surface =
        secp(SDL_CreateRGBSurfaceFrom(
                 image_pixels,
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

SDL_Texture *load_png_file_as_texture(SDL_Renderer *renderer,
                                      const char *image_filename)
{
    SDL_Surface *image_surface = load_png_file_as_surface(image_filename);
    return secp(SDL_CreateTextureFromSurface(renderer, image_surface));
}

void render_digit_at(SDL_Renderer *renderer, SDL_Texture *digits, size_t digit_index,
                     size_t wiggle_index, int *pen_x, int *pen_y, float user_scale, float fit_scale)
{
    const int effective_digit_width = (int) floorf((float) DIGIT_WIDTH * user_scale * fit_scale);
    const int effective_digit_height = (int) floorf((float) DIGIT_HEIGHT * user_scale * fit_scale);

    const SDL_Rect src_rect = {
        (int) (digit_index * SPRITE_DIGIT_WIDTH),
        (int) (wiggle_index * SPRITE_DIGIT_HEIGHT),
        SPRITE_DIGIT_WIDTH,
        SPRITE_DIGIT_HEIGHT
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

void initial_pen(SDL_Window *window, int *pen_x, int *pen_y, float user_scale, float *fit_scale)
{
    int w, h;
    SDL_GetWindowSize(window, &w, &h);

    float text_aspect_ratio = (float) TEXT_WIDTH / (float) TEXT_HEIGHT;
    float window_aspect_ratio = (float) w / (float) h;
    if(text_aspect_ratio > window_aspect_ratio) {
        *fit_scale = (float) w / (float) TEXT_WIDTH;
    } else {
        *fit_scale = (float) h / (float) TEXT_HEIGHT;
    }

    const int effective_digit_width = (int) floorf((float) DIGIT_WIDTH * user_scale * *fit_scale);
    const int effective_digit_height = (int) floorf((float) DIGIT_HEIGHT * user_scale * *fit_scale);
    *pen_x = w / 2 - effective_digit_width * DIGITS_COUNT / 2;
    *pen_y = h / 2 - effective_digit_height / 2;
}

int main(int argc, char **argv)
{
    int ascending = 1;
    float time = 0.0f;

    if (argc > 1) {
        ascending = 0;
        time = strtof(argv[1], NULL);
    }

    secc(SDL_Init(SDL_INIT_VIDEO));

    SDL_Window *window =
        secp(SDL_CreateWindow(
                 "sowon",
                 0, 0, TEXT_WIDTH, TEXT_HEIGHT,
                 SDL_WINDOW_RESIZABLE));

    SDL_Renderer *renderer =
        secp(SDL_CreateRenderer(
                 window, -1,
                 SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED));

    secc(SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear"));

    SDL_Texture *digits = load_png_file_as_texture(renderer, "./digits.png");
    secc(SDL_SetTextureColorMod(digits, MAIN_COLOR_R, MAIN_COLOR_G, MAIN_COLOR_B));

    int quit = 0;
    size_t wiggle_index = 0;
    float wiggle_cooldown = WIGGLE_DURATION;
    int paused = 0;
    float user_scale = 1.0f;
    while (!quit) {
        // INPUT BEGIN //////////////////////////////
        SDL_Event event = {0};
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT: {
                quit = 1;
            } break;

            case SDL_KEYDOWN: {
                switch (event.key.keysym.sym) {
                case SDLK_SPACE: {
                    paused = !paused;
                    if (paused) {
                        secc(SDL_SetTextureColorMod(digits, PAUSE_COLOR_R, PAUSE_COLOR_G, PAUSE_COLOR_B));
                    } else {
                        secc(SDL_SetTextureColorMod(digits, MAIN_COLOR_R, MAIN_COLOR_G, MAIN_COLOR_B));
                    }
                } break;

                case SDLK_KP_PLUS:
                case SDLK_EQUALS: {
                    user_scale += SCALE_FACTOR * user_scale;
                } break;

                case SDLK_KP_MINUS:
                case SDLK_MINUS: {
                    user_scale -= SCALE_FACTOR * user_scale;
                } break;

                case SDLK_KP_0:
                case SDLK_0: {
                    user_scale = 1.0f;
                } break;
                }
            } break;

            case SDL_MOUSEWHEEL: {
                if(event.wheel.y > 0)
                {
                    user_scale += SCALE_FACTOR * user_scale;
                }
                else if(event.wheel.y < 0)
                {
                    user_scale -= SCALE_FACTOR * user_scale;
                }
            }

            default: {}
            }
        }
        // INPUT END //////////////////////////////

        // RENDER BEGIN //////////////////////////////
        SDL_SetRenderDrawColor(renderer, BACKGROUND_COLOR_R, BACKGROUND_COLOR_G, BACKGROUND_COLOR_B, 255);
        SDL_RenderClear(renderer);
        {
            int pen_x, pen_y;
            float fit_scale = 1.0;
            initial_pen(window, &pen_x, &pen_y, user_scale, &fit_scale);

            const size_t t = (size_t) ceilf(fmaxf(time, 0.0f));

            const size_t hours = t / 60 / 60;
            render_digit_at(renderer, digits, hours / 10, wiggle_index, &pen_x, &pen_y, user_scale, fit_scale);
            render_digit_at(renderer, digits, hours % 10, wiggle_index, &pen_x, &pen_y, user_scale, fit_scale);
            render_digit_at(renderer, digits, COLON_INDEX, wiggle_index, &pen_x, &pen_y, user_scale, fit_scale);

            const size_t minutes = t / 60 % 60;
            render_digit_at(renderer, digits, minutes / 10, wiggle_index, &pen_x, &pen_y, user_scale, fit_scale);
            render_digit_at(renderer, digits, minutes % 10, wiggle_index, &pen_x, &pen_y, user_scale, fit_scale);
            render_digit_at(renderer, digits, COLON_INDEX, wiggle_index, &pen_x, &pen_y, user_scale, fit_scale);

            const size_t seconds = t % 60;
            render_digit_at(renderer, digits, seconds / 10, wiggle_index, &pen_x, &pen_y, user_scale, fit_scale);
            render_digit_at(renderer, digits, seconds % 10, wiggle_index, &pen_x, &pen_y, user_scale, fit_scale);
        }
        SDL_RenderPresent(renderer);
        // RENDER END //////////////////////////////

        // UPDATE BEGIN //////////////////////////////
        if (wiggle_cooldown <= 0.0f) {
            wiggle_index = (wiggle_index + 1) % WIGGLE_COUNT;
            wiggle_cooldown = WIGGLE_DURATION;
        }
        wiggle_cooldown -= DELTA_TIME;

        if (!paused) {
            if (ascending) {
                time += DELTA_TIME;
            } else {
                if (time > 1e-6) {
                    time -= DELTA_TIME;
                } else {
                    time = 0.0f;
                }
            }
        }
        // UPDATE END //////////////////////////////

        SDL_Delay((int) floorf(DELTA_TIME * 1000.0f));
    }

    SDL_Quit();

    return 0;
}
