#include <stdio.h>
#include <stdlib.h>

#include <SDL.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

const size_t SCREEN_WIDTH = 800;
const size_t SCREEN_HEIGHT = 600;
const size_t FPS = 60;
const float DELTA_TIME = 1.0f / (float) FPS;
const size_t SPRITE_DIGIT_WIDTH = 300 / 2;
const size_t SPRITE_DIGIT_HEIGHT = 380 / 2;
const size_t DIGIT_WIDTH = 300 / 2;
const size_t DIGIT_HEIGHT = 380 / 2;
const size_t DIGITS_COUNT = 11;
const size_t WIGGLE_COUNT = 3;
const float WIGGLE_DURATION = 0.40 / (float) WIGGLE_COUNT;
const size_t COLON_INDEX = 10;
const SDL_Color MAIN_COLOR = {220, 220, 220, 255};
const SDL_Color PAUSE_COLOR = {220, 120, 120, 255};
const SDL_Color BACKGROUND_COLOR = {24, 24, 24, 255};
const float SCALE_FACTOR = 0.15f;

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
                     size_t wiggle_index, int *pen_x, int *pen_y, float scale)
{
    const int effective_digit_width = (int) floorf((float) DIGIT_WIDTH * scale);
    const int effective_digit_height = (int) floorf((float) DIGIT_HEIGHT * scale);

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

void initial_pen(SDL_Window *window, int *pen_x, int *pen_y, float scale)
{
    int w, h;
    SDL_GetWindowSize(window, &w, &h);

    const int effective_digit_width = (int) floorf((float) DIGIT_WIDTH * scale);
    const int effective_digit_height = (int) floorf((float) DIGIT_HEIGHT * scale);
    *pen_x = w / 2 - effective_digit_width * 8 / 2;
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
                 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                 SDL_WINDOW_RESIZABLE));

    SDL_Renderer *renderer =
        secp(SDL_CreateRenderer(
                 window, -1,
                 SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED));

    secc(SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear"));

    SDL_Texture *digits = load_png_file_as_texture(renderer, "./digits.png");
    secc(SDL_SetTextureColorMod(digits, MAIN_COLOR.r, MAIN_COLOR.g, MAIN_COLOR.b));

    int quit = 0;
    size_t wiggle_index = 0;
    float wiggle_cooldown = WIGGLE_DURATION;
    int paused = 0;
    float scale = 1.0f;
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
                        secc(SDL_SetTextureColorMod(digits, PAUSE_COLOR.r, PAUSE_COLOR.g, PAUSE_COLOR.b));
                    } else {
                        secc(SDL_SetTextureColorMod(digits, MAIN_COLOR.r, MAIN_COLOR.g, MAIN_COLOR.b));
                    }
                } break;

                case SDLK_EQUALS: {
                    scale += SCALE_FACTOR * scale;
                } break;

                case SDLK_MINUS: {
                    scale -= SCALE_FACTOR * scale;
                } break;

                case SDLK_0: {
                    scale = 1.0f;
                } break;
                }
            } break;

            default: {}
            }
        }
        // INPUT END //////////////////////////////

        // RENDER BEGIN //////////////////////////////
        SDL_SetRenderDrawColor(renderer, BACKGROUND_COLOR.r, BACKGROUND_COLOR.g, BACKGROUND_COLOR.b, 255);
        SDL_RenderClear(renderer);
        {
            int pen_x, pen_y;
            initial_pen(window, &pen_x, &pen_y, scale);

            const size_t t = (size_t) ceilf(fmaxf(time, 0.0f));

            const size_t hours = t / 60 / 60;
            render_digit_at(renderer, digits, hours / 10, wiggle_index, &pen_x, &pen_y, scale);
            render_digit_at(renderer, digits, hours % 10, wiggle_index, &pen_x, &pen_y, scale);
            render_digit_at(renderer, digits, COLON_INDEX, wiggle_index, &pen_x, &pen_y, scale);

            const size_t minutes = t / 60 % 60;
            render_digit_at(renderer, digits, minutes / 10, wiggle_index, &pen_x, &pen_y, scale);
            render_digit_at(renderer, digits, minutes % 10, wiggle_index, &pen_x, &pen_y, scale);
            render_digit_at(renderer, digits, COLON_INDEX, wiggle_index, &pen_x, &pen_y, scale);

            const size_t seconds = t % 60;
            render_digit_at(renderer, digits, seconds / 10, wiggle_index, &pen_x, &pen_y, scale);
            render_digit_at(renderer, digits, seconds % 10, wiggle_index, &pen_x, &pen_y, scale);
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
