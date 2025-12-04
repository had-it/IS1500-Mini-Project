#include <stdint.h>

//branch

// Functions from fractals.c 
extern int mandelbrot(int32_t c_re, int32_t c_im, int max_iter);
extern int burningship(int32_t c_re, int32_t c_im, int max_iter);
extern void build_palette(uint8_t pal[256], int palette);
extern uint8_t iter_to_index(int iter, int max_iter);

// Dimensions of the screen size
#define W 320
#define H 240
#define MAX_ITER 50   // keep same value used when building palette / testing

// MMIO (DTEK-V memory map / addresses)
#define VGA      ((volatile uint8_t *)0x08000000UL)   // UL stands for unsigned long (not strictly necessary)
#define SWITCH   ((volatile uint32_t *)0x04000010UL)
#define BUTTON   ((volatile uint32_t *)0x040000D0UL)

// BUTTON
#define BUTTON_EDGE         ((volatile int*) 0x040000dc)
#define BUTTON_INTERRUPT    ((volatile int*) 0x040000d8)

// Masked bits for switches and buttons
#define SWITCH_BIT_MASK  (1u << 0) // 1u means 1 unsigned. (1u << 0) is basically just 1
#define BUTTON_DRAW_MASK (1u << 0)

// GLOBAL VARIABLES
volatile int menu_state = 0;
volatile int fractal_type = 0;
static uint8_t palette[256];
static uint8_t *current_palette; // pointer for choosed pallette, needs for draw_fractal

// Initial center of the Fractals
int32_t center_x = -32768;   // -0.5 * (1 << 16)
int32_t center_y = 0;

int32_t scale = 5 * (1 << 16);  // 5.0 in fixed point format (Q16.16)
int32_t pixel;

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

    // Choose fractal outside the loop for speedoptimization
    int (*fractal_func)(int32_t, int32_t, int);
    if (fractal_type == 0){
        fractal_func = mandelbrot;
    } else {
        fractal_func = burningship;
    }

    for (int py = 0; py < H; ++py) {
        for (int px = 0; px < W; ++px) {
            int32_t cx = center_x + (int32_t)(((int64_t)(px - half_w) * pixel));
            int32_t cy = center_y + (int32_t)(((int64_t)(py - half_h) * pixel));

            int iter = fractal_func(cx, cy, MAX_ITER);

            uint8_t idx = iter_to_index(iter, MAX_ITER);
            fb[py * W + px] = palette[idx]; // Drawing with vga
        }
    }
}

static volatile int last_btn = 0;

void handle_interrupt(unsigned cause) {

    int edge = *BUTTON_EDGE;
    *BUTTON_EDGE = 0; // Reset the edge button

    // Return if no edge
    if (edge == 0){
        return;
    }

    int btn = get_btn() & 1;

    if (btn){
        last_btn = 1;
        return;
    }

    if(!last_btn){
        return;
    }

    last_btn = 0;

    int sw = get_sw();


    /* ------------- 1. PALETTE MENU -------------*/
    if (menu_state == 0){
        if(sw & (1u << 0)) {
            build_palette(palette, 0); // Palette 1
            current_palette = palette;
            menu_state = 1;
            return; 
        }
        // Switch 1
        if (sw & (1u << 1)) {
            build_palette(palette, 1); // Palette 2
            current_palette = palette;
            menu_state = 1;
            return; 
        } 
        return;
    }
    /* ------------- 2. FRACTAL MENU -------------*/
    else if (menu_state == 1){

        if (sw & (1u << 0)) {// If switch 0 is on and button is pressed
                fractal_type = 0; // Mandelbrot
                draw_fractal_to_fb(fractal_type, current_palette, scale, center_x, center_y);
                menu_state = 2;
                return; 
            }
            // Switch 1
        if (sw & (1u << 1))  { // If switch 1 is on and button is pressed
                fractal_type = 1; // Burning Ship
                draw_fractal_to_fb(fractal_type, current_palette, scale, center_x, center_y);
                menu_state = 2;
                return; 
            }
            return;
    }
    /* ------------- 3. NAVIGATION STATE -------------*/
    else if (menu_state == 2){
        if (sw & (1u << 0)) { // If switch 1 is on, we go up
                    center_y += pixel; // Move the center up 
                } else if (sw & (1u << 1)) { // If switch 2 is on, we go down
                    center_y -= pixel; // Move the center down
                } else if (sw & (1u << 2)) { // If switch 3 is on, we go right
                    center_x += pixel; // Move the center right
                } else if (sw & (1u << 3)) { // If switch 4 is on, we go left
                    center_x -= pixel; // Move the center left
                } else if (sw & (1u << 4)) { // If switch 5 is on, we zoom in
                    scale -= pixel; // Zoom in by reducing scale
                } else if (sw & (1u << 5)) { // If switch 6 is on, we zoom out
                    scale += pixel; // Zoom out by increasing scale
                }
                draw_fractal_to_fb(fractal_type, current_palette, scale, center_x, center_y);
                return;
        }
}

void labinit(void) {
  asm volatile ("csrsi mstatus,3"); // mstatus = machine status control register. Enabe interrupts

  // Button
  *BUTTON_EDGE = 0; //resets edgecapture to 0
  *BUTTON_INTERRUPT = 0x1; // 1 on bit0 enables interrupt

  asm volatile ("csrsi mie,18"); // machine interrupt enable control register. Accept interrupts from Switches
}

int main(void) {
    labinit();
    clearScreen();

    pixel = (int32_t)(((int64_t)scale) / W);

    while (1) {     
        // Palette selection loop (panel 1)
        //clearScreen();
        // Fractal selection loop (panel 2)
        //clearScreen();
        // Navigation loop (panel 3)
    }

    return 0; // Does not reach here
}