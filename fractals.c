// NOTE: Change float to fixed because float is slow on embedded systems

#include <stdint.h> // for uint16_t, uint8_t, and float?

#define MAX_ITER 50 // Maximum iterations for fractal calculations

// Function for absolute values (<math.h> not allowed)
static inline int32_t abs(int32_t f) {
    if (f < 0){
        f = -f;
    }
    return f;
    // we might need to handle special cases for -0.0 and NaN (according to Copilot)
}

// Mandelbrot set equation iteration
int mandelbrot(int32_t c_re, int32_t c_im, int max_iter) {
    int32_t x = 0;
    int32_t y = 0;

    int i = 0;

    // Mandelbrot [Z = Z^2 + C]
    while (i < max_iter && ((int64_t)x*x + (int64_t)y*y) <= (int64_t)4 << 32) {
        int32_t x_new = ((int64_t)x*x - (int64_t)y*y) >> 16;
        x_new += c_re;
        y = ((int64_t)2*x*y) >> 16;
        y += c_im; // Imaginary part
        x = x_new;
        i++;
    }
    return i;
}

// Burning Ship equation iteration
int burningship(int32_t c_re, int32_t c_im, int max_iter) {
    int32_t x = 0;
    int32_t y = 0;

    int i = 0;

    // Burning ship [Z = (abs(Re(Z))) + i*abs((Im(Z))))^2 + C]
    while (i < max_iter && ((int64_t)x*x + (int64_t)y*y) <= (int64_t)4 << 32) {
        int32_t abs_x = abs(x);
        int32_t abs_y = abs(y);

        
        int32_t x_new = ((int64_t)abs_x*abs_x - (int64_t)abs_y*abs_y) >> 16;
        x_new += c_re;
        y = ((int64_t)2*abs_x*abs_y) >> 16;
        y += c_im;
        x = x_new;

        i++;
    }
    return i;
}

uint8_t rgb332(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xE0))       |   // 3 MSB av r
           ((g & 0xE0) >> 3) |   // 3 MSB av g
           ((b & 0xC0) >> 6);    // 2 MSB av b
}


// One simple palette (blue -> cyan -> white)
void build_palette(uint8_t pal[256], int palette) {
    
    for (int i = 0; i < 256; i++) {
        if (palette == 0) {
            pal[i] = rgb332(i, i/4, 0); // Fire
        }
        else if (palette == 1){
            pal[i] = rgb332(i/2, i/4, i); // Sea
        }
    }
}

// Map iteration count to color
uint8_t iter_to_index(int iter, int max_iter) {
    if (iter >= max_iter) {
        return 0; // inside = black
    }
    
    int idx = (iter * 254) / (max_iter - 1) + 1; // we compute an index between 0 and 255
    if (idx < 1) {
        idx = 1;
    }
    if (idx > 255) { 
        idx = 255;
    }
    return (uint8_t)idx;
}