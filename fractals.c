// Compile: gcc mandelbrotset.c -o mandelbrotset $(pkg-config --cflags --libs sdl2) -lm
// Run: ./mandelbrotset

#include <SDL2/SDL.h>   // Library for graphics
#include <complex.h>    // Library for complex numbers
#include <math.h>       // Library for mathematical functions

// Dimensions of the screen size
#define W 800
#define H 800

// Maximum number of iterations for pixels (the higher, the slower, but better quality)
#define MAXI 500

int main(void) {
    if (SDL_Init(SDL_INIT_VIDEO)) return 1;
    SDL_Window *win = SDL_CreateWindow("Mandelbrot (simple)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W, H, 0);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, 0);

    for (int py = 0; py < H; ++py) {
        for (int px = 0; px < W; ++px) {
            /* map pixel to complex plane: x in [-2,1], y in [-1.5,1.5] */
            float c_re = -2.0 + (3.0 * px) / (W - 1);
            float c_im =  1.5 - (3.0 * py) / (H - 1);

            float x = 0,
            float y = 0;

            int i = 0;

            // Mandelbrot
            if(fractal_type == 0){
                while (i < MAXI && x*x + y*y <= 4) {
                    float x_new = x*x - y*y + c_re;
                    y = 2*x*y + c_im
                    x = x_new;
                    z = z*z + c;
                    ++i
                }
            }

            // Burning ship
            if (fractal_type == 1){
                while (ii < MAXI && x*x + y*y <= 4) { 
                    float abs_x = my_abs(x);
                    float abs_y = my_abs(y);
                    float x_new = abs_x*abs_x - abs_y*abs_y + c_re;
                    y = 2 * abs_x * abs_y + c_im;
                    x = x_new;
                    ++i;
                }
            }

            /* simple color: inside = black, outside = gray scaled */
            if (i == MAXI) {
                pixel_color = 0; // black
            } else {
                if (color_palette == 0){
                    pixel_color = (i * 6) % 255;
                }
                if (color_palette == 0){
                    pixel_color = (i * 10 + 50) % 255;
                }
            }

            
            SDL_RenderDrawPoint(ren, px, py);
        }
    }

    SDL_RenderPresent(ren);

    /* wait until window closed or ESC pressed */
    int running = 1;
    SDL_Event e;
    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) running = 0;
        }
        SDL_Delay(10);
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
