#include <stdlib.h>
#include <unistd.h>
#include <curses.h>
#include <math.h>

#define FRAMETIME 30000

typedef struct Points {
	float x;
	float y;
	float z;
} Point;

typedef struct Edges {
	int a;
	int b;
} Edge;


void plotChar(int x, int y, char c) {
	mvaddch(y, x, c);
}

void drawLineLow(int x0, int y0, int x1, int y1, char c) {
    int x = x0;
    int y = y0;
    int dx = x1 - x0;
    int dy = y1 - y0;
    int yi = 1;
    if (dy < 0) {
        yi = -1;
        dy = -dy;
    }
    int d = 2 * dy - dx;
    for (; x < x1; x++) {
		plotChar(x, y, c);
        if (d > 0) {
            y += yi;
            d -= 2 * dx;
        }
        d += 2 * dy;
    }
}

void drawLineHigh(int x0, int y0, int x1, int y1, char c) {
    int x = x0;
    int y = y0;
    int dx = x1 - x0;
    int dy = y1 - y0;
    int xi = 1;
    if (dx < 0) {
        xi = -1;
        dx = -dx;
    }
    int d = 2 * dx - dy;
    for (; y < y1; y++) {
		plotChar(x, y, c);
        if (d > 0) {
            x += xi;
            d -= 2 * dy;
        }
        d += 2 * dx;
    }
}

void drawLine(int x0, int y0, int x1, int y1, char c) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    if (abs(dy) < abs(dx)) {
        if (x0 > x1)
            drawLineLow(x1, y1, x0, y0, c);
        else
            drawLineLow(x0, y0, x1, y1, c);
    } else {
        if (y0 > y1)
            drawLineHigh(x1, y1, x0, y0, c);
        else
            drawLineHigh(x0, y0, x1, y1, c);
    }
}

void multiplyMatrix(float *out, float *a, float *b) {
	int r = 0;
	int c = 0;
	float temp[16];
	for (; r < 4; r++) {
		for (c = 0; c < 4; c++) {
			temp[r*4+c] =a[r*4+0]*b[c+0] +
						a[r*4+1]*b[c+4] +
						a[r*4+2]*b[c+8] +
						a[r*4+3]*b[c+12];
		}
	}
	for (r = 0; r < 16; r++)
		out[r] = temp[r];
}

void transformPoint(Point *out, Point* in, float *mat) {
	out->x = in->x * mat[0] + in->y * mat[1] + in->z * mat[2] + mat[3];
	out->y = in->x * mat[4] + in->y * mat[5] + in->z * mat[6] + mat[7];
	out->z = in->x * mat[8] + in->y * mat[9] + in->z * mat[10] + mat[11];
}

void draw(Point *vertices, int vcount, Edge *edges, int ecount, float *mat, char c) {
	static Point pointBuffer[16];
	int i;
	for (i = 0; i < vcount; i++) 
		transformPoint(&pointBuffer[i], &vertices[i], mat);
	for (i = 0; i < ecount; i++) {
		Point a = pointBuffer[edges[i].a];
		Point b = pointBuffer[edges[i].b];
		drawLine((int)a.x, (int)a.y, (int)b.x, (int)b.y, c);
	}
}

void rotationMatrix(float *out, float x, float y, float z, float r) {
	float c = cosf(r);
	float oc = 1 -c;
	float s = sinf(r);
	out[0] = c+x*x*oc;
	out[1] = x*y*oc-z*s;
	out[2] = x*z*oc+y*s;
	out[3] = 0;

	out[4] = y*x*oc+z*s;
	out[5] = c+y*y*oc;
	out[6] = y*z*oc-x*s;
	out[7] = 0;

	out[8] = z*x*oc-y*s;
	out[9] = z*y*oc+x*s;
	out[10] = c+z*z*oc;
	out[11] = 0;

	out[12] = 0;
	out[13] = 0;
	out[14] = 0;
	out[15] = 1;
}

void printMat(float *mat) {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			printf("%f, ", mat[i*4+j]);
		}
		printf("\n");
	}
}

int main(void) {
	int time = 0;
	
	initscr();
	noecho();
	cbreak();
	curs_set(0);

	Point cube[8] = {{-1,-1,-1}, {1,-1,-1}, {1,1,-1}, {-1,1,-1},
					{-1,-1,1},{1,-1,1}, {1,1,1}, {-1,1,1}};
	
	Edge edges[12] = {{0,1}, {1,2}, {2,3}, {3,0},
					{4,5}, {5,6}, {6,7}, {7,4},
					{0,4}, {1,5}, {2,6}, {3,7}};

	float size;

	if (COLS < LINES)
		size = (float)COLS / 6;
	else
		size = (float)LINES / 6;

	float identityMat[] = {1,0,0,0,
						0,1,0,0,
						0,0,1,0,
						0,0,0,1};

	float viewMat[] = {1,0,0,COLS/2,
						0,1,0,LINES/2,
						0,0,1,5,
						0,0,0,1};
		
	float scaleMat[] = {size,0,0,0,
						0,size,0,0,
						0,0,size,0,
						0,0,0,1};

	multiplyMatrix(viewMat, viewMat, scaleMat);

	float rot[16];
	float matBuffer[16];

	while (true) {
		clear();
		time++;

		rotationMatrix(rot, 0,1,0, (float)time * 0.01);
		multiplyMatrix(matBuffer, viewMat, rot);

		rotationMatrix(rot, 1,0,0, (float)time * 0.033);
		multiplyMatrix(matBuffer, matBuffer, rot);

		rotationMatrix(rot, 0,0,1, (float)time * 0.021);
		multiplyMatrix(matBuffer, matBuffer, rot);

		draw(cube, 8, edges, 12, matBuffer, 'C');

		refresh();
		if (usleep(FRAMETIME))
			break;
	}

	endwin();
	return 0;
}
