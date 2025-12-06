/* main.c - Fractal Visualizer (no <string.h> dependency)
   - Simple fractal chooser panel (mainp) and fractal drawing
   - Uses your existing fractals.c functions:
       mandelbrot(), burningship(), build_palette(), iter_to_index()
   - UI uses two palette indices: 0 = black, 255 = white (must be true in palette)
*/

#include <stdint.h> // library for uint8_t, int32_t, etc.

// Functions from fractals.c
extern int mandelbrot(int32_t c_re, int32_t c_im, int max_iter);
extern int burningship(int32_t c_re, int32_t c_im, int max_iter);
extern void build_palette(uint8_t pal[256], int palette);
extern uint8_t iter_to_index(int iter, int max_iter);

// Dimensions of the screen size
#define W 320
#define H 240
#define MAX_ITER 50 // how many iterations for fractal calculation

// VGA
#define VGA      ((volatile uint8_t *)0x08000000UL) // Address of VGA framebuffer
#define VGA_CTRL ((volatile uint32_t *)0x04000100UL) // Address of VGA control registers

// VGA DMA
#define DMA_SWAP        VGA_CTRL[0]  // Adress to trigger buffer swap
#define DMA_BACKBUFFER  VGA_CTRL[1]  // Address to set backbuffer address
#define DMA_STATUS      VGA_CTRL[3]  // Address to check DMA status

// Framebuffer addresses
#define FB_ADDR   (0x08000000u) // Base address of framebuffer region
#define FB2_ADDR  (FB_ADDR + (W * H)) // same as FB_ADDR + size of one framebuffer. Difference between two fbs is that one frambuffer is after another in memory

// SWITCH
#define SWITCH   ((volatile uint32_t *)0x04000010UL)

// BUTTON
/* BUTTON has uint32_t while others have int because... */
#define BUTTON            ((volatile uint32_t *)0x040000D0UL)
#define BUTTON_EDGE       ((volatile int*) 0x040000dc)
#define BUTTON_INTERRUPT  ((volatile int*) 0x040000d8)

// Colors for palettes used in UI
#define BLACK 0
#define WHITE 255

// GLOBAL VARIABLES
volatile int menu_state = 0;       // 0 = palette menu, 1 = fractal menu, 2 = navigation
volatile int fractal_type = 0;     // 0 = mandelbrot, 1 = burningship
static uint8_t palette[256];       // storage for current palette
static uint8_t *current_palette;   // pointer for chosen palette, needed for draw_fractal
static volatile int last_btn = 0;  // last button state

// Parameters for fractal drawing
int32_t center_x = -32768;    // -0.5 * (1 << 16). -32768 is the fixed point equivelent of -0.5
int32_t center_y = 0;
int32_t scale = 5 * (1 << 16); // 5.0 in fixed point format (Q16.16)
int32_t pixel;

/* Remove  FB2_ADDR/FB_ADDR and add their values here*/
static uint32_t bb_addr = FB2_ADDR;
static uint32_t fb_addr = FB_ADDR;

// Function that calculates length of string
// s is a pointer to the first char in a string (null-terminated string)
// *s is the char at the current position
static int string_length(const char *s) {
    int n = 0; // length counter of string
    while (s && *s) { // while we haven't reached the null in the string (the end of the string)
        n++; 
        s++; // move the pointer to the next char in the string
    }
    return n;
}

// Getting switch and button
static inline int get_sw(void) { 
    return (int)(*SWITCH); 
}
static inline int get_btn(void) { 
    return (int)(*BUTTON); 
}

 // Clearing the whole VGA buffer by setting all pixels to black (0)
static void clearScreen(void) {
    volatile uint8_t *fb = VGA;
    for (int i = 0; i < W*H; i++) {
        fb[i] = 0;
    }
}

static void buffer_swap(uint32_t bb_addr) {
    DMA_BACKBUFFER = bb_addr; // set backbuffer address
    DMA_SWAP = 0;  // This triggers the swap

    // wait for swap to complete
    while (DMA_STATUS & 0x1) { 
        continue; 
    }
}

/* ---------- 5x7 font + drawing helpers ---------- */
static const uint8_t font5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, // Space
    {0x7C,0x12,0x11,0x12,0x7C}, // A
    {0x7F,0x49,0x49,0x49,0x36}, 
    {0x3E,0x41,0x41,0x41,0x22},
    {0x7F,0x41,0x41,0x22,0x1C},{0x7F,0x49,0x49,0x49,0x41},{0x7F,0x09,0x09,0x09,0x01},
    {0x3E,0x41,0x49,0x49,0x7A},{0x7F,0x08,0x08,0x08,0x7F},{0x00,0x41,0x7F,0x41,0x00},
    {0x20,0x40,0x41,0x3F,0x01},{0x7F,0x08,0x14,0x22,0x41},{0x7F,0x40,0x40,0x40,0x40},
    {0x7F,0x02,0x0C,0x02,0x7F},{0x7F,0x04,0x08,0x10,0x7F},{0x3E,0x41,0x41,0x41,0x3E},
    {0x7F,0x09,0x09,0x09,0x06},{0x3E,0x41,0x51,0x21,0x5E},{0x7F,0x09,0x19,0x29,0x46},
    {0x46,0x49,0x49,0x49,0x31},{0x01,0x01,0x7F,0x01,0x01},{0x3F,0x40,0x40,0x40,0x3F},
    {0x1F,0x20,0x40,0x20,0x1F},{0x3F,0x40,0x38,0x40,0x3F},{0x63,0x14,0x08,0x14,0x63},
    {0x07,0x08,0x70,0x08,0x07},{0x61,0x51,0x49,0x45,0x43}
};

static inline void put_pixel(uint8_t *fb, int x, int y, uint8_t col) {
    if ((unsigned)x >= (unsigned)W || (unsigned)y >= (unsigned)H) return;
    fb[y * W + x] = col;
}

static void fill_background(uint8_t *fb, int x0, int y0, int w, int h, uint8_t col) {
    if (w <= 0 || h <= 0) return;
    int x1 = x0 + w;
    int y1 = y0 + h;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > W) x1 = W;
    if (y1 > H) y1 = H;
    for (int y = y0; y < y1; ++y)
        for (int x = x0; x < x1; ++x)
            fb[y * W + x] = col;
}

static void rect(uint8_t *fb, int x0, int y0, int w, int h, uint8_t col) {
    if (w <= 0 || h <= 0) return;
    int x1 = x0 + w - 1;
    int y1 = y0 + h - 1;
    for (int x = x0; x <= x1; ++x) {
        put_pixel(fb, x, y0, col);
        put_pixel(fb, x, y1, col);
    }
    for (int y = y0; y <= y1; ++y) {
        put_pixel(fb, x0, y, col);
        put_pixel(fb, x1, y, col);
    }
}

static void draw_char(uint8_t *fb, char ch, int x, int y, int scale, uint8_t col) {
    if (ch == ' ') return;
    if (ch >= 'a' && ch <= 'z') ch = ch - 'a' + 'A';
    if (ch < 'A' || ch > 'Z') return;
    const uint8_t *g = font5x7[(ch - 'A') + 1];
    for (int cx = 0; cx < 5; ++cx) {
        uint8_t bits = g[cx];
        for (int by = 0; by < 7; ++by) {
            if (bits & (1 << by)) {
                int px = x + cx * scale;
                int py = y + by * scale;
                for (int sy = 0; sy < scale; ++sy)
                    for (int sx = 0; sx < scale; ++sx)
                        put_pixel(fb, px + sx, py + sy, col);
            }
        }
    }
}

static void draw_string(uint8_t *fb, const char *s, int x, int y, int scale, uint8_t col) {
    int gap = scale;
    int cx = x;
    while (*s) {
        if (*s == ' ') { cx += (5 * scale) + gap; ++s; continue; }
        draw_char(fb, *s, cx, y, scale, col);
        cx += (5 * scale) + gap;
        ++s;
    }
}

/* ---------- draw the fractal-chooser panel (mainp) ---------- */
static void draw_fractal_panel_and_swap(int selected_right) {
    uint8_t *bb = (uint8_t*)bb_addr;
    const char *title;
    const char *L1;
    const char *L2;
    
    if (menu_state == 0){
        title = "CHOOSE PALETTE";
        L1 = "SEA";
        L2 = "LAVA";
    }
    if (menu_state == 1){
        title = "CHOOSE FRACTAL";
        L1 = "MANDELBROT";
        L2 = "BURNINGSHIP";
    }


    /* background */
    fill_background(bb, 0, 0, W, H, (uint8_t)BLACK);

    /* title */
    int tscale = 2;
    int title_w = string_length(title) * ((5 * tscale) + tscale);
    int tx = (W - title_w) / 2;
    draw_string(bb, title, tx, 8+35, tscale, (uint8_t)WHITE);

    /* two boxes */
    int box_w = 140, box_h = 90, gap = 20;
    int total_w = box_w * 2 + gap;
    int left_x = (W - total_w) / 2;
    int top_y = 100;
    int left_box_x = left_x;
    int right_box_x = left_x + box_w + gap;

    if (!selected_right) {
        /* left thicker outline */
        rect(bb, left_box_x - 2, top_y - 2, box_w + 4, box_h + 4, (uint8_t)WHITE);
        rect(bb, left_box_x - 1, top_y - 1, box_w + 2, box_h + 2, (uint8_t)WHITE);
        rect(bb, right_box_x, top_y, box_w, box_h, (uint8_t)WHITE);
    } else {
        rect(bb, right_box_x - 2, top_y - 2, box_w + 4, box_h + 4, (uint8_t)WHITE);
        rect(bb, right_box_x - 1, top_y - 1, box_w + 2, box_h + 2, (uint8_t)WHITE);
        rect(bb, left_box_x, top_y, box_w, box_h, (uint8_t)WHITE);
    }

    int lscale = 2;
    int L1_w = string_length(L1) * ((5 * lscale) + lscale);
    int L2_w = string_length(L2) * ((5 * lscale) + lscale);
    int L1_x = left_box_x + (box_w - L1_w) / 2;
    int L2_x = right_box_x + (box_w - L2_w) / 2;
    int Ly = top_y + (box_h / 2) - ((7 * lscale) / 2);

    draw_string(bb, L1, L1_x, Ly, lscale, (uint8_t)WHITE);
    draw_string(bb, L2, L2_x, Ly, lscale, (uint8_t)WHITE);

    buffer_swap(bb_addr);
    uint32_t tmp = bb_addr; bb_addr = fb_addr; fb_addr = tmp;
}

/* ---------- fractal drawing (your logic) ---------- */
static void draw_fractal_to_fb(int fractal_type_local, uint8_t palette_local[256],
                               int32_t scale_local, int32_t center_x_local, int32_t center_y_local)
{
    uint8_t *bb = (uint8_t*)bb_addr;
    int32_t pix = (int32_t)(((int64_t)scale_local) / W);
    int half_w = W / 2;
    int half_h = H / 2;

    int (*fractal_func)(int32_t,int32_t,int) =
        (fractal_type_local == 0) ? mandelbrot : burningship;

    for (int py = 0; py < H; ++py) {
        int32_t cy = center_y_local + (int32_t)(((int64_t)(py - half_h) * pix));
        uint8_t *row = &bb[py * W];
        int32_t cx = center_x_local + (int32_t)(((int64_t)(- half_w) * pix));
        for (int px = 0; px < W; ++px) {
            int iter = fractal_func(cx, cy, MAX_ITER);
            cx += pix;
            uint8_t idx = iter_to_index(iter, MAX_ITER);
            row[px] = palette_local[idx];
        }
    }

    buffer_swap(bb_addr);
    uint32_t tmp = bb_addr; bb_addr = fb_addr; fb_addr = tmp;
}

/* ---------- interrupt handler (updated flow) ---------- */
void handle_interrupt(unsigned cause)
{

    *BUTTON_EDGE = 0;

    int btn = get_btn() & 1;
    if (btn) { last_btn = 1; return; } /* handle on release */
    last_btn = 0;

    int sw = get_sw();

    /* 1) Palette menu */
    if (menu_state == 0) {
        if (sw & (1u << 0)) {
            build_palette(palette, 0);
            current_palette = palette;
            menu_state = 1;
            draw_fractal_panel_and_swap((get_sw() & 1) ? 1 : 0);
            return;
        }
        if (sw & (1u << 1)) {
            build_palette(palette, 1);
            current_palette = palette;
            menu_state = 1;
            draw_fractal_panel_and_swap((get_sw() & 1) ? 1 : 0);
            return;
        }
        return;
    }

    /* 2) Fractal chooser panel (mainp) - confirm on button release */
    else if (menu_state == 1) {
        fractal_type = (sw & 1) ? 1 : 0;
        draw_fractal_to_fb(fractal_type, current_palette, scale, center_x, center_y);
        menu_state = 2;
        return;
    }

    /* 3) Navigation state */
    else if (menu_state == 2) {
        if (sw & (1u << 0)) center_y += pixel;
        else if (sw & (1u << 1)) center_y -= pixel;
        else if (sw & (1u << 2)) center_x += pixel;
        else if (sw & (1u << 3)) center_x -= pixel;
        else if (sw & (1u << 4)) scale -= (pixel * 10);
        else if (sw & (1u << 5)) scale += (pixel * 10);

        draw_fractal_to_fb(fractal_type, current_palette, scale, center_x, center_y);
        return;
    }
}

/* ---------- init + main ---------- */
void labinit(void)
{
    asm volatile ("csrsi mstatus,3"); /* enable interrupts */
    *BUTTON_EDGE = 0;
    *BUTTON_INTERRUPT = 0x1;
    asm volatile ("csrsi mie,18");
}

int main(void)
{
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
                draw_fractal_panel_and_swap(sw0);
                last_sw0 = sw0;
            }
            for (volatile int d = 0; d < 20000; ++d) ;
        } else if (menu_state == 1) {
            /* keep chooser visible and update when SW0 toggles */
            static int last_sw0 = -1;
            int sw0 = (get_sw() & 1) ? 1 : 0;
            if (sw0 != last_sw0) {
                draw_fractal_panel_and_swap(sw0);
                last_sw0 = sw0;
            }
            for (volatile int d = 0; d < 20000; ++d) ;
        } else {
            asm volatile ("wfi");
        }
    }

    return 0;
}