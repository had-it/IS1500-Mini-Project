/* Fractal Visualizer
Date: 2025-12-xx
Authors: 
*/

/* ------ HARDWARE CONFIGURATION ------- */

/* A framebuffer located at address 0x8000000 (a writing
to this area means writing to the screen)
• A VGA Pixelbuffer DMA at 0x4000100 (a device that
fetches data from framebuffer and sends it to the screen)
*/

// code from lectureslides Canvas xx
int main(){
    // Create a pointer to the VGA pixel buffer. This is the “drawing” area
    volatile char *VGA = (volatile char*) 0x08000000; 

    // Fill the drawing area with some values
    for (int i = 0; i < 320*480; i++){
        VGA[i] = i / 320;
    }

    unsigned int y_ofs= 0;

    // Create a pointer to the VGA DMA
    volatile int *VGA_CTRL = (volatile int*) 0x04000100;
    while (1)
    { 
        // Update the backbuffer to point to the VGA pixel buffer + 320*y_ofs
        *(VGA_CTRL+1) = (unsigned int) (VGA+y_ofs*320);
        // Write to the backbuffer control register to perform the swap.
        *(VGA_CTRL+0) = 0;
        // Increase y_ofs by one and wrap around when reaching 240
        y_ofs= (y_ofs+ 1) % 240;
        for (int i = 0; i < 1000000; i++){
            // Delay for some unit of time
            asm volatile ("nop");
        }
    }
}


// Switches and buttons




// 

int fractal_option = 0; // 0 = Mandelbrot, 1 = Burning Ship

/* ------ MAIN ------- */

main(){

    // SET UP VGA

    // CLEAR DISPLAY

    // DISPLAY FRACTAL
    fractals(fractal_option); // Mandelbrot

    while(1){
        // Switch: color palette

        // Switch: change fractal 

        // Switch: Zoom out

        // Switch: Navigation Y and X axis

        // Button: Infinite Zoom in (while pressed once - zooming in, while pressed again - zomming stops)

    }


}
