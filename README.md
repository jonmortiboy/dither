# dither.exe
A small program written in C demonstrating implementations of common image dithering algorithms such as Floyd-Steinberg, error-diffuse dithering and ordered dithering. The program takes an image file and optionally a text file containing a list of hex codes to use as a colour palette.

The code is in <em>dither.c</em>. Methods prefixed with <em>render</em> (render_monochrome_1D_errordiffuse, render_monochrome_floydstein, render_monochrome_ordered4x4 etc.) contain the implementations of the different algorithms for reference. The final quantised colour for each pixel is determined by the shortest euclidean distance between colours: <em>sqrt(Δr + Δg + Δb)</em>

# usage
Dither.exe runs in the Windows command prompt (or under any terminal emulator on Windows):

dither.exe <em>imagefile.png</em> <em>palette.txt</em> <br />
./dither <em>imagefile.png</em> <em>palette.txt</em> (unix/bash terminal emulators)

<em>imagefile.png</em>   - A <em>png</em> image (also supports <em>jpeg</em>) <br />
<em>palette.txt</em>     - A text file with hex colour codes arranged on separate lines (see included files for examples)

Move back and forth between the different dithering algorithms using the < arrow keys >.

# compilation
A windows build of the program is included with the required DLLs. The program requires <em>SDL2</em> and <em>SDL2_Image</em> which must be downloaded if you wish to compile the code for a different OS.
