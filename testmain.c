#include <stdint.h>

// Functions from fractals.c 
extern int mandelbrot(int32_t c_re, int32_t c_im, int max_iter);
extern int burningship(int32_t c_re, int32_t c_im, int max_iter);
extern void build_palette(uint8_t pal[256], int palette);
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
#define SWITCH_BIT_MASK  (1u << 0) // 1u means 1 unsigned. (1u << 0) is basically just 1
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
static void draw_fractal_to_fb(int fractal_type, uint8_t palette[256], int32_t scale, int32_t center_x, int32_t center_y) {
    volatile uint8_t *fb = VGA;
    int32_t pixel = (int32_t)(((int64_t)scale) / W);

    int half_w = W / 2;
    int half_h = H / 2;

    for (int py = 0; py < H; ++py) {
        for (int px = 0; px < W; ++px) {
            int32_t cx = center_x + (int32_t)(((int64_t)(px - half_w) * pixel));
            int32_t cy = center_y + (int32_t)(((int64_t)(py - half_h) * pixel));

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

    clearScreen();

    int sw  = get_sw();
    int btn = get_btn();
    int last_btn = 0;
    int fractal_type;


    
    static uint8_t palette1[256];
    static uint8_t palette2[256];
    static uint8_t *palette;

    // Initial center of the Fractals
    int32_t center_x = -32768;   // -0.5 * (1 << 16)
    int32_t center_y = 0;

    int32_t scale = 5 * (1 << 16);  // 5.0 in fixed point format (Q16.16)
    int32_t pixel = (int32_t)(((int64_t)scale) / W);

    // Palette selection loop (panel 1)
    while (1) {
        int sw  = get_sw();
        int btn = get_btn();

        // Switch 0
        if (sw & (1u << 0) && ((btn & BUTTON_DRAW_MASK) && !(last_btn & BUTTON_DRAW_MASK))) {
            build_palette(palette1, 0); // Palette 1
            palette = palette1;
            asm_pause(200000);
            break; // Exit loop after selecting palette
        }
        // Switch 1
        if (sw & (1u << 1) && ((btn & BUTTON_DRAW_MASK) && !(last_btn & BUTTON_DRAW_MASK))) {
            build_palette(palette2, 1); // Palette 2
            palette = palette2;
            asm_pause(200000);
            break; // Exit loop after selecting palette
        }    

        last_btn = btn;
        asm_pause(200000);
    }

    last_btn = 0;
    //clearScreen();

    // Fractal selection loop (panel 2)
    while (1) {
        int sw  = get_sw();
        int btn = get_btn();

        // Switch 0
        if (sw & (1u << 0) && ((btn & BUTTON_DRAW_MASK) && !(last_btn & BUTTON_DRAW_MASK)))  { // If switch 0 is on and button is pressed
            fractal_type = 0; // Mandelbrot
            draw_fractal_to_fb(fractal_type, palette, scale, center_x, center_y);
            asm_pause(200000);
            break; // Exit loop after drawing
        }
        // Switch 1
        if (sw & (1u << 1) && ((btn & BUTTON_DRAW_MASK) && !(last_btn & BUTTON_DRAW_MASK))) { // If switch 1 is on and button is pressed
            fractal_type = 1; // Burning Ship
            draw_fractal_to_fb(fractal_type, palette, scale, center_x, center_y);
            asm_pause(200000);
            break; // Exit loop after drawing
        }

        last_btn = btn;
        asm_pause(200000); // Small pause to debounce button presses
    }

    last_btn = 0;

    // Navigation loop (panel 3)
    while (1) {
        int sw  = get_sw();
        int btn = get_btn();

        if ((btn & BUTTON_DRAW_MASK) && (last_btn & BUTTON_DRAW_MASK)) { // If button is held down
            if (sw & (1u << 0)) { // If switch 1 is on, we go up
                center_y += pixel; // Move the center up 
                draw_fractal_to_fb(fractal_type, palette, scale, center_x, center_y);
            } else if (sw & (1u << 1)) { // If switch 2 is on, we go down
                center_y -= pixel; // Move the center down
                draw_fractal_to_fb(fractal_type, palette, scale, center_x, center_y);
            } else if (sw & (1u << 2)) { // If switch 3 is on, we go right
                center_x += pixel; // Move the center right
                draw_fractal_to_fb(fractal_type, palette, scale, center_x, center_y);
            } else if (sw & (1u << 3)) { // If switch 4 is on, we go left
                center_x -= pixel; // Move the center left
                draw_fractal_to_fb(fractal_type, palette, scale, center_x, center_y);
            } else if (sw & (1u << 4)) { // If switch 5 is on, we zoom in
                scale -= pixel; // Zoom in by reducing scale
                draw_fractal_to_fb(fractal_type, palette, scale, center_x, center_y);
            } else if (sw & (1u << 5)) { // If switch 6 is on, we zoom out
                scale += pixel; // Zoom out by increasing scale
                draw_fractal_to_fb(fractal_type, palette, scale, center_x, center_y);
            }
        }

        last_btn = btn;
        asm_pause(200000); // Small pause to debounce button presses        
    }

    return 0; // Does not reach here
}