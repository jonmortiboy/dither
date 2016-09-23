#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

SDL_Renderer* renderer;
SDL_Texture* texture;
SDL_Surface* image, *buffer;

Uint32* pixels, *imgpixels;
int screenw, screenh;
int imgw, imgh;	

typedef struct RGB {
	int r;
	int g;
	int b;
} RGB;

// Define the 4bit colour palette
int numCols = 16;
RGB cols4bit[] = { 	{0,0,0}, {128,0,0}, {255,0,0}, {255,0,255},
					{0,128,128}, {0,128,0}, {0,255,0}, {0,255,255},
					{0,0,128}, {128,0,128}, {0,0,255}, {192,192,192},
				 	{128,128,128}, {128,128,0}, {255,255,0}, {255,255,255} };

RGB* cols = cols4bit;
int readPalette = 0;

RGB getRGB(Uint32* pixels, int x, int y);
void setRGB(Uint32* pixels, int x, int y, RGB rgb);
RGB difRGB(RGB from, RGB to);
RGB addRGB(RGB a, RGB b);
RGB divRGB(RGB rgb, double d);
RGB mulRGB(RGB rgb, double d);
RGB nearestRGB(RGB rgb, RGB* rgbs, int numRGBs);
double distRGB(RGB from, RGB to);

void readPaletteFile(char filename[]);

void reset_pixels();
void render();

void render_normal();
void render_monochrome();
void render_monochrome_1D_errordiffuse();
void render_monochrome_floydstein();
void render_monochrome_ordered4x4();
void render_monochrome_ordered8x8();
void render_monochrome_orderedhalftone();

void render_4bit();
void render_4bit_1D_errordiffuse();
void render_4bit_floydstein();
void render_4bit_ordered4x4();
void render_4bit_ordered8x8();
void render_4bit_orderedhalftone();

int functionCount = 13;
int currentFunction = 0;
void (*render_functions[])() = { render_normal, render_monochrome, render_monochrome_1D_errordiffuse,
								render_monochrome_floydstein, render_monochrome_ordered4x4, 
								render_monochrome_ordered8x8, render_monochrome_orderedhalftone, render_4bit,
								render_4bit_1D_errordiffuse, render_4bit_floydstein,
								render_4bit_ordered4x4, render_4bit_ordered8x8,
								render_4bit_orderedhalftone};

char *function_names[] = {	"normal", "monochrome", "monochrome 1D error diffuse", 
							"monochrome floyd-steinberg", "monochrome ordered 4x4", 
							"monochrome ordered 8x8", "monochrome ordered halftone", "palette",
							 "palette 1D error diffuse", "palette floyd-steinberg", 
							 "palette ordered 4x4", "palette ordered 8x8", "palette ordered halftone" };

int main(int argc, char** argv) {
	if (SDL_Init(SDL_INIT_VIDEO)) {
		printf("SDL_Init Error");
		return 1;
	}

	if (argc > 1)
		image = IMG_Load(argv[1]);
	else 
		image = IMG_Load("./christine.png");
	
	if (!image)  {
		printf("Image not loaded :(\n");
		SDL_Quit();
		return 0;
	}
	
	if (argc > 2) 
		readPaletteFile(argv[2]);
	
	imgw = image->w;
	imgh = image->h;
	imgpixels = image->pixels;
	pixels = (Uint32*)malloc(imgw * imgh * sizeof(Uint32));

	buffer = SDL_CreateRGBSurface(0, imgw, imgh, 32, 0xff, 0xff00, 0xff0000, 0xff000000);
	
	screenw = imgw;
	screenh = imgh;
	
	SDL_Window* window = SDL_CreateWindow("normal",
											SDL_WINDOWPOS_CENTERED,
											SDL_WINDOWPOS_CENTERED,
											screenw, screenh, 0);

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	render_normal();
	
	SDL_Event e;
	while (1) {
		SDL_WaitEvent(&e);
		if (e.type == SDL_QUIT) break;
		if (e.type == SDL_KEYDOWN) {
			if (e.key.keysym.sym == SDLK_RIGHT) {
				currentFunction++;
				if (currentFunction < 0) currentFunction = functionCount-1;
				if (currentFunction >= functionCount) currentFunction = 0;
				render_functions[currentFunction]();
				SDL_SetWindowTitle(window, function_names[currentFunction]);
			}
			
			if (e.key.keysym.sym == SDLK_LEFT) {
				currentFunction--;
				if (currentFunction < 0) currentFunction = functionCount-1;
				if (currentFunction >= functionCount) currentFunction = 0;
				render_functions[currentFunction]();			
				SDL_SetWindowTitle(window, function_names[currentFunction]);
			}
		}
	}

	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_FreeSurface(image);
	SDL_FreeSurface(buffer);
	free(pixels);
	if (readPalette) free(cols);
	SDL_Quit();
	return 0;
}

void readPaletteFile(char filename[]) {
	FILE* fp;
	char line[8];
		
	int linecount = 0;
	
	fp = fopen(filename, "r");
	
	// Count lines
	while (fgets(line, 8, fp) != NULL) {
		linecount++;
	}
	
	numCols = linecount;
	
	rewind(fp);	
	cols = malloc(sizeof(RGB)*linecount);
	
	int i = 0;
	while (fgets(line, 8, fp) != NULL) {
		int col = (int)strtol(line, NULL, 16);
		cols[i].r = (col & 0xff0000) >> 16;
		cols[i].g = (col & 0xff00) >> 8;
		cols[i].b = (col & 0xff);
		
		i++;
	}
	
	readPalette = 1;
	fclose(fp);
}

void reset_pixels() {
	memcpy(pixels, imgpixels, imgw * imgh * sizeof(Uint32));
}

RGB getRGB(Uint32* pixels, int x, int y) {
	RGB rgb; rgb.r = 0; rgb.g = 0; rgb.b = 0;
	
	if (x < 0 || x >= imgw || y < 0 || y >= imgh) return rgb;
	
	rgb.r = (pixels[y * imgw + x] & 0xff);
	rgb.g = (pixels[y * imgw + x] & 0xff00) >> 8;
	rgb.b = (pixels[y * imgw + x] & 0xff0000) >> 16;
	
	return rgb;
}

void setRGB(Uint32* pixels, int x, int y, RGB rgb) {
	if (x < 0 || x >= imgw || y < 0 || y >= imgh) return;
	
	pixels[y * imgw + x] = 0xff000000 + (rgb.r) + (rgb.g << 8) + (rgb.b << 16);
}

RGB difRGB(RGB from, RGB to) {
	RGB dif;
	dif.r = to.r - from.r;
	dif.g = to.g - from.g;
	dif.b = to.b - from.b;
	
	return dif;
}

RGB addRGB(RGB a, RGB b) {
	RGB sum;
	sum.r = a.r + b.r;
	sum.g = a.g + b.g;
	sum.b = a.b + b.b;
	
	if (sum.r > 255) sum.r = 255; if (sum.r < 0) sum.r = 0;
	if (sum.g > 255) sum.g = 255; if (sum.g < 0) sum.g = 0;
	if (sum.b > 255) sum.b = 255; if (sum.b < 0) sum.b = 0;
	
	return sum;
}

RGB divRGB(RGB rgb, double d) {
	RGB div;
	div.r = (int)((double)rgb.r/d);
	div.g = (int)((double)rgb.g/d);
	div.b = (int)((double)rgb.b/d);
	
	return div;
}

RGB mulRGB(RGB rgb, double d) {
	RGB mul;
	mul.r = (int)((double)rgb.r*d);
	mul.g = (int)((double)rgb.g*d);
	mul.b = (int)((double)rgb.b*d);
	
	return mul;
}

double distRGB(RGB from, RGB to) {
	RGB dif = difRGB(from, to);
	double dist = dif.r*dif.r + dif.g*dif.g + dif.b*dif.b;
	
	return dist;
}

RGB nearestRGB(RGB rgb, RGB rgbs[], int numRGBs) {
	double dist = -1, tempDist;
	RGB nearest;
	
	int i;
	for (i = 0; i < numRGBs; i++) {
		tempDist = distRGB(rgb, rgbs[i]);
		
		if (tempDist < dist || dist < 0) {
			dist = tempDist;
			nearest = rgbs[i];
		}
	}
	
	return nearest;
}

void render() {
	memcpy((Uint32*)buffer->pixels, pixels, imgw * imgh * sizeof(Uint32));

	SDL_DestroyTexture(texture);
	texture = SDL_CreateTextureFromSurface(renderer, buffer);
	SDL_RenderClear(renderer);
	
	SDL_Rect rec; rec.x = 0; rec.y = 0; rec.w = screenw; rec.h = screenh;
	SDL_RenderCopy(renderer, texture, NULL, &rec);
	SDL_RenderPresent(renderer);
}

void render_normal() {
	reset_pixels();
	render();
}

void render_monochrome() {
	reset_pixels();
	
	int i;
	RGB rgb;
	for (i = 0; i < imgw * imgh; i++) {
		rgb = getRGB(pixels, i%imgw, i/imgw); 
		
		double lum = 0.2126*rgb.r + 0.7152*rgb.g + 0.0722*rgb.b;
		
		pixels[i] = (lum > 128) ? 0xffffffff : 0x00000000;
	}
	
	render();
}

void render_monochrome_1D_errordiffuse() {
	reset_pixels();
	
	int i;
	RGB rgb, rgberror;
	for (i = 0; i < imgw * imgh; i++) {
		rgb = getRGB(pixels, i%imgw, i/imgw); 
		
		double lum = 0.2126*rgb.r + 0.7152*rgb.g + 0.0722*rgb.b;
		RGB black; black.r = 0;   black.g = 0;   black.b = 0;
		RGB white; white.r = 255; white.g = 255; white.b = 255;	
		
		rgberror = (lum > 128) ? difRGB(white, rgb) : difRGB(black, rgb);
		
		setRGB(pixels, (i%imgw)+1, i/imgw, addRGB(getRGB(pixels, (i%imgw)+1, i/imgw), rgberror));
		
		pixels[i] = (lum > 128) ? 0xffffffff : 0x00000000;	
	}
	
	render();
}

void render_monochrome_floydstein() {
	reset_pixels();
	
	int i, x, y;
	RGB rgb, rgberror;
	for (i = 0; i < imgw * imgh; i++) {
		rgb = getRGB(pixels, i%imgw, i/imgw); 
		
		double lum = 0.2126*rgb.r + 0.7152*rgb.g + 0.0722*rgb.b;
		RGB black; black.r = 0;   black.g = 0;   black.b = 0;
		RGB white; white.r = 255; white.g = 255; white.b = 255;	
		
		rgberror = (lum > 128) ? difRGB(white, rgb) : difRGB(black, rgb);
		rgberror = divRGB(rgberror, 16);
		
		x = i%imgw; y = i/imgw;
		
		setRGB(pixels, x+1, y, addRGB(getRGB(pixels, x+1, y), mulRGB(rgberror, 7)));
		setRGB(pixels, x-1, y+1, addRGB(getRGB(pixels, x-1, y+1), mulRGB(rgberror, 3)));
		setRGB(pixels, x, y+1, addRGB(getRGB(pixels, x, y+1), mulRGB(rgberror, 5)));
		setRGB(pixels, x+1, y+1, addRGB(getRGB(pixels, x+1, y+1), rgberror));
		
		pixels[i] = (lum > 128) ? 0xffffffff : 0x00000000;	
	}
	
	render();
}

void render_monochrome_ordered4x4() {
	reset_pixels();
	
	int bayer[] = { 1,  9,  3,  11,
					13, 5,  15, 7,
					4,  12, 2,  10,
					16, 8,  14, 6 };

	
	int i, x, y;
	RGB rgb;
	for (i = 0; i < imgw * imgh; i++) {
		x = i%imgw; y = i/imgw;
		rgb = getRGB(pixels, x, y); 
		rgb = divRGB(rgb, 10);
		rgb = mulRGB(rgb, bayer[((y%4)*4) + (x%4)]);
		
		double lum = 0.2126*rgb.r + 0.7152*rgb.g + 0.0722*rgb.b;
		pixels[i] = (lum > 128) ? 0xffffffff : 0x00000000;	
	}
	
	render();
}

void render_monochrome_ordered8x8() {
	reset_pixels();
	
	int bayer[] = { 1,  49, 13, 61, 4,  52, 16, 64,
					33, 17, 45, 29, 36, 20, 48, 32,
					9,  57, 5,  53, 12, 60, 8,  56,
					41, 25, 37, 21, 44, 28, 40, 24,
					3,  51, 15, 63, 2,  50, 14, 62,
					35, 19, 47, 31, 34, 18, 46, 30,
					11, 59, 7,  55, 10, 58, 6,  54,
					43, 27, 39, 23, 42, 26, 38, 22 };

	int i, x, y;
	RGB rgb;
	for (i = 0; i < imgw * imgh; i++) {
		x = i%imgw; y = i/imgw;
		rgb = getRGB(pixels, x, y); 
		rgb = divRGB(rgb, 17);
		rgb = mulRGB(rgb, bayer[((y%8)*8) + (x%8)]);
		
		double lum = 0.2126*rgb.r + 0.7152*rgb.g + 0.0722*rgb.b;
		pixels[i] = (lum > 128) ? 0xffffffff : 0x00000000;	
	}
	
	render();
}

void render_monochrome_orderedhalftone() {
	reset_pixels();

	int bayer[] = { 24, 10, 12, 26, 35, 47, 49, 37,
					8,  0,  2,  14, 45, 59, 61, 51,
					22, 6,  4,  16, 43, 57, 63, 53,
					30, 20, 18, 28, 33, 41, 55, 39,
					34, 46, 48, 36, 25, 11, 13, 27,
					44, 58, 60, 50, 9,  1,  3,  15,
					42, 56, 62, 52, 23, 7,  5,  17,
					32, 40, 54, 38, 31, 21, 19, 29 };
	
	int i, x, y;
	RGB rgb;
	for (i = 0; i < imgw * imgh; i++) {
		x = i%imgw; y = i/imgw;
		rgb = getRGB(pixels, x, y); 
		rgb = divRGB(rgb, 17);
		rgb = mulRGB(rgb, bayer[((y%8)*8) + (x%8)]);
		
		double lum = 0.2126*rgb.r + 0.7152*rgb.g + 0.0722*rgb.b;
		pixels[i] = (lum > 128) ? 0xffffffff : 0x00000000;	
	}
	
	render();
}

void render_4bit() {
	reset_pixels();
	
	int i;
	RGB rgb, nearest;
	for (i = 0; i < imgw * imgh; i++) {
		rgb = getRGB(pixels, i%imgw, i/imgw); 
		nearest = nearestRGB(rgb, cols, numCols);
		
		setRGB(pixels, i%imgw, i/imgw, nearest);
	}
	
	render();
}

void render_4bit_1D_errordiffuse() {
	reset_pixels();
	
	int i;
	RGB rgb, nearest, rgberror;
	for (i = 0; i < imgw * imgh; i++) {
		rgb = getRGB(pixels, i%imgw, i/imgw); 
		nearest = nearestRGB(rgb, cols, numCols);
		
		rgberror = difRGB(nearest, rgb);
		
		setRGB(pixels, (i%imgw)+1, i/imgw, addRGB(getRGB(pixels, (i%imgw)+1, i/imgw), rgberror));

		setRGB(pixels, i%imgw, i/imgw, nearest);
	}
	
	render();	
}

void render_4bit_floydstein() {
	reset_pixels();
	
	int i, x, y;
	RGB rgb, nearest, rgberror;
	for (i = 0; i < imgw * imgh; i++) {
		rgb = getRGB(pixels, i%imgw, i/imgw); 
		nearest = nearestRGB(rgb, cols, numCols);		
		
		rgberror = difRGB(nearest, rgb);
		rgberror = divRGB(rgberror, 16);
		
		x = i%imgw; y = i/imgw;
		
		setRGB(pixels, x+1, y, addRGB(getRGB(pixels, x+1, y), mulRGB(rgberror, 7)));
		setRGB(pixels, x-1, y+1, addRGB(getRGB(pixels, x-1, y+1), mulRGB(rgberror, 3)));
		setRGB(pixels, x, y+1, addRGB(getRGB(pixels, x, y+1), mulRGB(rgberror, 5)));
		setRGB(pixels, x+1, y+1, addRGB(getRGB(pixels, x+1, y+1), rgberror));
		
		setRGB(pixels, i%imgw, i/imgw, nearest);
	}
	
	render();
}

void render_4bit_ordered4x4() {
	reset_pixels();

	int bayer[] = { 1,  9,  3,  11,
					13, 5,  15, 7,
					4,  12, 2,  10,
					16, 8,  14, 6 };
	
	int i, x, y;
	RGB rgb, nearest;
	for (i = 0; i < imgw * imgh; i++) {
		x = i%imgw; y = i/imgw;
		rgb = getRGB(pixels, x, y); 
		rgb = divRGB(rgb, 10);
		rgb = mulRGB(rgb, bayer[((y%4)*4) + (x%4)]);
		
		nearest = nearestRGB(rgb, cols, numCols);
		setRGB(pixels, x, y, nearest);
	}
	
	render();
}

void render_4bit_ordered8x8() {
	reset_pixels();
	
	int bayer[] = { 1,  49, 13, 61, 4,  52, 16, 64,
					33, 17, 45, 29, 36, 20, 48, 32,
					9,  57, 5,  53, 12, 60, 8,  56,
					41, 25, 37, 21, 44, 28, 40, 24,
					3,  51, 15, 63, 2,  50, 14, 62,
					35, 19, 47, 31, 34, 18, 46, 30,
					11, 59, 7,  55, 10, 58, 6,  54,
					43, 27, 39, 23, 42, 26, 38, 22 };

	int i, x, y;
	RGB rgb, nearest;
	for (i = 0; i < imgw * imgh; i++) {
		x = i%imgw; y = i/imgw;
		rgb = getRGB(pixels, x, y); 
		rgb = divRGB(rgb, 17);
		rgb = mulRGB(rgb, bayer[((y%8)*8) + (x%8)]);
		
		nearest = nearestRGB(rgb, cols, numCols);
		setRGB(pixels, x, y, nearest);
	}
	
	render();
}

void render_4bit_orderedhalftone() {
	reset_pixels();

	int bayer[] = { 24, 10, 12, 26, 35, 47, 49, 37,
					8,  0,  2,  14, 45, 59, 61, 51,
					22, 6,  4,  16, 43, 57, 63, 53,
					30, 20, 18, 28, 33, 41, 55, 39,
					34, 46, 48, 36, 25, 11, 13, 27,
					44, 58, 60, 50, 9,  1,  3,  15,
					42, 56, 62, 52, 23, 7,  5,  17,
					32, 40, 54, 38, 31, 21, 19, 29 };

	int i, x, y;
	RGB rgb, nearest;
	for (i = 0; i < imgw * imgh; i++) {
		x = i%imgw; y = i/imgw;
		rgb = getRGB(pixels, x, y); 
		rgb = divRGB(rgb, 17);
		rgb = mulRGB(rgb, bayer[((y%8)*8) + (x%8)]);
		
		nearest = nearestRGB(rgb, cols, numCols);
		setRGB(pixels, x, y, nearest);
	}
	
	render();
}













