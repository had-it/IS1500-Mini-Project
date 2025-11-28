#include <stdint.h>

// Functions from fractals.c 
extern int mandelbrot(float c_re, float c_im, int max_iter);
extern int burningship(float c_re, float c_im, int max_iter);
extern void build_palette(uint8_t pal[256]);
extern uint8_t iter_to_index(int iter, int max_iter);
extern void asm_pause(unsigned int loops);

// Dimensions of the screen size
#define W 320
#define H 240
#define MAX_ITER 50   // keep same value used when building palette / testing

// MMIO (DTEK-V memory map / addresses)
#define VGA_FRAMEBUF  ((volatile uint8_t *)0x08000000UL)   // UL stands for unsigned long (not strictly necessary)
#define SWITCH   ((volatile uint32_t *)0x04000010UL)
#define BUTTON   ((volatile uint32_t *)0x040000D0UL)
#define VGA_

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

 /* Clear the entire VGA buffer area by writing the value 0 (=black) */
static void clearScreen(void){
    volatile uint8_t *fb = VGA_FRAMEBUF;
    for (int i = 0; i < W*H; ++i) {
        fb[i] = 0x0000;
    }
}

/* Changes from which line in the buffer the VGA starts drawing */
static void scroll(void){

    // se lecture 6

}

static void drawSprite(void){

    // a routine that draws a sprite to the screen at some location 
}

/* Draw using functions from fractals.c */
static void draw_fractal_to_fb(int fractal_type, uint8_t palette[256]) {
    volatile uint8_t *fb = VGA_FRAMEBUF;

    clearScreen();

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
                iter = mandelbrot(cx, cy, MAX_ITER);
                uint8_t idx = iter_to_index(iter, MAX_ITER);

                fb[py * W + px] = palette[idx]; // Drawing with vga
            } else {
                iter = burningship(cx, cy, MAX_ITER);
                uint8_t idx = iter_to_index(iter, MAX_ITER);
                fb[py * W + px] = palette[idx]; // Drawing with vga
            }

        }
    }
}

int main(void) {
    /* build single palette */
    static uint8_t palette[256];
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
