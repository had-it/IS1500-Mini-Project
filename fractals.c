// NOTE: Change float to fixed because float is slow on embedded systems

#include <stdint.h> // for uint16_t, uint8_t

#define MAX_ITER 80 // Maximum iterations for fractal calculations

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

// Convert RGB888 (color format used in computers) -> RGB565 (used in embedded systems) for VGA framebuffer
// RGB888: 8 bits for Red, 8 for Green, 8 for Blue (total 24 bits)
// RGB565: 5 bits for Red, 6 bits for Green, 5 bits for Blue (total 16 bits)
// Basically, we take 24-bit inputs, mask lower bits and shift to fit into 16 bits
static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return (uint16_t)(
           ((r & 0xF8) << 8) | // mask r with 0xF8 (= 1111 1000) to keep the upper 5 bits of red. Then shift left by 8 because red occupies the highest bits of the final 16-bit value.
           ((g & 0xFC) << 3) | // mask g with 0xFC (= 1111 1100) to keep the upper 6 bits of green. Then shift left by 3 because green occupies the middle bits of the final 16-bit value.
           (b >> 3) ); // We only need 5 bits so we shift it to the right by 3.
}

// One simple palette (blue -> cyan -> white)
void build_palette(uint16_t pal[256]) {
    int i;
    for (i = 0; i < 256; i++) {
        uint8_t v = (uint8_t)i;
        pal[i] = rgb565(v/2, v, 255);
    }
}

// Map iteration count to color
uint16_t iter_to_color(int iter, int max_iter, uint16_t pal[256]) {
    if (iter >= max_iter)
        return 0x0000; // inside = black

    int idx = (iter * 255) / (max_iter - 1);
    return pal[idx];
}
