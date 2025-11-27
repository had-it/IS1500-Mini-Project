#include <stdint.h>

// Functions from fractals.c 
extern int mandelbrot(float c_re, float c_im, int max_iter);
extern int burningship(float c_re, float c_im, int max_iter);
extern void build_palette(uint16_t pal[256]);
extern uint16_t iter_to_color(int iter, int max_iter, uint16_t pal[256]);

// Dimensions of the screen size
#define W 320
#define H 240
#define MAX_ITER 50   // keep same value used when building palette / testing

// MMIO (DTEK-V memory map / addresses)
#define VGA_FRAMEBUF  ((volatile uint16_t *)0x08000000UL)   // UL stands for unsigned long (not strictly necessary)
#define SWITCH   ((volatile uint32_t *)0x04000010UL)
#define BUTTON   ((volatile uint32_t *)0x040000D0UL)

/* selected ports bits */
#define SWITCH_BIT_MASK  (1u << 0) // 1u means 1 unsigned. (iu << 0) is basically just 1
#define BUTTON_DRAW_MASK (1u << 0)

// Calling assembly pause function for delay
extern void asm_pause(unsigned int loops);

// Getting switch and button
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
    for (int i = 0; i < W*H; ++i) {
        fb[i] = 0x0000;
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
                fb[py * W + px] = iter_to_color(iter, MAX_ITER, palette);// Drawing with vga
            } else {
                int iter = burningship(cx, cy, MAX_ITER);
                fb[py * W + px] = iter_to_color(iter, MAX_ITER, palette);// Drawing with vga
            }

            /* py: y-axis (row), px: x-axis (colum), W: width
               py * W + px = row * width + column (converts 2D (x,y) into 1D)
               Basically we are calculation the position of a pixel in the framebuffer */

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
            if ((btn & BUTTON_DRAW_MASK) && !(last_btn & BUTTON_DRAW_MASK)) {
            draw_fractal_to_fb(fractal_type, palette);
        }
        } else {
            int fractal_type = 0; // Mandelbrot
            if ((btn & BUTTON_DRAW_MASK) && !(last_btn & BUTTON_DRAW_MASK)) {
            draw_fractal_to_fb(fractal_type, palette);
        }
        }

        /* on rising edge of draw button, render fractal */
        

        last_btn = btn;

        /* simple debounce/idle */
        asm_pause(200000);
    }

    /* never reached */
    return 0;
}
