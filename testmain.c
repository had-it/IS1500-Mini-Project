#include <stdint.h>

// Functions from fractals.c 
extern int mandelbrot(float c_re, float c_im, int max_iter);
extern int burningship(float c_re, float c_im, int max_iter);
extern void build_palette(uint16_t pal[256]);
extern uint16_t iter_to_color(int iter, int max_iter, uint16_t pal[256]);

// Dimensions of the screen size
#define W 320
#define H 240
#define MAX_ITER 80   // keep same value used when building palette / testing

// MMIO (DTEK-V memory map / addresses)
#define VGA_FRAMEBUF  ((volatile uint16_t *)0x08000000)
#define SWITCH   ((volatile uint32_t *)0x04000010)
#define BUTTON   ((volatile uint32_t *)0x040000D0)

/* selected ports bits */
#define SWITCH_BIT_MASK  (1u << 0)
#define BUTTON_DRAW_MASK (1u << 0)

// Calling assembly pause function for delay
extern void asm_pause(unsigned int loops);

/* simple accessors like your lab main style */
static int get_sw(void) {
    return (int)(*SWITCH);
}
static int get_btn(void) {
    return (int)(*BUTTON);
}

/* Draw using functions from fractals.c */
static void draw_fractal_to_fb(int fractal_type, uint16_t palette[256]) {
    volatile uint16_t *fb = VGA_FRAMEBUF;

    // Clear the entire VGA buffer area by writing the value 0 (=black)
    for (int i = 0; i < 320*480; i++) {
        fb[i] = 0;
    }

    /* view parameters (static basic view) */
    float center_x = -0.5f;
    float center_y =  0.0f;
    float scale    =  3.0f;

    float halfw = (float)W / 2.0f;
    float halfh = (float)H / 2.0f;
    float pixel = scale / (float)W;

    for (int py = 0; py < H; ++py) {
        for (int px = 0; px < W; ++px) {
            float cx = center_x + ((float)px - halfw) * pixel;
            float cy = center_y + ((float)py - halfh) * pixel;

            if (fractal_type == 0) {
                int iter = mandelbrot(cx, cy, MAX_ITER);
            } else {
                int iter = burningship(cx, cy, MAX_ITER);
            }

            fb[py * W + px] = iter_to_color(iter, MAX_ITER, palette); // Drawing with vga
        }
    }
}

int main(void) {
    /* build single palette */
    static uint16_t palette[256];
    build_palette(palette);

    int last_btn = 0;

    while (1) {
        int sw  = get_sw();
        int btn = get_btn();

        if (sw & SWITCH_BIT_MASK) {
            int fractal_type = 1; // Burning Ship
        } else {
            int fractal_type = 0; // Mandelbrot
        }

        /* on rising edge of draw button, render fractal */
        if ((btn & BUTTON_DRAW_MASK) && !(last_btn & BUTTON_DRAW_MASK)) {
            draw_fractal_to_fb(fractal_type, palette);
        }

        last_btn = btn;

        /* simple debounce/idle */
        asm_pause(200000);
    }

    /* never reached */
    return 0;
}
