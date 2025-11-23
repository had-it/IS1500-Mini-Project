
// Dimensions of the screen size
#define W 320
#define H 240

// Maximum number of iterations for pixels (the higher, the slower, but better quality)
#define MAXI 50


// Function for abs
float my_fabs(float f){
    if (f < 0){
        f = -f;
    }
    return f;

    // we might need to handle special cases for -0.0 and NaN (according to Copilot)
}

int main(void) {

    for (int py = 0; py < H; ++py) {
        for (int px = 0; px < W; ++px) {
            /* map pixel to complex plane: x in [-2,1], y in [-1.5,1.5] */
            float c_re = -2.0 + (3.0 * px) / (W - 1);
            float c_im =  1.5 - (3.0 * py) / (H - 1);

            float x = 0;
            float y = 0;

            int i = 0;

            // Mandelbrot [Z = Z^2 + C]
            if(fractal_type == 0){
                while (i < MAXI && x*x + y*y <= 4) {
                    float x_new = x*x - y*y + c_re; //Real
                    y = 2*x*y + c_im; //Imaginary
                    x = x_new;
                    ++i;
                }
            }

            // Burning ship [Z = (abs(Re(Z))) + i*abs((Im(Z))))^2 + C]
            if (fractal_type == 1){
                while (i < MAXI && x*x + y*y <= 4) { 
                    float abs_x = my_fabs(x);
                    float abs_y = my_fabs(y);
                    float x_new = abs_x*abs_x - abs_y*abs_y + c_re; // Real
                    y = 2 * abs_x * abs_y + c_im; // Imaginary
                    x = x_new;
                    ++i;
                }
            }

            /* simple color: inside = black, outside = gray scaled */
            if (i == MAXI) {
                pixel_color = 0; // black
            } else {
                if (color_palette == 0){
                    pixel_color = (i * 6) % 255;
                }
                if (color_palette == 0){
                    pixel_color = (i * 10 + 50) % 255;
                }
            }
        }
    }

    return 0;
}
