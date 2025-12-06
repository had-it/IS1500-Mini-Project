FRACTAL VISUALIZER
------------------

Description:
The Fractal Visalizer graphically draws either the Mandelbrot Set or Burning Ship Fractal using the VGA framebuffer and DTEK-V Board.
The fractals are graphically represented in one of the two palettes - Lava and Sea. 
The fractals are interactive and can be moved along the x-, y- and z-axis.

Requirements:
- DTEK-V Board
- VGA Cable
- A screen

Compilation:
1. Open the terminal
2. Go to the directory of the files
3. Compile by typing the command: make 

Running the code:
After compiling, run the program by typing: dtekv-run main.bin

Program Instructions:
The program itself is divided int three Menus. 
By manipulating the I/O peripherals of the DTEK-V Board, you will get diffrent outputs.

- Menu 0:
In this menu, the user chooses the palette they want the frractals to be drawn with. 
There will appear a panel with two boxes with the names of the palettes.
The user can choose between two palettes - "SEA" and "FIRE". 
-- Sw0 is on: Selects SEA
-- Sw0 is off: Selects FIRE
-- KEY1: Chooses the palette the user has selected and goes to Menu 1

- Menu 1:
In this menu, the user chooses which of the two fractals you want to visualize. 
You will see a panel with two boxes with the names of the fractals.
The fractals that the user can select are Mandelbrot Set and Burning Ship Fractal, 
which are shown on the panel as "MANDELBROT" and "BURNING", respectively. 
-- SW0 is on: Selects MANDELBROT
-- SW0 is off: Selects BURNINGSHIP
-- KEY1: Chooses the fractal the user has selected and goes to Menu 2

- Menu 2:
In this menu, the chosen and palette and fractal is graphically visualized on the screen. 
Moreover, the user is able to interact with the fractal by moving it along the x-, y- and z axis on the screen.
Note: Before toggling a switch, make sure that the rest are off. 
-- SW0 is on: Moves the fractal up along the y-axis
-- SW1 is on: Moves the fractal down along the y-axis
-- SW2 is on: Moves the fractal left along the x-axis
-- SW3 is on: Moves the fractal right along the y-axis
-- SW4 is on: Zoom into the fractal along the z-axis
-- SW5 is on: Zoom out of the fractal along the z-axis

Note that in any instance of the program, you will reset the board when you press KEY0.


Authors:
Eliza Kizowska
Hadia Abdulova
