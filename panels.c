#include <stdint.h>
#define W 320
#define H 240

// Colors for palettes used in UI
#define BLACK 0
#define WHITE 255

extern void buffer_swap(uint32_t bb_addr) ;

// s is a pointer to the first char in as tring, *s is the char at current position
static int string_length(const char *s){
    int n = 0; // string length counter
    while (s && *s){ // while we havent't reach the end of the stirng
        n++;
        s++; // mov epoitner to next char
    }
    return n;
}

// Alphabet in pixel format - 5 bit
static const uint8_t font5x7[][5] = { 
    {0x00,0x00,0x00,0x00,0x00}, // space
    {0x7C,0x12,0x11,0x12,0x7C},{0x7F,0x49,0x49,0x49,0x36},{0x3E,0x41,0x41,0x41,0x22},
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
// draw the fractal-chooser panel
void draw_fractal_panel_and_swap(int selected_right, int menu_state, uint32_t bb_addr, uint32_t fb_addr) {
    uint8_t *bb = (uint8_t*)bb_addr;
    const char *title;
    const char *L1;
    const char *L2;
    
    if (menu_state == 0){
        title = "CHOOSE PALETTE";
        L1 = "FIRE";
        L2 = "SEA";
    }
    if (menu_state == 1){
        title = "CHOOSE FRACTAL";
        L1 = "MANDELBROT";
        L2 = "BURNINGSHIP";
    }

    // background 
    fill_background(bb, 0, 0, W, H, (uint8_t)BLACK);

    // title 
    int tscale = 2;
    int title_w = string_length(title) * ((5 * tscale) + tscale);
    int tx = (W - title_w) / 2;
    draw_string(bb, title, tx, 8+35, tscale, (uint8_t)WHITE);

    // two boxes 
    int box_w = 140, box_h = 90, gap = 20;
    int total_w = box_w * 2 + gap;
    int left_x = (W - total_w) / 2;
    int top_y = 100;
    int left_box_x = left_x;
    int right_box_x = left_x + box_w + gap;

    if (!selected_right) {
        // left thicker outline
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
