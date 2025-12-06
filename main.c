/* Fractal Visualizer
Date: 2025-12-xx
Authors: 
*/

#include <stdint.h>

// Functions from fractals.c 
extern int mandelbrot(int32_t c_re, int32_t c_im, int max_iter);
extern int burningship(int32_t c_re, int32_t c_im, int max_iter);
extern void build_palette(uint8_t pal[256], int palette);
extern uint8_t iter_to_index(int iter, int max_iter);

// Functions from panels.c
extern void draw_fractal_panel_and_swap(int selected_right, int menu_state, uint32_t bb_addr, uint32_t fb_addr);

// Dimensions of the screen size
#define W 320
#define H 240
#define MAX_ITER 50   // keep same value used when building palette / testing

// VGA
#define VGA      ((volatile uint8_t *)0x08000000UL)   // UL stands for unsigned long (not strictly necessary)
#define VGA_CTRL ((volatile uint32_t *)0x04000100UL) //-

// VGA DMA
#define DMA_SWAP        VGA_CTRL[0]
#define DMA_BACKBUFFER  VGA_CTRL[1]
#define DMA_STATUS      VGA_CTRL[3]

// Two framebuffers inside the VGA frame-memory region
#define FB_ADDR      (0x08000000u)   // Base address of framebuffer region
#define FB2_ADDR     (FB_ADDR + (W * H))   // Second framebuffer address. difference between two fbs is that one frambuffer is after another in memory

// SWITCH
#define SWITCH   ((volatile uint32_t *)0x04000010UL)

// BUTTON
#define BUTTON   ((volatile uint32_t *)0x040000D0UL)
#define BUTTON_EDGE         ((volatile int*) 0x040000dc)
#define BUTTON_INTERRUPT    ((volatile int*) 0x040000d8)

// GLOBAL VARIABLES
volatile int menu_state = 0;
volatile int fractal_type = 0;
static uint8_t palette[256];
static uint8_t *current_palette; // pointer for choosed pallette, needs for draw_fractal
static volatile int last_btn = 0; 

// Initial center of the Fractals
int32_t center_x = -32768;   // -0.5 * (1 << 16)
int32_t center_y = 0;
int32_t scale = 5 * (1 << 16);  // 5.0 in fixed point format (Q16.16)
int32_t pixel;

// current front and back buffer addresses
static uint32_t bb_addr = FB2_ADDR;
static uint32_t fb_addr = FB_ADDR;

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

void buffer_swap(uint32_t bb_addr) {
    DMA_BACKBUFFER = bb_addr; // set backbuffer address
    DMA_SWAP = 0;  // This triggers the swap

    // wait for swap to complete
    while (DMA_STATUS & 0x1) {
        continue;
    }
}

/* Draw using functions from fractals.c */
static void draw_fractal_to_fb(int fractal_type, uint8_t palette[256], int32_t scale, int32_t center_x, int32_t center_y) {
    uint8_t *bb = (uint8_t *) bb_addr; // Pointer to the backbuffer
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
        int32_t cy = center_y + (int32_t)(((int64_t)(py - half_h) * pixel));
        uint8_t *row = &bb[py * W];
        int32_t cx = center_x + (int32_t)(((int64_t)(- half_w) * pixel));
        for (int px = 0; px < W; ++px) {
            int iter = fractal_func(cx, cy, MAX_ITER);
            cx += pixel;
            uint8_t idx = iter_to_index(iter, MAX_ITER);
            row[px] = palette[idx];
        }
    }

     /* swap to the buffer we just wrote, and flip for next frame */
    buffer_swap(bb_addr);
    
    // Swap front and back buffer addresses
    uint32_t tmp = bb_addr; 
    bb_addr = fb_addr; 
    fb_addr = tmp;
}



void handle_interrupt(unsigned cause) {

    *BUTTON_EDGE = 0; // Reset the edge button
    int btn = get_btn() & 1;

    // Return if no rising edge detected 
    if (btn){
        last_btn = 1;
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
                    scale -= (pixel*10); // Zoom in by reducing scale
                } else if (sw & (1u << 5)) { // If switch 6 is on, we zoom out
                    scale += (pixel*10); // Zoom out by increasing scale
                }
                draw_fractal_to_fb(fractal_type, current_palette, scale, center_x, center_y);
                return;
        }
}

void labinit(void) {

  // Button
  *BUTTON_EDGE = 0; //resets edgecapture to 0
  *BUTTON_INTERRUPT = 0x1; // 1 on bit0 enables interrupt
  asm volatile ("csrsi mstatus,3"); // mstatus = machine status control register. Enable interrupts
  asm volatile ("csrsi mie,18"); // machine interrupt enable control register. Accept interrupts from Switches
}

int main(void) {
    labinit();

    /* ensure the palette used for fractals is installed (also provides UI black/white if your build_palette does so) */
    build_palette(palette, 0);
    current_palette = palette;

    clearScreen();

    pixel = (int32_t)(((int64_t)scale) / W);
    
while (1) {
        if (menu_state == 0) {
            static int last_sw0 = -1;
            int sw0 = (get_sw() & 1) ? 1 : 0;
            if (sw0 != last_sw0) {
                draw_fractal_panel_and_swap(sw0, menu_state, bb_addr, fb_addr);
                last_sw0 = sw0;
            }
            for (volatile int d = 0; d < 20000; ++d) ;
        } else if (menu_state == 1) {
            /* keep chooser visible and update when SW0 toggles */
            static int last_sw0 = -1;
            int sw0 = (get_sw() & 1) ? 1 : 0;
            if (sw0 != last_sw0) {
                draw_fractal_panel_and_swap(sw0, menu_state, bb_addr, fb_addr);
                last_sw0 = sw0;
            }
            for (volatile int d = 0; d < 20000; ++d) ;
        } else {
            asm volatile ("wfi");
        }
    }
    return 0; // Does not reach here
}