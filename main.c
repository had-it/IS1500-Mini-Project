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

// Buttons
int get_btn(void) {
  volatile int* button = (volatile int*) 0x040000d0; // The button is mapped to memory address 0x040000d0.
  return (*button) & 1; // Return current status of the push-button (which lies in lsb)
}

// Swtiches
int get_sw( void ){
  volatile int* switches = (volatile int*) 0x04000010;  //  The toggles (or switches) are mapped to memory address 0x04000010
  return (*switches) & 0x3FF; // Checks the status of the switches in the 10 lsbs
}




// 

int fractal_type = 0; // 0 = Mandelbrot, 1 = Burning Ship



// Labinit and timeinterrupt
// Code from Lab3

/* Below is the function that will be called when an interrupt is triggered. */
void handle_interrupt(unsigned cause) 
{
    volatile unsigned int* T_Stat = (volatile unsigned int*) 0x04000020;
    *T_Stat = 0;     // Clear TO bit, resetting the status
    timeoutcount++;

    if(timeoutcount == 10){
        timeoutcount = 0; 
        // set_display with first argument being the display and second being the int do be displayed
        for(int i = 1; i<= 6; i++){
            set_displays(i, get_time_digit(mytime, i));
        }
        tick( &mytime ); // Ticks the clock once
    }
    *(T_Ctrl) = 0x5;
}

/* Add your code here for initializing interrupts. */ // Local interupts
void labinit(void)
{
    
  int period = ((30000000/10) - 1); // 10 ms (-1 because count to 0)
  volatile unsigned short* T_periodLo = (unsigned short*) 0x04000028;
  volatile unsigned short* T_periodHi = (unsigned short*) 0x0400002c;

  *(T_periodLo) = period & 0xFFFF; // (16 LSB)
  *(T_periodHi) = period >> 16; // (shifts 16 bits to the left = 16 MSB)
  
  *(T_Ctrl) = 0x5; // Start the timer and ITO=1
  asm volatile ("csrsi mie,16"); // machine interrupt enable control register. Accept interrupts from Timer
  asm volatile ("csrsi mstatus,3"); // mstatus = machine status control register. Enabe interrupts
}

/* ------ MAIN ------- */

main(){
    labinit();

    // SET UP VGA

    // CLEAR DISPLAY

    // DISPLAY FRACTAL
    fractals(fractal_type); // Mandelbrot

    while(1){

        // Switch: color palette
            // time interrupt

        // Switch: change fractal
            // time interrupt

        // Switch: Zoom out
 
        // Switch: Navigation Y and X axis

        // Button: Infinite Zoom in (while pressed once - zooming in, while pressed again - zomming stops)


    }
    return 0;


}
