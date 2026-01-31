#define _CRT_SECURE_NO_WARNINGS
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define OEMRESOURCE
#include <windows.h>
#endif

#ifndef __APPLE__
#include <GL/gl.h>
#else
#include <OpenGL/gl.h>
#endif

#define RGFW_IMPLEMENTATION
#define RGFW_OPENGL
#include "RGFW.h"
typedef struct { i32 x, y, w, h; } RGFW_rect;

#ifdef _WIN32
void RGFW_sleep(u64 ms) {
	Sleep((u32)ms);
}

u64 RGFW_getTimerFreq(void) {
	static u64 frequency = 0;
	if (frequency == 0) QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);

	return frequency;
}

u64 RGFW_getTimerValue(void) {
	u64 value;
	QueryPerformanceCounter((LARGE_INTEGER*)&value);
	return value;
}
#else
// TODO: make RGFW_getTimerFreq, RGFW_getTimerValue, RGFW_sleep compile on the rest of the platforms (Windows, MacOS)
void RGFW_sleep(u64 ms) {
	struct timespec time;
	time.tv_sec = 0;
	time.tv_nsec = (long int)((double)ms * 1e+6);

	#ifndef RGFW_NO_UNIX_CLOCK
	nanosleep(&time, NULL);
	#endif
}

u64 RGFW_getTimerFreq(void) { return 1000000000LLU; }
u64 RGFW_getTimerValue(void) {
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
    return (u64)ts.tv_sec * RGFW_getTimerFreq() + (u64)ts.tv_nsec;
}

#endif // _WIN32

#define RGL_LOAD_IMPLEMENTATION
#include "rglLoad.h"

#include "common.c"

const char *vert_shader_source =
    "#version 330\n"
    "precision mediump float;\n"
    "uniform vec2 scr_size;\n"
    "uniform vec4 dst_rect;\n"
    "out vec2 uv;\n"
    "void main(void)\n"
    "{\n"
    "    uv.x = (gl_VertexID & 1);\n"
    "    uv.y = ((gl_VertexID >> 1) & 1);\n"
    "    vec2 p = (dst_rect.xy + dst_rect.zw*uv)/scr_size;\n"
    "    p.y = 1.0 - p.y;\n"
    "    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);\n"
    "}\n";

const char *frag_shader_source =
    "#version 330\n"
    "precision mediump float;\n"
    "uniform sampler2D tex;\n"
    "uniform vec2 tex_size;\n"
    "uniform vec4 src_rect;\n"
    "uniform vec4 color_mod;\n"
    "in vec2 uv;\n"
    "out vec4 out_color;\n"
    "void main(void) {\n"
    "    vec2 coord = (src_rect.xy + src_rect.zw*uv)/tex_size;\n"
    "    out_color = texture(tex, coord)*color_mod;\n"
    "}\n";

const char *shader_type_as_cstr(GLuint shader)
{
    switch (shader) {
    case GL_VERTEX_SHADER:
        return "GL_VERTEX_SHADER";
    case GL_FRAGMENT_SHADER:
        return "GL_FRAGMENT_SHADER";
    default:
        return "(Unknown)";
    }
}

bool compile_shader_source(const GLchar *source, GLenum shader_type, GLuint *shader)
{
    *shader = glCreateShader(shader_type);
    glShaderSource(*shader, 1, &source, NULL);
    glCompileShader(*shader);

    GLint compiled = 0;
    glGetShaderiv(*shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled) {
        GLchar message[1024];
        GLsizei message_size = 0;
        glGetShaderInfoLog(*shader, sizeof(message), &message_size, message);
        fprintf(stderr, "ERROR: could not compile %s\n", shader_type_as_cstr(shader_type));
        fprintf(stderr, "%.*s\n", message_size, message);
        return false;
    }

    return true;
}

bool link_program(GLuint vert_shader, GLuint frag_shader, GLuint *program)
{
    *program = glCreateProgram();

    glAttachShader(*program, vert_shader);
    glAttachShader(*program, frag_shader);
    glLinkProgram(*program);

    GLint linked = 0;
    glGetProgramiv(*program, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLsizei message_size = 0;
        GLchar message[1024];

        glGetProgramInfoLog(*program, sizeof(message), &message_size, message);
        fprintf(stderr, "Program Linking: %.*s\n", message_size, message);
        return false;
    }

    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);

    return true;
}


GLint load_image_data_as_gl_texture(uint32_t *data, size_t width, size_t height)
{
    static GLint texture_units_count = 0;
    if (texture_units_count >= GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS) return -1;

    GLint texture_unit = texture_units_count++;
    GLuint texture;
    glActiveTexture(GL_TEXTURE0 + texture_unit);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 width,
                 height,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 data);
    return texture_unit;
}

GLint tex_uni;
GLint dst_rect_uni;
GLint scr_size_uni;
GLint src_rect_uni;
GLint tex_size_uni;
GLint color_mod_uni;

void set_texture_color_mod(GLfloat r, GLfloat g, GLfloat b)
{
    glUniform4f(color_mod_uni, r, g, b, 1);
}

void texture_copy(GLint texture_unit, int tex_width, int tex_height, RGFW_rect src_rect, RGFW_rect dst_rect)
{
    glUniform1i(tex_uni, texture_unit);
    glUniform4f(dst_rect_uni, dst_rect.x, dst_rect.y, dst_rect.w, dst_rect.h);
    glUniform4f(src_rect_uni, src_rect.x, src_rect.y, src_rect.w, src_rect.h);
    glUniform2f(tex_size_uni, tex_width, tex_height);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void render_digit_at(GLint digits_tex_unit, size_t digit_index, size_t wiggle_index, int *pen_x, int *pen_y, float user_scale, float fit_scale)
{
    const int effective_digit_width = (int) floorf((float) CHAR_WIDTH * user_scale * fit_scale);
    const int effective_digit_height = (int) floorf((float) CHAR_HEIGHT * user_scale * fit_scale);

    const RGFW_rect src_rect = {
        (int) (digit_index * SPRITE_CHAR_WIDTH),
        (int) (wiggle_index * SPRITE_CHAR_HEIGHT),
        SPRITE_CHAR_WIDTH,
        SPRITE_CHAR_HEIGHT
    };
    const RGFW_rect dst_rect = {
        *pen_x,
        *pen_y,
        effective_digit_width,
        effective_digit_height
    };

    texture_copy(digits_tex_unit, digits_width, digits_height, src_rect, dst_rect);

    *pen_x += effective_digit_width;
}

#ifdef PENGER
void render_penger_at(GLint penger_tex_unit, int window_width, int window_height, float time, int flipped)
{
    int sps  = PENGER_STEPS_PER_SECOND;

    int step = (int)(time*sps)%(60*sps); //step index [0,60*sps-1]

    float progress  = step/(60.0*sps); // [0,1]

    int frame_index = step%2;

    float penger_drawn_width = ((float)penger_width / 2) / PENGER_SCALE;

    float penger_walk_width = window_width + penger_drawn_width;

    RGFW_rect src_rect = {
        (int) (penger_width / 2) * frame_index,
        0,
        (int) penger_width / 2,
        (int) penger_height
    };

    RGFW_rect dst_rect = {
        floorf((float)penger_walk_width * progress - penger_drawn_width),
        window_height - (penger_height / PENGER_SCALE),
        (int) (penger_width / 2) / PENGER_SCALE,
        (int) penger_height / PENGER_SCALE
    };

    if (flipped) {
        src_rect.x += src_rect.w;
        src_rect.w *= -1;
    }
    texture_copy(penger_tex_unit, penger_width, penger_height, src_rect, dst_rect);
}
#endif

int main(int argc, char **argv)
{
    State state = {0};

    parse_state_from_args(&state, argc, argv);

    RGFW_glHints *hints = RGFW_getGlobalHints_OpenGL();
    hints->profile = RGFW_glCore;
    hints->major = 3;
    hints->minor = 3;
    RGFW_setGlobalHints_OpenGL(hints);

    RGFW_rect win_rect = {0, 0, TEXT_WIDTH, TEXT_HEIGHT*2};
    RGFW_window* win = RGFW_createWindow("sowon (RGFW)", win_rect.x, win_rect.y, win_rect.w, win_rect.h, RGFW_windowOpenGL);

    RGFW_window_makeCurrentContext_OpenGL(win);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLuint vert_shader;
    if (!compile_shader_source(vert_shader_source, GL_VERTEX_SHADER, &vert_shader)) return 1;
    GLuint frag_shader;
    if (!compile_shader_source(frag_shader_source, GL_FRAGMENT_SHADER, &frag_shader)) return 1;
    GLuint program;
    if (!link_program(vert_shader, frag_shader, &program)) return 1;
    glUseProgram(program);

    tex_uni       = glGetUniformLocation(program, "tex");
    dst_rect_uni  = glGetUniformLocation(program, "dst_rect");
    scr_size_uni  = glGetUniformLocation(program, "scr_size");
    src_rect_uni  = glGetUniformLocation(program, "src_rect");
    tex_size_uni  = glGetUniformLocation(program, "tex_size");
    color_mod_uni = glGetUniformLocation(program, "color_mod");

    glUniform4f(dst_rect_uni, 0, 0, 500, 500);
    glUniform2f(scr_size_uni, win_rect.w, win_rect.h);
    glUniform4f(src_rect_uni, 0, 0, CHAR_WIDTH, CHAR_HEIGHT);
    glUniform2f(tex_size_uni, digits_width, digits_height);

    GLint digits_tex_unit = load_image_data_as_gl_texture(digits_data, digits_width, digits_height);
    #ifdef PENGER
    GLint penger_tex_unit = load_image_data_as_gl_texture(penger_data, penger_width, penger_height);
    #endif

    set_texture_color_mod(MAIN_COLOR_R/255.0f, MAIN_COLOR_G/255.0f, MAIN_COLOR_B/255.0f);
    if (state.paused) {
        set_texture_color_mod(PAUSE_COLOR_R/255.0f, PAUSE_COLOR_G/255.0f, PAUSE_COLOR_B/255.0f);
    } else {
        set_texture_color_mod(MAIN_COLOR_R/255.0f, MAIN_COLOR_G/255.0f, MAIN_COLOR_B/255.0f);
    }

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    bool should_close = false;
    uint64_t last_time = RGFW_getTimerValue();
    while (RGFW_window_shouldClose(win) == RGFW_FALSE) {
        if (should_close) break;

        uint64_t now = RGFW_getTimerValue();
        float dt = (float)(now - last_time)/RGFW_getTimerFreq();
        last_time = now;

        // INPUT BEGIN //////////////////////////////
        RGFW_event event = {0};
        while (RGFW_window_checkEvent(win, &event)) {
            switch (event.type) {
            case RGFW_windowResized: {
                glViewport(0, 0, win->w, win->h);
                glUniform2f(scr_size_uni, win->w, win->h);
            } break;
            case RGFW_keyPressed: {
                switch (event.key.value) {
                case RGFW_space: {
                    state.paused = !state.paused;
                    if (state.paused) {
                        set_texture_color_mod(PAUSE_COLOR_R/255.0f, PAUSE_COLOR_G/255.0f, PAUSE_COLOR_B/255.0f);
                    } else {
                        set_texture_color_mod(MAIN_COLOR_R/255.0f, MAIN_COLOR_G/255.0f, MAIN_COLOR_B/255.0f);
                    }
                } break;

                // Stop on '^Q' or '^C' key
                case RGFW_q:
                case RGFW_c: {
                 if (RGFW_window_isKeyDown(win, RGFW_controlL) || RGFW_window_isKeyDown(win, RGFW_controlR)) {
                        should_close = true;
                    }
                } break;

                // TODO: add support for RGFW_kpPlus when RGFW 1.8.0 is released
                // case RGFW_kpPlus:
                case RGFW_equals: {
                    state.user_scale += SCALE_FACTOR * state.user_scale;
                } break;

                // TODO: add support for RGFW_kpMinus when RGFW 1.8.0 is released
                // case RGFW_kpMinus:
                case RGFW_minus: {
                    state.user_scale -= SCALE_FACTOR * state.user_scale;
                } break;

                // TODO: add support for RGFW_kp0 when RGFW 1.8.0 is released
                // case RGFW_kp0:
                case RGFW_0: {
                    state.user_scale = 1.0f;
                } break;

                case RGFW_F5: {
                    parse_state_from_args(&state, argc, argv);
                    if (state.paused) {
                        set_texture_color_mod(PAUSE_COLOR_R/255.0f, PAUSE_COLOR_G/255.0f, PAUSE_COLOR_B/255.0f);
                    } else {
                        set_texture_color_mod(MAIN_COLOR_R/255.0f, MAIN_COLOR_G/255.0f, MAIN_COLOR_B/255.0f);
                    }
                } break;

                case RGFW_F11: {
                    RGFW_windowFlags window_flags = RGFW_window_getFlags(win); // TODO: use RGFW_window_getFlags() when RGFW 1.8.0 is released
                    if (window_flags & RGFW_windowFullscreen) {
                        RGFW_window_setFlags(win, window_flags & (~RGFW_windowFullscreen));
                    } else {
                        RGFW_window_setFlags(win, window_flags | RGFW_windowFullscreen);
                    }
                } break;
                }
            } break;
            case RGFW_mouseScroll: {
                if (RGFW_isKeyDown(RGFW_controlL) || RGFW_isKeyDown(RGFW_controlR)) {
                    if (event.scroll.y > 0) {
                        state.user_scale += SCALE_FACTOR * state.user_scale;
                    } else if (event.scroll.y < 0) {
                        state.user_scale -= SCALE_FACTOR * state.user_scale;
                    }
                }
            } break;
            }
        }
        // INPUT END //////////////////////////////

        // RENDER BEGIN //////////////////////////////
        glClearColor(BACKGROUND_COLOR_R/255.0f, BACKGROUND_COLOR_G/255.0f, BACKGROUND_COLOR_B/255.0f, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        {
            const size_t t = (size_t) floorf(fmaxf(state.displayed_time, 0.0f));
            // PENGER BEGIN //////////////////////////////

            #ifdef PENGER
            render_penger_at(penger_tex_unit, win->w, win->h, state.displayed_time, state.mode==MODE_COUNTDOWN);
            #endif

            // PENGER END //////////////////////////////

            // DIGITS BEGIN //////////////////////////////
            int pen_x, pen_y;
            float fit_scale = 1.0;
            initial_pen(win->w, win->h, &pen_x, &pen_y, state.user_scale, &fit_scale);

            // TODO: support amount of hours >99
            const size_t hours = t / 60 / 60;
            render_digit_at(digits_tex_unit, hours / 10,   state.wiggle_index      % WIGGLE_COUNT, &pen_x, &pen_y, state.user_scale, fit_scale);
            render_digit_at(digits_tex_unit, hours % 10,  (state.wiggle_index + 1) % WIGGLE_COUNT, &pen_x, &pen_y, state.user_scale, fit_scale);
            render_digit_at(digits_tex_unit, COLON_INDEX,  state.wiggle_index      % WIGGLE_COUNT, &pen_x, &pen_y, state.user_scale, fit_scale);

            const size_t minutes = t / 60 % 60;
            render_digit_at(digits_tex_unit, minutes / 10, (state.wiggle_index + 2) % WIGGLE_COUNT, &pen_x, &pen_y, state.user_scale, fit_scale);
            render_digit_at(digits_tex_unit, minutes % 10, (state.wiggle_index + 3) % WIGGLE_COUNT, &pen_x, &pen_y, state.user_scale, fit_scale);
            render_digit_at(digits_tex_unit, COLON_INDEX,  (state.wiggle_index + 1) % WIGGLE_COUNT, &pen_x, &pen_y, state.user_scale, fit_scale);

            const size_t seconds = t % 60;
            render_digit_at(digits_tex_unit, seconds / 10, (state.wiggle_index + 4) % WIGGLE_COUNT, &pen_x, &pen_y, state.user_scale, fit_scale);
            render_digit_at(digits_tex_unit, seconds % 10, (state.wiggle_index + 5) % WIGGLE_COUNT, &pen_x, &pen_y, state.user_scale, fit_scale);

            char title[TITLE_CAP];
            snprintf(title, sizeof(title), "%02zu:%02zu:%02zu - sowon (RGFW)", hours, minutes, seconds);
            if (strcmp(state.prev_title, title) != 0) {
                RGFW_window_setName(win, title);
            }
            memcpy(title, state.prev_title, TITLE_CAP);
            // DIGITS END //////////////////////////////
        }

        RGFW_window_swapBuffers_OpenGL(win);
        // RENDER END //////////////////////////////

        // UPDATE BEGIN //////////////////////////////
        state_update(&state, dt);
        // UPDATE END //////////////////////////////

        now = RGFW_getTimerValue();
        uint64_t frame_time = ((now - last_time)*1000.0f)/RGFW_getTimerFreq();
        uint64_t frame_cap = 1000/FPS;
        if (frame_time < frame_cap) RGFW_sleep(frame_cap - frame_time);
    }

    RGFW_window_close(win);

    return 0;
}
