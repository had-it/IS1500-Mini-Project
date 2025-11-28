// main.c (8-bit framebuffer, full 320x480 draw)
// Writes one byte-per-pixel palette indices into 0x08000000.

#include <stdint.h>

/* fractal iteration functions (from fractals.c) */
extern int mandelbrot(float c_re, float c_im, int max_iter);
extern int burningship(float c_re, float c_im, int max_iter);
extern void build_palette(uint8_t pal[256]);                 // fill remap table (index -> final index)
extern uint8_t iter_to_index(int iter, int max_iter);       // map iter -> 0..255
extern void asm_pause(unsigned int loops);

/* constants */
#define W 320
#define H 480
#define MAX_ITER 80

/* MMIO */
#define VGA ((volatile uint8_t*)0x08000000UL)  // VGA framebuffer has 8-bits per pixel
#define SWITCH ((volatile int*)0x04000010)
#define BUTTON ((volatile int*)0x040000D0)

#define SWITCH_BIT_MASK  (1u << 0)
#define BUTTON_DRAW_MASK (1u << 0)

// Getting switch and button
static int get_sw(void)  {
    return *SWITCH; 
}
static int get_btn(void) { 
    return *BUTTON; 
}

// Draw fractal to framebuffer
static void draw_fractal(int fractal_type, uint8_t palette[256]) {
    volatile uint8_t *fb = VGA;

    // Clear the entire VGA buffer area by writing the value 0 (=black)
    for (int i = 0; i < W * H; ++i) {
        fb[i] = 0;
    }

    // Parameters for view (static basic view)
    float center_x = -0.5f;
    float center_y = 0.0f;
    float scale = 3.0f;

    float halfw = (float)W / 2.0f;
    float halfh = (float)H / 2.0f;
    float pixel = scale / (float)W;

    for (int py = 0; py < H; ++py) {
        for (int px = 0; px < W; ++px) {
            float cx = center_x + ((float)px - halfw) * pixel;
            float cy = center_y + ((float)py - halfh) * pixel;

            uint8_t idx = iter_to_index(iter, MAX_ITER);

            int iter;
            if (fractal_type == 0) {
                iter = mandelbrot(cx, cy, MAX_ITER);
                fb[py * W + px] = palette[idx]; // Drawing with vga
            } else {
                iter = burningship(cx, cy, MAX_ITER);
                fb[py * W + px] = palette[idx]; // Drawing with vga
            }
        }
    }
}

int main(void) {
    // Create palette
    static uint8_t palette[256];
    build_palette(palette);

    int last_btn = 0;

    while (1) {
        int sw = get_sw();
        int btn = get_btn();

        if (sw & SWITCH_BIT_MASK) {
            int fractal_type = 1; // Burning Ship
            if ((btn & BUTTON_DRAW_MASK) && !(last_btn & BUTTON_DRAW_MASK)) {
            draw_fractal(fractal_type, palette);
        }
        } else {
            int fractal_type = 0; // Mandelbrot
            if ((btn & BUTTON_DRAW_MASK) && !(last_btn & BUTTON_DRAW_MASK)) {
            draw_fractal(fractal_type, palette);
        }
        }

        last_btn = btn;
        asm_pause(200000);
    }
    return 0;
}
