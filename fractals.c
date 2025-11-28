// NOTE: Change float to fixed because float is slow on embedded systems

#include <stdint.h> // for uint16_t, uint8_t, and float?

#define MAX_ITER_DEF 80 // Maximum iterations for fractal calculations

// Function for absolute values (<math.h> not allowed)
static inline float abs(float f) {
    if (f < 0.0f){
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
        ++i;
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
        float ax = abs(x);
        float ay = abs(y);

        // Burning ship [Z = (abs(Re(Z))) + i*abs((Im(Z))))^2 + C]
        float x_new = ax*ax - ay*ay + c_re;
        y = 2.0f * ax * ay + c_im;
        x = x_new;
        ++i;
    }
    return i;
}

/* --- Palette / index mapping for 8-bit framebuffer --- */
/* build_palette fills a remap table pal[256] that maps a logical index (0..255)
   to the final index actually written to the framebuffer. For many simple setups
   we can use identity mapping (pal[i]=i). If you wish to remap to make colors
   more pleasing, change the function below. */

void build_palette(uint8_t pal[256]) {
    for (int i = 0; i < 256; ++i) {
        /* default: identity mapping (index -> same index) */
        pal[i] = (uint8_t)i;
    }

    /* Example: you could remap to emphasize certain ranges:
       pal[i] = (uint8_t)((i < 64) ? i*2 : (i < 192) ? 128 + ((i-64)*127/128) : 255);
       But leave identity for now unless you know the board's palette mapping.
    */
}

/* Map iter -> 0..255. We reserve 0 for "inside" (black) and map escapes to 1..255. */
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
