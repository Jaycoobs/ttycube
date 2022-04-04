#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <termios.h>

#define FRAMETIME 30000

typedef struct {
    size_t rows;
    size_t columns;
} dimension_t;

typedef struct {
    dimension_t size;
    float* data;
} mat_t;

typedef struct {
    size_t a;
    size_t b;
} edge_t;

void clear_screen() {
    printf("\033[2J");
}

void get_cursor_pos(size_t* r, size_t* c) {
    // Save terminal attrs...
    struct termios attrs;
    tcgetattr(STDIN_FILENO, &attrs);

    // Set terminal to raw mode
    struct termios raw;
    cfmakeraw(&raw);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    // Request cursor position
    printf("\033[6n");

    // Read response...
    const size_t RESPONSE_LENGTH = 100;
    char response[RESPONSE_LENGTH];
    int in;
    size_t i = 0;
    while ((in = fgetc(stdin)) != 'R' && i < RESPONSE_LENGTH-1) {
	response[i++] = (char)in;
    }
    response[i] = '\0';
    printf("%s\n", response);

    // row begins after '['
    char* row = strchr(response, '[') + 1;

    // column begins after ';'
    char* column = strchr(row, ';') + 1;

    // Mark the end of the row text.
    *(column-1) = '\0';

    *r = atol(row);
    *c = atol(column);

    // Restore terminal attrs
    tcsetattr(STDIN_FILENO, TCSANOW, &attrs);
}

void get_terminal_size(size_t* r, size_t* c) {
    // Save cursor pos
    printf("\033[s");

    // Move cursor down and right
    printf("\033[999;999H");

    // Get the cursor position
    get_cursor_pos(r, c);

    // Restore cursor pos
    printf("\033[u");

}

void plot_char(int x, int y, char c) {
    printf("\033[%d;%dH%c", y, x, c);
}

void draw_line_low(int x0, int y0, int x1, int y1, char c) {
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
        plot_char(x, y, c);
        if (d > 0) {
            y += yi;
            d -= 2 * dx;
        }
        d += 2 * dy;
    }
}

void draw_line_high(int x0, int y0, int x1, int y1, char c) {
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
        plot_char(x, y, c);
        if (d > 0) {
            x += xi;
            d -= 2 * dy;
        }
        d += 2 * dx;
    }
}

void draw_line(int x0, int y0, int x1, int y1, char c) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    if (abs(dy) < abs(dx)) {
        if (x0 > x1)
            draw_line_low(x1, y1, x0, y0, c);
        else
            draw_line_low(x0, y0, x1, y1, c);
    } else {
        if (y0 > y1)
            draw_line_high(x1, y1, x0, y0, c);
        else
            draw_line_high(x0, y0, x1, y1, c);
    }
}

void draw(size_t edge_count, mat_t* vertices, edge_t* edges) {
    float a[2];
    float b[2];

    for (size_t i = 0; i < edge_count; i++) {
        size_t ai = edges[i].a;
        size_t bi = edges[i].b;

        a[0] = vertices->data[ai];
        a[1] = vertices->data[ai + vertices->size.columns];

        b[0] = vertices->data[bi];
        b[1] = vertices->data[bi + vertices->size.columns];

        draw_line(a[0], a[1], b[0], b[1], 'C');
    }
}

static inline void mat_multiply(mat_t* out, mat_t* a, mat_t* b) {
    assert(a->size.columns == b->size.rows);
    assert(out->size.rows == a->size.rows);
    assert(out->size.columns == b->size.columns);

    for (size_t r = 0; r < out->size.rows; r++) {
        for (size_t c = 0; c < out->size.columns; c++) {
            float sum = 0.0;

            for (size_t i = 0; i < a->size.columns; i++)
                sum += a->data[r * a->size.columns + i] * b->data[i * b->size.columns + c];

            out->data[r * out->size.columns + c] = sum;
        }
    }
}

// Rotation matrix of angle r about axis <xyz> -> m
static inline void mat_rotation(mat_t* m, float x, float y, float z, float r) {
    assert(m->size.rows == m->size.columns);
    assert(m->size.rows == 4);

    float* out = m->data;

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

static inline void mat_copy(mat_t* dest, mat_t* source) {
    assert(dest->size.rows == source->size.rows);
    assert(dest->size.columns == source->size.columns);
    size_t bytes = dest->size.rows * dest->size.columns * sizeof(float);
    memcpy(dest->data, source->data, bytes);
}

void mat_print(mat_t* m) {
    for (size_t r = 0; r < m->size.rows; r++) {
        for (size_t c = 0; c < m->size.columns; c++) {
            printf("%7.2f ", m->data[r * m->size.columns + c]);
        }
        printf("\n");
    }
}

int main(void) {
    // Current time of the animation
    int time = 0;

    // Space to hold the transformed positions of the vertices.
    float output_data[32];
    mat_t output = {
        .size = {
            .rows = 4,
            .columns = 8
        },
        .data = output_data
    };

    // Matrix of column vectors representing the vertices of a cube with side
    // length 2 and centered on 0,0,0...
    float vertex_data[] = {
        -1,  1, -1,  1, -1,  1, -1,  1,
        -1, -1,  1,  1, -1, -1,  1,  1,
        -1, -1, -1, -1,  1,  1,  1,  1,
         1,  1,  1,  1,  1,  1,  1,  1
    };
    mat_t vertices = {
        .size = {
            .rows = 4,
            .columns = 8
        },
        .data = vertex_data
    };

    // Tuples of vertex indices which are connected by edges
    edge_t edges[12] = {
        {0,1}, {1,3}, {3,2}, {2,0},
        {4,5}, {5,7}, {7,6}, {6,4},
        {0,4}, {1,5}, {2,6}, {3,7}
    };

    // Get the terminal size to determine how to scale the cube.
    size_t trows;
    size_t tcols;

    get_terminal_size(&trows, &tcols);

    float size;
    if (trows < tcols)
        size = (float)trows / 6;
    else
        size = (float)tcols / 6;

    // Transformation to move and scale the cube to fit the window.
    float view_data[] = {size,0,0,tcols/2,
                         0,size,0,trows/2,
                         0,0,size,5,
                         0,0,0,1};
    mat_t view_matrix = {
        .size = {4, 4},
        .data = view_data
    };

    // Space for the final transform of the cube.
    float transform_data[16];
    mat_t transform_matrix = {
        .size = {4, 4},
        .data = transform_data
    };

    // Populated with various rotation matrices and multiplied into the
    // final transform.
    float rotation_data[16];
    mat_t rotation_matrix = {
        .size = {4, 4},
        .data = rotation_data
    };

    // The destination for matrix multiplication cannot be one of the
    // coefficients... this matrix is used to repeatedly multiply the
    // final transform by rotation matrices.
    float working_data[16];
    mat_t working_matrix = {
	.size = {4, 4},
	.data = working_data
    };

    while (1) {
        clear_screen();
        time++;

        mat_rotation(&rotation_matrix, 0,1,0, (float)time * 0.01);
        mat_multiply(&transform_matrix, &view_matrix, &rotation_matrix);

	mat_copy(&working_matrix, &transform_matrix);
        mat_rotation(&rotation_matrix, 1,0,0, (float)time * 0.033);
        mat_multiply(&transform_matrix, &working_matrix, &rotation_matrix);

	mat_copy(&working_matrix, &transform_matrix);
        mat_rotation(&rotation_matrix, 0,0,1, (float)time * 0.021);
        mat_multiply(&transform_matrix, &working_matrix, &rotation_matrix);

        mat_multiply(&output, &transform_matrix, &vertices);

        draw(12, &output, edges);
        fflush(stdout);

        if (usleep(FRAMETIME))
            break;
    }

    return 0;
}
