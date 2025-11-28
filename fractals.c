// NOTE: Change float to fixed because float is slow on embedded systems

#include <stdint.h> // for uint16_t, uint8_t, and float?

#define MAX_ITER 50 // Maximum iterations for fractal calculations

// Function for absolute values (<math.h> not allowed)
static inline float abs(float f) {
    if (f < 0){
        f = -f;
    }
    return f;
    // we might need to handle special cases for -0.0 and NaN (according to Copilot)
}

// Mandelbrot set equation iteration
int mandelbrot(float c_re, float c_im, int max_iter) {
    float x = 0.0f;
    float y = 0.0f;

    int i = 0;

    // Mandelbrot [Z = Z^2 + C]
    while (i < max_iter && (x*x + y*y) <= 4.0f) {
        float x_new = x*x - y*y + c_re; // Real part
        y = 2.0f * x * y + c_im; // Imaginary part
        x = x_new;
        i++;
    }
    return i;
}

// Burning Ship equation iteration
int burningship(float c_re, float c_im, int max_iter) {
    float x = 0.0f;
    float y = 0.0f;

    int i = 0;

    // Burning ship [Z = (abs(Re(Z))) + i*abs((Im(Z))))^2 + C]
    while (i < max_iter && (x*x + y*y) <= 4.0f) {
        float abs_x = abs(x);
        float abs_y = abs(y);

        float x_new = abs_x*abs_x - abs_y*abs_y + c_re; // Real part
        y = 2.0f * abs_x * abs_y + c_im; // Imaginary part
        x = x_new;

        i++;
    }
    return i;
}


// One simple palette (blue -> cyan -> white)
void build_palette(uint8_t pal[256]) {
    for (int i = 0; i < 256; i++) {
        pal[i] = (uint8_t)i;
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

