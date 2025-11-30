#include <stdint.h>

// Functions from fractals.c 
extern int mandelbrot(int32_t c_re, int32_t c_im, int max_iter);
extern int burningship(int32_t c_re, int32_t c_im, int max_iter);
extern void build_palette(uint8_t pal[256]);
extern uint8_t iter_to_index(int iter, int max_iter);
extern void asm_pause(unsigned int loops);

// Dimensions of the screen size
#define W 320
#define H 240
#define MAX_ITER 50   // keep same value used when building palette / testing

// MMIO (DTEK-V memory map / addresses)
#define VGA      ((volatile uint8_t *)0x08000000UL)   // UL stands for unsigned long (not strictly necessary)
#define SWITCH   ((volatile uint32_t *)0x04000010UL)
#define BUTTON   ((volatile uint32_t *)0x040000D0UL)

// Masked bits for switches and buttons
#define SWITCH_BIT_MASK  (1u << 0) // 1u means 1 unsigned. (iu << 0) is basically just 1
#define BUTTON_DRAW_MASK (1u << 0)

// Getting switch and button
static int get_sw(void) {
    return (*SWITCH);
}
static int get_btn(void) {
    return (*BUTTON);
}

 // Clearing the whole VGA buffer by setting all pixels to black (0)
static void clearScreen(void){
    volatile uint8_t *fb = VGA;
    for (int i = 0; i < W*H; ++i) {
        fb[i] = 0;
    }
}

/* Draw using functions from fractals.c */
static void draw_fractal_to_fb(int fractal_type, uint8_t palette[256], int32_t scale) {
    volatile uint8_t *fb = VGA;

    clearScreen();

    /* view parameters (static basic view) */
    int32_t center_x = -32768;   // -0.5 * (1 << 16)
    int32_t center_y =  0;

    int32_t pixel = (int32_t)(((int64_t)scale) / W);

    int half_w = W / 2;
    int half_h = H / 2;

    for (int py = 0; py < H; ++py) {
        for (int px = 0; px < W; ++px) {
            int32_t cx = center_x + (int32_t)(( (int64_t)(px - half_w) * pixel ));
            int32_t cy = center_y + (int32_t)(( (int64_t)(py - half_h) * pixel ));

            int iter = mandelbrot(cx, cy, MAX_ITER);

            if (fractal_type == 1) {
                iter = burningship(cx, cy, MAX_ITER);
            }

            uint8_t idx = iter_to_index(iter, MAX_ITER);
            fb[py * W + px] = palette[idx]; // Drawing with vga
        }
    }
}

int main(void) {

    // Menu for palette opens 
    // 3 switches for each palette
    // Menu for fractals
    // 2 switches for each fractal
    // Draw fractal
    // 


    // Making a palette
    static uint8_t palette[256];
    build_palette(palette);

    int last_btn = 0;

    while (1) {
        int sw  = get_sw();
        int btn = get_btn();
        int32_t scale = 5 * (1 << 16);  // 3.0 in fixed point format (Q16.16)

        int fractal_type = 0; // Mandelbrot

        if (sw & SWITCH_BIT_MASK) {
            fractal_type = 1; // Burning Ship
        }

        if ((btn & BUTTON_DRAW_MASK) && !(last_btn & BUTTON_DRAW_MASK)) {
            draw_fractal_to_fb(fractal_type, palette, scale);
        }      
        scale = (scale * 62259) >> 16;  // 62259 / 65536 ≈ 0.95

        last_btn = btn;
        asm_pause(200000); // Small pause to debounce button presses
    }

    return 0; // Does not reach here
}