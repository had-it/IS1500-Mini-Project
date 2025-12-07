#include <stdint.h>
#include <stddef.h>

// Dimensions of the screen size
#define W 320
#define H 240

extern void buffer_swap(void);

// s is a pointer to the first char in a string, *s is the char at current position
static int string_length(const char *s){
    int n = 0; // string length counter
    while (s && *s){
        n++;
        s++; // mov epoitner to next char
    }
    return n;
}

// An array of letteres. NOTE: Each letter has 5 pixels horizontally and 7 vertically
// Source: ChatGPT
static const uint8_t letters[][5] = { 
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

// Make the whole screen black
static void black_background(uint8_t *fb) {
    for (int i = 0; i < H; i++) {
        for (int j = 0; j < W; j++){
            fb[i * W + j] = 0;
        }
    }
}

// Drawing the border of the rectangle
static void draw_rect(uint8_t *fb, int x0, int y0, int w, int h) {
    int x1 = x0 + w - 1; 
    int y1 = y0 + h - 1;
    for (int x = x0; x <= x1; x++) {
        fb[y0 * W + x] = 255;  // Drawing along x-axis at top border
        fb[y1 * W + x] = 255;  // Drawing along x-axis at bottom border
    }
    for (int y = y0; y <= y1; y++) {
        fb[y * W + x0] = 255;  // Drawing along y-axis at the left side
        fb[y * W + x1] = 255;  // Drawing along y-axis at the right side
    }
}

static void draw_characters(uint8_t *fb, const char *ch, int x, int y, int size) {
    int new_x = x;
    while (*ch) {        // Loops all character in the string
        if (*ch != ' ') {  // Checking that there is no space
            const uint8_t *g = letters[*ch - 65];  // same index calculation

            uint8_t bits = 0;
            int px = 0;
            int py = 0;

            // Loop the 5 columns of a letter
            for (int cx = 0; cx < 5; ++cx) {
                px = new_x + cx * size;      // x position for this column
                bits = g[cx];
                // Loop the 7 rows of a column of a letter
                for (int by = 0; by < 7; by++) {
                    if (bits & (1 << by)) { // If bit is set, draw scaled pixel block
                        py = y + by * size;
                        for (int sy = 0; sy < size; sy++){
                            for (int sx = 0; sx < size; sx++){
                                fb[(py + sy) * W + (px + sx)] = 255;
                            }
                        }
                    }
                }
            }
        }
        ch++;        // We get the next char
        new_x += (6 * size);       // New position of x. 6 because a letter uses 5 pixels horizontally (+ 1 needed for space)
    }
}

// draw the fractal-chooser panel
void draw_menu_panel(int selected_right, int menu_state, uint32_t bb_addr, uint32_t fb_addr) {
    uint8_t *bb = (uint8_t*)bb_addr;    // Address to the backbuffer
    const char *title;                  // Address to the title of the panel (CHOOSE ...)
    const char *option1;                // Address to the first choice one can select
    const char *option2;                // Address to the seconds choice one can select
    int size = 2;                       // How big the strings are going to be

    // black back background (clearing the screen)
    black_background(bb);

    if (menu_state == 0){  // If it's the first menu
        title = "CHOOSE PALETTE";
        option1 = "FIRE";
        option2 = "SEA";
    }
    if (menu_state == 1){  // If it's the second menu
        title = "CHOOSE FRACTAL";
        option1 = "MANDELBROT";
        option2 = "BURNINGSHIP";
    }

    // Making/drawing the title
    int twidth = string_length(title) * (6 * size);  // 6 because a letter uses 5 pixels horizontally (+ 1 needed for space)
    int tcenter = (W - twidth) / 2;                  // Find the the width position so that title will be in the center
    draw_characters(bb, title, tcenter, 8+35, size); // Characters of title are drawn here

    // two boxes 
    int box_w = 140;                        // Width of a box
    int box_h = 90;                         // Height of a box
    int total_w = box_w * 2 + 20;           // Width of the two boxes + space betweem them
    int lbox_x = (W - total_w) / 2;         // The left side of left box
    int rbox_x = lbox_x + box_w + 20;       // The left side of right box
    int top_y = 100;                        // Space above boxes

    if (!selected_right) {
        // left thicker border
        draw_rect(bb, lbox_x - 2, top_y - 2, box_w + 4, box_h + 4);
        draw_rect(bb, lbox_x - 1, top_y - 1, box_w + 2, box_h + 2);
        draw_rect(bb, rbox_x, top_y, box_w, box_h);
    } else {
        draw_rect(bb, rbox_x - 2, top_y - 2, box_w + 4, box_h + 4);
        draw_rect(bb, rbox_x - 1, top_y - 1, box_w + 2, box_h + 2);
        draw_rect(bb, lbox_x, top_y, box_w, box_h);
    }

    int option1_w = string_length(option1) * ((5 * size) + size);
    int option2_w = string_length(option2) * ((5 * size) + size);
    int option1_x = lbox_x + (box_w - option1_w) / 2;
    int option2_x = rbox_x + (box_w - option2_w) / 2;
    int Ly = top_y + (box_h / 2) - ((7 * size) / 2);

    draw_characters(bb, option1, option1_x, Ly, size);
    draw_characters(bb, option2, option2_x, Ly, size);

    buffer_swap();
}
