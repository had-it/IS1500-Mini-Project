
// Compile: gcc burningship.c -o burningship $(pkg-config --cflags --libs sdl2) -lm
// Run: ./burningship

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
    SDL_Window *win = SDL_CreateWindow("BurningShip (simple)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W, H, 0);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, 0);

    for (int py = 0; py < H; ++py) {
        for (int px = 0; px < W; ++px) {
            /* map pixel to complex plane: x in [-2,1], y in [-1.5,1.5] */
            double cx = -2.0 + (3.0 * px) / (W - 1);
            double cy =  1.5 - (3.0 * py) / (H - 1);
            double zr = creal(z);
            double zi = cimag(z);
            double complex c = cx + cy * I;
            double complex z = 0;
            int i = 0;
            
            
            while (i < MAXI && cabs(z) <= 2.0) { 
                z = (fabs(zr) - fabs(zi)*I) * (fabs(zr) - fabs(zi)*I) + c;
                ++i;
            }

            /* simple color: inside = black, outside = gray scaled */
            if (i == MAXI) {
                SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
            } else {
                unsigned char v = (unsigned char)(255 * i / (double)MAXI);
                SDL_SetRenderDrawColor(ren, v, 0, 255 - v, 255);
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