#define _CRT_SECURE_NO_WARNINGS
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <SDL.h>

#include "./digits.h"

/*  window    */
#define TITLE_CAP 256
#define SCALE_FACTOR 0.15f

/*  rendering frames  */
#define FPS 60
#define DELTA_TIME (1.0f / FPS)


/*  sprite  */
#define SPRITE_CHAR_WIDTH (300 / 2)
#define SPRITE_CHAR_HEIGHT (380 / 2)

/*  char    */
#define CHAR_WIDTH (300 / 2)
#define CHAR_HEIGHT (380 / 2)
#define CHARS_COUNT 8

/*  initial window size, text size    */
#define TEXT_WIDTH (CHAR_WIDTH * CHARS_COUNT)
#define TEXT_HEIGHT (CHAR_HEIGHT)

#define WIGGLE_COUNT 3
#define WIGGLE_DURATION (0.40f / WIGGLE_COUNT)

#define COLON_INDEX 10


/*  color   */
#define MAIN_COLOR_R 220
#define MAIN_COLOR_G 220
#define MAIN_COLOR_B 220

#define PAUSE_COLOR_R 220
#define PAUSE_COLOR_G 120
#define PAUSE_COLOR_B 120

#define BACKGROUND_COLOR_R 24
#define BACKGROUND_COLOR_G 24
#define BACKGROUND_COLOR_B 24





typedef enum {
    MODE_ASCENDING = 0,
    MODE_COUNTDOWN,
    MODE_CLOCK,
} Mode;

typedef struct Config {
    Mode mode;
    float displayed_time;
    float displayed_time_initial;
    int paused;
    int exit_after_countdown;
    size_t wiggle_index;
    float wiggle_cooldown;
    float user_scale;
    char prev_title[TITLE_CAP];
    int p_flag;
    int w;
    int h;
    float fit_scale;
    int pen_x;
    int pen_y;
} Config;



/********** ERROR HANDLER **********/

void secc(int code)
{
    if (code < 0) {
        fprintf(stderr, "SDL pooped itself: %s\n", SDL_GetError());
        abort();
    }
}

void secp(void *ptr)
{
    if (ptr == NULL) {
        fprintf(stderr, "SDL pooped itself: %s\n", SDL_GetError());
        abort();
    }
}









/********** PARSERS **********/

/*  time parser     */
float parse_time(const char *time) {
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

            // seconds
            case 's':
                result += x;
                break;
            // minutes
            case 'm':
                result += x * 60.0f;
                break;
            // hours
            case 'h':
                result += x * 60.0f * 60.0f;
                break;

            default:
                fprintf(stderr, "`%c` is an unknown time unit\n", *endptr);
                exit(1);
        }

        time = endptr;
        if (*time) time += 1;
    }

    return result;
}


/* argument parser  */
void argumentParser(int argc, char **argv, Config *config) {

    // MODE_ASCENDING: stop watch 
    *config = (Config){MODE_ASCENDING, 
                       0.0f, 
                       0.0f, 
                       0, 
                       0, 
                       0, 
                       WIGGLE_DURATION, 
                       1.0f, 
                       "hello world",
                       0,
                       TEXT_WIDTH,
                       TEXT_HEIGHT,
                       1.0f,
                       0,
                       0};


    

    for (int i = 1; i < argc; ++i) {
        // pause
        if (strcmp(argv[i], "-p") == 0) {
            config->p_flag = 1;
            config->paused = 1;
        } 
        // exist
        else if (strcmp(argv[i], "-e") == 0) {
            config->exit_after_countdown = 1;
        }
        // time clock
        else if (strcmp(argv[i], "clock") == 0) {
            config->mode = MODE_CLOCK;
        }
        // countdown
        else {
            config->mode = MODE_COUNTDOWN;
            config->displayed_time = parse_time(argv[i]);
        }
    }
}









/********** CONFIGURATION **********/

void defaultConfig(Config *config) {

    switch(config->mode) {
        case MODE_ASCENDING:
            break;

        case MODE_COUNTDOWN:
            config->displayed_time_initial = config->displayed_time;
            break;

        case MODE_CLOCK:
            break;
    }
}











/***********  SDL *************/
void initializeSDL() {
    secc(SDL_Init(SDL_INIT_VIDEO));
    secc(SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear"));
}


void createWindow(SDL_Window **window) {
    *window = SDL_CreateWindow(
                     "sowon",
                     0, 0, TEXT_WIDTH, TEXT_HEIGHT,
                     SDL_WINDOW_RESIZABLE);
    secp(window);
}

void createRenderer(SDL_Window *window, SDL_Renderer **renderer) {
    *renderer = SDL_CreateRenderer(window, -1,
                 SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);

    secp(renderer);
}



SDL_Texture *createTextureFromFile(SDL_Renderer *renderer) {
   SDL_Surface* image_surface;
    image_surface =  SDL_CreateRGBSurfaceFrom(
                        png,
                        (int) png_width,
                        (int) png_height,
                        32,
                        (int) png_width * 4,
                        0x000000FF,
                        0x0000FF00,
                        0x00FF0000,
                        0xFF000000);

    secp(image_surface);
    
    SDL_Texture *digits = SDL_CreateTextureFromSurface(renderer, image_surface);

    secp(digits);
    secc(SDL_SetTextureColorMod(digits, MAIN_COLOR_R, MAIN_COLOR_G, MAIN_COLOR_B));
    return digits;
}


void windowSize(SDL_Window *window, int *w, int *h) {
    SDL_GetWindowSize(window, w, h);
}

void fullScreenToggle(SDL_Window *window) {
    Uint32 window_flags;
    secc(window_flags = SDL_GetWindowFlags(window));

    if(window_flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
        secc(SDL_SetWindowFullscreen(window, 0));
    } 
    else {
        secc(SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP));
    }
}


// SELECTS DIGIT FROM DIGITS.PNG DIGITS IMAGE
// digit_index, column
// wiggle_index, row
void srcRect(int digit_index,
             int wiggle_index,
             SDL_Rect *src_rect) {

   *src_rect = (SDL_Rect){(int) (digit_index*SPRITE_CHAR_WIDTH),
                              (int) (wiggle_index*SPRITE_CHAR_HEIGHT),
                              SPRITE_CHAR_WIDTH,
                              SPRITE_CHAR_HEIGHT};
}

void dstRect(int *pen_x,
             int pen_y, 
             float user_scale, 
             float fit_scale, 
             SDL_Rect *dst_rect) {

   // RESIZING DIGIT 
   // transforms digit chosen form image
   // new dimensions
   const int effective_digit_width = (int) floorf((float) CHAR_WIDTH * user_scale * fit_scale);
   const int effective_digit_height = (int) floorf((float) CHAR_HEIGHT * user_scale * fit_scale);


   *dst_rect = (SDL_Rect){*pen_x,
                          pen_y,
                          effective_digit_width,
                          effective_digit_height};

    // new cartesian coordinates of next new digit
   *pen_x += effective_digit_width;
}








/*  choosing subimage from and image    
    and resizing */
void render_digit_at(SDL_Renderer *renderer, 
                     SDL_Texture *digits,
                     SDL_Rect *src_rect,
                     SDL_Rect *dst_rect) {


   // ADDS EACH NEW DIGIT TO RENDERER, ONE BY ONE
   SDL_RenderCopy(renderer, digits, src_rect, dst_rect);
    
}


/*  B   
            https://stackoverflow.com/questions/10279718/append-char-to-string-in-c
            https://www.w3schools.com/c/c_strings.php

        iteration over a c-string
            https://stackoverflow.com/questions/3213827/how-to-iterate-over-a-string-in-c

        number of digits
            https://www.geeksforgeeks.org/program-count-digits-integer-3-different-methods/
*/
void hoursMinutesSeconds(Config *config, char str[9]) {
        // TODO: support amount of hours >99

        const size_t time = (size_t) ceilf(fmaxf(config->displayed_time, 0.0f));
        const size_t hours = time/60/60;
        const size_t minutes = time/60%60;
        const size_t seconds = time % 60;

        /*  hours   */
        const size_t hoursfirstdigit = hours/10;
        const size_t hoursseconddigit = hours%10;

        /*  minutes */
        const size_t minutesfirstdigit = minutes/10;
        const size_t minutesseconddigit = minutes%10;

        /*  seconds */
        const size_t secondsfirstdigit = seconds/10;
        const size_t secondsseconddigit = seconds%10;
        
        str[0] = hoursfirstdigit; 

        str[1] = hoursseconddigit; 

        str[2] = COLON_INDEX;

        str[3] = minutesfirstdigit; 

        str[4] = minutesseconddigit; 

        str[5] = COLON_INDEX;

        str[6] = secondsfirstdigit; 

        str[7] = secondsseconddigit; 

        str[8] = '\0';
        // TODO: support amount of hours >99
}






void createRendering(SDL_Renderer *renderer, 
                     SDL_Texture *digits, 
                     Config *config, 
                     char str[9]) {
        
        // black background color 
        SDL_SetRenderDrawColor(renderer, BACKGROUND_COLOR_R, BACKGROUND_COLOR_G, BACKGROUND_COLOR_B, 255);
        
        // texture colour, digits
        if (config->paused) {
            secc(SDL_SetTextureColorMod(digits, PAUSE_COLOR_R, PAUSE_COLOR_G, PAUSE_COLOR_B));
        } else {
            secc(SDL_SetTextureColorMod(digits, MAIN_COLOR_R, MAIN_COLOR_G, MAIN_COLOR_B));
        }
        
        SDL_RenderClear(renderer);


        SDL_Rect src_rect;
        SDL_Rect dst_rect;
       

        for (int i = 0; i<8; ++i) {
            srcRect(str[i],
                    (config->wiggle_index + i)%WIGGLE_COUNT,
                    &src_rect);

            dstRect(&config->pen_x, config->pen_y,
                    config->user_scale, config->fit_scale, 
                    &dst_rect);

            render_digit_at(renderer, 
                            digits, 
                            &src_rect,
                            &dst_rect);
        }   

}











void timeInWindowTitle(SDL_Window *window, Config *config, char str[9]) {

        /*  print time as window's title */
        char title[TITLE_CAP] ="Hello World!";
        //snprintf(title, sizeof(title), "%02zu:%02zu:%02zu - sowon", hours, minutes, seconds);
        snprintf(title, sizeof(title), "%d%d:%d%d:%d%d - sowon", str[0], str[1], str[3], str[4], str[6], str[7]);

        if (strcmp(config->prev_title, title) != 0) {
            SDL_SetWindowTitle(window, title);
        }

        memcpy(title, config->prev_title, TITLE_CAP);
}

void renderingToScreen(SDL_Renderer *renderer) {
        SDL_RenderPresent(renderer);
}




/*  UPDATE CONFIGURATION STATE  */

/*  pre:
    post: window resize adjusted
*/
void fitScale(int w, int h, float *fit_scale) {

    *fit_scale = 1.0;

    // width/height ratio
    float text_aspect_ratio = (float) TEXT_WIDTH / (float) TEXT_HEIGHT;
    float window_aspect_ratio = (float) w / (float) h;

    if(text_aspect_ratio > window_aspect_ratio) {
        *fit_scale = (float) w / (float) TEXT_WIDTH;
    } else {
        *fit_scale = (float) h / (float) TEXT_HEIGHT;
    }
}


/*  pre:    
    post:   cartesian coordinates
            position where rendering starts 
            to fit CHAR_COUNT characters 
            at user_scale*fit_scale scale
*/
void initial_pen(int w, 
                 int h,
                 float user_scale,
                 float fit_scale, 
                 int *pen_x,
                 int *pen_y) {
    
    // character width after scaling 
    const int effective_digit_width = (int)floorf(
                                            (float)CHAR_WIDTH*user_scale*fit_scale
                                      );
    // character height after scaling
    const int effective_digit_height = (int)floorf(
                                            (float)CHAR_HEIGHT*user_scale*fit_scale
                                       );
    
    // position where rendering starts 
    *pen_x = w/2 - effective_digit_width*CHARS_COUNT/2;
    *pen_y = h/2 - effective_digit_height/2;
}



/*  paused  */

void pauseToggle(Config *config) {
    config->paused = !config->paused;
}

/*  zoom   */

void zoomInitial(Config *config) {
    config->user_scale = 1.0f;
}

/* pre:
   post: zoom in, represente in 'user_scale'
*/
void zoomIn(Config *config) {
    config->user_scale += SCALE_FACTOR*config->user_scale;
}

void zoomOut(Config *config) {
    config->user_scale -= SCALE_FACTOR*config->user_scale;
}


/*  reset clock  */

void resetClock(Config *config, SDL_Texture *digits) {

        config->displayed_time = 0.0f;
        config->paused = 0;
        
        if (config->p_flag) {
            config->paused = 1;
        }
        else {
            config->displayed_time = config->displayed_time_initial;
        }

        if (config->paused) {
            secc(SDL_SetTextureColorMod(digits, PAUSE_COLOR_R, PAUSE_COLOR_G, PAUSE_COLOR_B));
        }
        else {
            secc(SDL_SetTextureColorMod(digits, MAIN_COLOR_R, MAIN_COLOR_G, MAIN_COLOR_B));
        }
}

int quitSDL() {
   SDL_Quit();
   return 0;
}



/*  EVENTS  */

/*  event key down  */

void keyDownCases(SDL_Event event, Config *config, SDL_Texture *digits, SDL_Window *window) {
    // https://www.libsdl.org/release/SDL-1.2.15/docs/html/sdlkey.html
    switch (event.key.keysym.sym) {
        case SDLK_SPACE: {
            pauseToggle(config);
        } 
        break;

        case SDLK_KP_PLUS:

        case SDLK_EQUALS: {
            zoomInitial(config);
        } 
        break;

        case SDLK_KP_MINUS:

        case SDLK_MINUS: {
            zoomOut(config);
        } 
        break;

        case SDLK_KP_0:
        
        // in a spanish keyboard, 'equals key' is <shift+0> for zoom in
        case SDLK_0: {
            if (event.key.keysym.mod & KMOD_SHIFT) {
                zoomIn(config);
            }
            else 
                zoomInitial(config);
        } 
        break;

        case SDLK_F5: {
            resetClock(config, digits);
        } 
        break;

        case SDLK_F11: {
            fullScreenToggle(window);
        } 
        break;
    }
}

/*  mouse wheel */

void mouseWheel(SDL_Event event, Config *config) {
    if (SDL_GetModState() & KMOD_CTRL) {
        if (event.wheel.y > 0) {
            config->user_scale += SCALE_FACTOR * config->user_scale;
        } else if (event.wheel.y < 0) {
            config->user_scale -= SCALE_FACTOR * config->user_scale;
        }
    }
}



/* even loop    */
void eventLoop(int *quit, Config *config, SDL_Window *window, SDL_Texture *digits) {
    SDL_Event event = {0};
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT: {
                *quit = 1;
            } break;

            case SDL_KEYDOWN: {
                keyDownCases(event, config, digits, window);
            } break;

            case SDL_MOUSEWHEEL: {
                mouseWheel(event, config);
            } break;

            default: {
            }
        }
    }
}









// UPDATE

void wiggleCoolDown(Config *config) {
    if (config->wiggle_cooldown <= 0.0f) {
        config->wiggle_index++;
        config->wiggle_cooldown = WIGGLE_DURATION;
    }

    config->wiggle_cooldown -= DELTA_TIME;
}

void updateTime(Config *config) {
    switch (config->mode) {
        case MODE_ASCENDING: {
            config->displayed_time += DELTA_TIME;
        } 
        break;
    
        case MODE_COUNTDOWN: {
            if (config->displayed_time > 1e-6) {
                config->displayed_time -= DELTA_TIME;
            } 
            else {
                config->displayed_time = 0.0f;
                if (config->exit_after_countdown) {
                    quitSDL();
                }
            }
        } 
        break;
    
        case MODE_CLOCK: {
            time_t t = time(NULL);
            struct tm *tm = localtime(&t);
            config->displayed_time = tm->tm_sec
                           + tm->tm_min  * 60.0f
                           + tm->tm_hour * 60.0f * 60.0f;
        } 
        break;
    }
}

void updateConfig(SDL_Window *window, Config *config) {

        // window width and height
        windowSize(window, &config->w, &config->h);

        // widow resize 
        fitScale(config->w, config->h, &config->fit_scale);
        
        // pen
        initial_pen(config->w,
                    config->h,
                    config->user_scale,
                    config->fit_scale,
                    &config->pen_x, 
                    &config->pen_y);

        wiggleCoolDown(config);

        if (!config->paused) {
            updateTime(config);
        }
}

// INFINITE LOOP
void infiniteLoop(SDL_Window *window, SDL_Renderer *renderer, SDL_Texture *digits, Config *config) {
    int quit = 0;
    while (!quit) {

        eventLoop(&quit, config, window, digits);
        
        char str[9];
        hoursMinutesSeconds(config, str);
        createRendering(renderer, digits, config, str);


        timeInWindowTitle(window, config, str);
        renderingToScreen(renderer);
        
        updateConfig(window, config);

        SDL_Delay((int) floorf(DELTA_TIME * 1000.0f));


    }
}




/*  MAIN    */
int main(int argc, char **argv) {
    
    Config config;
    argumentParser(argc, argv, &config);
    defaultConfig(&config);


    initializeSDL();

    SDL_Window *window;
    createWindow(&window);

    SDL_Renderer *renderer;
    createRenderer(window, &renderer);

    SDL_Texture *digits;
    digits = createTextureFromFile(renderer);

    

    infiniteLoop(window, renderer, digits, &config);
    
    quitSDL();

    return 0;
}
