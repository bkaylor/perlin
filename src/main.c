#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>

#include "SDL.h"
#include "SDL_ttf.h"
#include "SDL_image.h"

#include "vec2.h"
#include "draw.h"
#include "button.h"

#include "perlin.h"

typedef struct {
    int x;
    int y;
} Window;

typedef struct {
    Window window;
    Gui gui;

    Mouse_Info mouse_info;

    SDL_Surface *surface;

    int frequency;
    int depth;
    float scale;

    bool show_menu;
    bool do_iteration;
    bool quit;
} State;

int get_2d_index(int x, int y)
{
    return y * 1280 + x;
}

void set_pixel_on_surface(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
    Uint32 * const target_pixel = (Uint32 *) ((Uint8 *) surface->pixels
                                             + y * surface->pitch
                                             + x * surface->format->BytesPerPixel);
    *target_pixel = pixel;
}

void render(SDL_Renderer *renderer, State state)
{
    SDL_RenderClear(renderer);

    // Set background color.
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, NULL);

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, state.surface);
    SDL_RenderCopy(renderer, texture, NULL, NULL);

    SDL_DestroyTexture(texture);

    draw_all_buttons(renderer, &state.gui);

    SDL_RenderPresent(renderer);
}

void update(State *state) 
{
    bool changed = false;

    Gui *g = &state->gui;
    gui_frame_init(g);
    g->mouse_info = state->mouse_info;
    new_button_stack(g, 5, 5, 5);

    if (do_button_tiny(g, "?"))
    {
        state->show_menu = !state->show_menu;
    }

    if (state->show_menu)
    {
        static char frequency_string[30];
        sprintf(frequency_string, "freq=%d", state->frequency);
        do_button(g, frequency_string);

        if (do_button(g, "+"))
        {
            state->frequency += 1;
            changed = true;
        }

        if (do_button(g, "-"))
        {
            state->frequency -= 1;
            if (state->frequency < 1) state->frequency = 1;
            changed = true;
        }

        static char depth_string[30];
        sprintf(depth_string, "depth=%d", state->depth);
        do_button(g, depth_string);

        if (do_button(g, "+"))
        {
            state->depth += 1;
            changed = true;
        }

        if (do_button(g, "-"))
        {
            state->depth -= 1;
            if (state->depth < 1) state->depth = 1;
            changed = true;
        }

        static char scale_string[30];
        sprintf(scale_string, "scale=%0.4f", state->scale);
        do_button(g, scale_string);

        if (do_button(g, "+"))
        {
            state->scale *= 2;
            changed = true;
        }

        if (do_button(g, "-"))
        {
            state->scale *= 0.5;
            if (state->scale <= 0) state->scale = 0.002;
            changed = true;
        }
    }

    if (changed) state->do_iteration = true;

    if (state->do_iteration)
    {
        // Get colors
        for (int x = 0; x < 1280; x += 1)
        {
            for (int y = 0; y < 720; y += 1)
            {
                float noise = Perlin_Get2d(x*state->scale, 
                                           y*state->scale, 
                                           state->frequency, 
                                           state->depth);

                int r = 0;
                int g = 0;
                int b = 0;

                // Greyscale color
                if (0)
                {
                    float color = noise * 255.0;

                    r = color;
                    g = color;
                    b = color;
                }

                // Terrain color 
                if (1)
                {
                    if (noise > 0.8)
                    {
                        // Dark green
                        r = 32;
                        g = 71;
                        b = 40;
                    }
                    else if (noise > 0.55)
                    {
                        // Green
                        r = 52;
                        g = 91;
                        b = 60;
                    }
                    else if (noise > 0.50)
                    {
                        // Light tan
                        r = 196;
                        g = 194;
                        b = 179;
                    }
                    else if (noise > 0.48)
                    {
                        // Darker blue
                        r = 34;
                        g = 85;
                        b = 134;
                    }
                    else if (noise > 0.15)
                    {
                        // Lighter blue
                        r = 69;
                        g = 120;
                        b = 139;
                    }
                    else if (noise >= 0)
                    {
                        // Purple
                        r = 68;
                        g = 60;
                        b = 106;
                    }

                }

                Uint32 formatted_color = ((r << 16) + (g << 8) + (b << 0));
                set_pixel_on_surface(state->surface, x, y, formatted_color);
            }
        }

        state->do_iteration = false;
    }

    return;
}

void get_input(State *state)
{
    SDL_GetMouseState(&state->mouse_info.x, &state->mouse_info.y);
    state->mouse_info.clicked = false;

    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                    case SDLK_SPACE:
                        state->do_iteration = true;
                        SEED = rand();

                        break;

                    case SDLK_ESCAPE:
                        state->quit = true;
                        break;
                }
                break;

            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    state->mouse_info.clicked = true;
                }
                break;

            case SDL_QUIT:
                state->quit = true;
                break;

            default:
                break;
        }
    }
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_EVERYTHING);
    IMG_Init(IMG_INIT_PNG);

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        printf("SDL_Init video error: %s\n", SDL_GetError());
        return 1;
    }

    if (SDL_Init(SDL_INIT_AUDIO) != 0)
    {
        printf("SDL_Init audio error: %s\n", SDL_GetError());
        return 1;
    }

    // SDL_ShowCursor(SDL_DISABLE);

	// Setup window
	SDL_Window *win = SDL_CreateWindow("Perlin",
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			1280, 720,
			SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

	// Setup renderer
	SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	// Setup font
	TTF_Init();
	TTF_Font *font = TTF_OpenFont("liberation.ttf", 12);
	if (!font)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error: Font", TTF_GetError(), win);
		return -666;
	}

    // Setup main loop
    srand(time(NULL));

    // Main loop
    State state;
    state.quit = false;
    state.show_menu = false;
    state.do_iteration = true;
    state.surface = SDL_CreateRGBSurface(0, 1280, 720, 32, 0, 0, 0, 0);
    state.frequency = 7;
    state.depth = 7;
    state.scale = 0.001;

    gui_init(&state.gui, font);

    while (!state.quit)
    {
        SDL_PumpEvents();
        get_input(&state);

        if (!state.quit)
        {
            int x, y;
            SDL_GetWindowSize(win, &state.window.x, &state.window.y);

            update(&state);
            render(ren, state);
        }
    }

    return 0;
}
