#include <stdint.h>
#include <stddef.h>

// Dimensions of the screen size
#define W 320
#define H 240

extern void buffer_swap(uint32_t bb_addr);

// s is a pointer to the first char in a string, *s is the char at current position
static int string_length(const char *s){
    int n = 0; // string length counter
    while (s && *s){
        n++;
        s++; // mov epoitner to next char
    }
    return n;
}


// An array of letteres
// Source: ChatGPT
static const uint8_t letters[][5] = { 
    {0x00,0x00,0x00,0x00,0x00}, // space
    {0x7C,0x12,0x11,0x12,0x7C}, // A
    {0x7F,0x49,0x49,0x49,0x36}, // B
    {0x3E,0x41,0x41,0x41,0x22}, // C
    {0x7F,0x41,0x41,0x22,0x1C}, // D
    {0x7F,0x49,0x49,0x49,0x41}, // E
    {0x7F,0x09,0x09,0x09,0x01}, // F
    {0x3E,0x41,0x49,0x49,0x7A}, // G
    {0x7F,0x08,0x08,0x08,0x7F}, // H
    {0x00,0x41,0x7F,0x41,0x00}, // I
    {0x20,0x40,0x41,0x3F,0x01}, // J
    {0x7F,0x08,0x14,0x22,0x41}, // K
    {0x7F,0x40,0x40,0x40,0x40}, // L
    {0x7F,0x02,0x0C,0x02,0x7F},
    {0x7F,0x04,0x08,0x10,0x7F},
    {0x3E,0x41,0x41,0x41,0x3E},
    {0x7F,0x09,0x09,0x09,0x06},
    {0x3E,0x41,0x51,0x21,0x5E},
    {0x7F,0x09,0x19,0x29,0x46},
    {0x46,0x49,0x49,0x49,0x31},
    {0x01,0x01,0x7F,0x01,0x01},
    {0x3F,0x40,0x40,0x40,0x3F},
    {0x1F,0x20,0x40,0x20,0x1F},
    {0x3F,0x40,0x38,0x40,0x3F},
    {0x63,0x14,0x08,0x14,0x63},
    {0x07,0x08,0x70,0x08,0x07},
    {0x61,0x51,0x49,0x45,0x43}
};

static void black_background(uint8_t *fb) {
    for (int i = 0; i < H; i++) {
        for (int j = 0; j < W; j++){
            fb[i * W + j] = 0;
        }
    }
}

static void draw_rect(uint8_t *fb, int x0, int y0, int w, int h) {
    if (w <= 0 || h <= 0) return;
    int x1 = x0 + w - 1;
    int y1 = y0 + h - 1;                            
    for (int x = x0; x <= x1; ++x) {
        fb[y0 * W + x] = 255;
        fb[y1 * W + x] = 255;
    }
    for (int y = y0; y <= y1; ++y) {
        fb[y * W + x0] = 255;
        fb[y * W + x1] = 255;
    }
}

static void draw_char(uint8_t *fb, char ch, int x, int y, int scale) {
    const uint8_t *g = letters[(ch - 'A') + 1];
    for (int cx = 0; cx < 5; ++cx) {
        uint8_t bits = g[cx];
        for (int by = 0; by < 7; ++by) {
            if (bits & (1 << by)) {
                int px = x + cx * scale;
                int py = y + by * scale;
                for (int sy = 0; sy < scale; ++sy){
                    for (int sx = 0; sx < scale; ++sx){ 
                        fb[(py + sy) * W + (px + sx)] = 255;
                    }
                }
            }
        }
    }
}

static void draw_string(uint8_t *fb, const char *s, int x, int y, int scale) {
    int new_x = x;
    while (*s) {        // Loops all string character
        if (*s == ' ') {        // Check if there is any space
            new_x += (6 * scale);       // Moves new_x by pixels
            s++;        // We get the next char
        } else {
            draw_char(fb, *s, new_x, y, scale);
            new_x += (6 * scale);
            s++;
        }
    }
}


// draw the fractal-chooser panel
void draw_fractal_panel_and_swap(int selected_right, int menu_state, uint32_t bb_addr, uint32_t fb_addr) {
    uint8_t *bb = (uint8_t*)bb_addr;
    const char *title;
    const char *option1;
    const char *option2;

    // black back background 
    black_background(bb);

    if (menu_state == 0){
        title = "CHOOSE PALETTE";
        option1 = "FIRE";
        option2 = "SEA";
    }
    if (menu_state == 1){
        title = "CHOOSE FRACTAL";
        option1 = "MANDELBROT";
        option2 = "BURNINGSHIP";
    }

    // title 
    int tscale = 2;
    int twidth = string_length(title) * ((5 * tscale) + tscale);
    int tcenter = (W - twidth) / 2;
    draw_string(bb, title, tcenter, 8+35, tscale);

    // two boxes 
    int box_w = 140, box_h = 90, gap = 20;
    int total_w = box_w * 2 + gap;
    int left_x = (W - total_w) / 2;
    int top_y = 100;
    int left_box_x = left_x;
    int right_box_x = left_x + box_w + gap;

    if (!selected_right) {
        // left thicker outline
        draw_rect(bb, left_box_x - 2, top_y - 2, box_w + 4, box_h + 4);
        draw_rect(bb, left_box_x - 1, top_y - 1, box_w + 2, box_h + 2);
        draw_rect(bb, right_box_x, top_y, box_w, box_h);
    } else {
        draw_rect(bb, right_box_x - 2, top_y - 2, box_w + 4, box_h + 4);
        draw_rect(bb, right_box_x - 1, top_y - 1, box_w + 2, box_h + 2);
        draw_rect(bb, left_box_x, top_y, box_w, box_h);
    }

    int lscale = 2;
    int option1_w = string_length(option1) * ((5 * lscale) + lscale);
    int option2_w = string_length(option2) * ((5 * lscale) + lscale);
    int option1_x = left_box_x + (box_w - option1_w) / 2;
    int option2_x = right_box_x + (box_w - option2_w) / 2;
    int Ly = top_y + (box_h / 2) - ((7 * lscale) / 2);

    draw_string(bb, option1, option1_x, Ly, lscale);
    draw_string(bb, option2, option2_x, Ly, lscale);

    buffer_swap(bb_addr);
}
