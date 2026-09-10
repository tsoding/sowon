#include "digits.h"

#ifdef PENGER
#include "penger_walk_sheet.h"
#endif

#include <math.h>
#include <time.h>

#define FPS 60
#define COLON_INDEX 10
#define SPRITE_CHAR_WIDTH (300 / 2)
#define SPRITE_CHAR_HEIGHT (380 / 2)
#define WIGGLE_COUNT 3
#define WIGGLE_DURATION (0.40f / WIGGLE_COUNT)
#define CHAR_WIDTH (300 / 2)
#define CHAR_HEIGHT (380 / 2)
#define CHARS_COUNT 8
#define TEXT_WIDTH (CHAR_WIDTH * CHARS_COUNT)
#define TEXT_HEIGHT (CHAR_HEIGHT)
#define MAIN_COLOR_R 220
#define MAIN_COLOR_G 220
#define MAIN_COLOR_B 220
#define PAUSE_COLOR_R 220
#define PAUSE_COLOR_G 120
#define PAUSE_COLOR_B 120
#define BACKGROUND_COLOR_R 24
#define BACKGROUND_COLOR_G 24
#define BACKGROUND_COLOR_B 24
#define PENGER_STEPS_PER_SECOND 3
#define PENGER_SCALE 4
#define SCALE_FACTOR 0.15f
#define TITLE_CAP 256
#define DEMO_WINDOW_WIDTH 1085
#define DEMO_WINDOW_HEIGHT 610
#define DEMO_SCALE 0.722500

typedef enum {
    MODE_ASCENDING = 0,
    MODE_COUNTDOWN,
    MODE_CLOCK,
} Mode;

float parse_time(const char *time)
{
    float result = 0.0f;

    while (*time) {
        char *endptr = NULL;
        float x = strtof(time, &endptr);

        if (time == endptr) {
            fprintf(stderr, "`%s` is not a number\n", time);
            exit(1);
        }

        switch (*endptr) {
        case '\0':
        case 's': result += x;                 break;
        case 'm': result += x * 60.0f;         break;
        case 'h': result += x * 60.0f * 60.0f; break;
        default:
            fprintf(stderr, "`%c` is an unknown time unit\n", *endptr);
            exit(1);
        }

        time = endptr;
        if (*time) time += 1;
    }

    return result;
}

typedef struct {
    Mode mode;
    float displayed_time;
    int paused;
    int exit_after_countdown;

    int quit;
    size_t wiggle_index;
    float wiggle_cooldown;
    float user_scale;
    char prev_title[TITLE_CAP];
} State;

void parse_state_from_args(State *state, int argc, char **argv)
{
    memset(state, 0, sizeof(*state));

    state->wiggle_cooldown = WIGGLE_DURATION;
    state->user_scale = 1.0f;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-p") == 0) {
            state->paused = 1;
        } else if (strcmp(argv[i], "-e") == 0) {
            state->exit_after_countdown = 1;
        } else if (strcmp(argv[i], "clock") == 0) {
            state->mode = MODE_CLOCK;
        } else {
            state->mode = MODE_COUNTDOWN;
            state->displayed_time = parse_time(argv[i]);
        }
    }
}

void state_update(State *state, float dt)
{
    if (state->wiggle_cooldown <= 0.0f) {
        state->wiggle_index++;
        state->wiggle_cooldown = WIGGLE_DURATION;
    }
    state->wiggle_cooldown -= dt;

    if (!state->paused) {
        switch (state->mode) {
        case MODE_ASCENDING: {
            // TODOOOOO: display_time should not depend on `dt` AT ALL!
            //
            // Capture some sort of timestamp from the start of the application, and depending on the mode
            // display the time relative to the start accordingly. That way the timer is alway accurate
            // regardless of the FPS.
            //
            // Maybe even wiggle animation should not depend on the `dt`.
            state->displayed_time += dt;
        } break;
        case MODE_COUNTDOWN: {
            if (state->displayed_time > 1e-6) {
                state->displayed_time -= dt;
            } else {
                state->displayed_time = 0.0f;
                if (state->exit_after_countdown) {
                    exit(0);
                }
            }
        } break;
        case MODE_CLOCK: {
            float displayed_time_prev = state->displayed_time;
            time_t t = time(NULL);
            struct tm *tm = localtime(&t);
            state->displayed_time = tm->tm_sec + tm->tm_min  * 60.0f + tm->tm_hour * 60.0f * 60.0f;
            if (state->displayed_time <= displayed_time_prev) {
                // same second, keep previous count and add subsecond resolution for penger
                if (floorf(displayed_time_prev) == floorf(displayed_time_prev+dt)) { // check for no newsecond shenaningans from dt
                    state->displayed_time = displayed_time_prev + dt;
                } else {
                    state->displayed_time = displayed_time_prev;
                }
            }
        } break;
        }
    }
}

void initial_pen(int w, int h, int *pen_x, int *pen_y, float user_scale, float *fit_scale)
{
    float text_aspect_ratio = (float) TEXT_WIDTH / (float) TEXT_HEIGHT;
    float window_aspect_ratio = (float) w / (float) h;
    if(text_aspect_ratio > window_aspect_ratio) {
        *fit_scale = (float) w / (float) TEXT_WIDTH;
    } else {
        *fit_scale = (float) h / (float) TEXT_HEIGHT;
    }

    const int effective_digit_width = (int) floorf((float) CHAR_WIDTH * user_scale * *fit_scale);
    const int effective_digit_height = (int) floorf((float) CHAR_HEIGHT * user_scale * *fit_scale);
    *pen_x = w / 2 - effective_digit_width * CHARS_COUNT / 2;
    *pen_y = h / 2 - effective_digit_height / 2;
}
