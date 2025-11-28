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
#define VGA  ((volatile uint8_t *)0x08000000UL)   // UL stands for unsigned long (not strictly necessary)
#define SWITCH   ((volatile uint32_t *)0x04000010UL)
#define BUTTON   ((volatile uint32_t *)0x040000D0UL)

/* selected ports bits */
#define SWITCH_BIT_MASK  (1u << 0) // 1u means 1 unsigned. (iu << 0) is basically just 1
#define BUTTON_DRAW_MASK (1u << 0)

// Getting switch and button
static int get_sw(void) {return *SWITCH;}
static int get_btn(void) {return *BUTTON;}

 /* Clear the entire VGA buffer area by writing the value 0 (=black) */
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
    int32_t center_x = -32768;   // -0.5 * 65536
    int32_t center_y =  0;

    for (int py = 0; py < H; ++py) {
        for (int px = 0; px < W; ++px) {
            int32_t cx = center_x + ((int64_t)(px - W/2) * scale) / W;
            int32_t cy = center_y + ((int64_t)(py - H/2) * scale) / H;
        
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
    /* build single palette */
    static uint8_t palette[256];
    build_palette(palette);

    int last_btn = 0;
    

    while (1) {
        int sw  = get_sw();
        int btn = get_btn();
        int32_t scale = 3 << 16;

        int fractal_type = 0; // Mandelbrot

        if (sw & SWITCH_BIT_MASK) {
            fractal_type = 1; // Burning Ship
        }

        if ((btn & BUTTON_DRAW_MASK) && !(last_btn & BUTTON_DRAW_MASK)) {
            draw_fractal_to_fb(fractal_type, palette, scale);
        }
        scale = (scale * 62259) >> 16;  // 62259 / 65536 ≈ 0.95
        

        /* on rising edge of draw button, render fractal */
        last_btn = btn;

        /* simple debounce/idle */
        asm_pause(200);
    }

    /* never reached */
    return 0;
}