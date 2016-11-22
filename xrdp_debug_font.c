#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define FONT_DATASIZE(_w, _h) (((_h * ((_w + 7) / 8)) + 3) & ~3)

static int check_header(int fd) {

    unsigned char buffer[128];

    /* Header string */
    read(fd, buffer, 4);
    if (!memcmp(buffer, "FNT1", 4))
        printf("Header OK!\n");
    else
        printf("Header Error!\n");

    /* Font Family */
    read(fd, buffer, 32);
    buffer[32] = 0;
    printf("Font Family: %s\n", buffer);

    /* Font size */
    read(fd, buffer, 2);
    printf("Font Size: %i\n", *((short*) buffer));

    /* Font style */
    read(fd, buffer, 2);
    printf("Font style: %i\n", *((short*) buffer));

    /* Read padding */
    read(fd, buffer, 8);

    return 0;

}

static int read_glyph(int fd, char stop) {

    unsigned char buffer[128];
    short w, h;
    int r;
    static int index = 32;

    /* Width */
    printf("\twidth: ");
    r = read(fd, buffer, 2);
    if (r == -1 || !r) {
        if (r == -1)
            printf("Error: %s.\n", strerror(errno));
        else
            printf("Unexpected end of file.\n");

        return r;
    }
    w = *((short*) buffer);
    printf("%i\n", w);

    /* Height */
    printf("\theight: ");
    r = read(fd, buffer, 2);
    if (r == -1 || !r) {
        if (r == -1)
            printf("Error: %s.\n", strerror(errno));
        else
            printf("Unexpected end of file.\n");

        return r;
    }
    h = *((short*) buffer);
    printf("%i\n", h);

    /* Baseline */
    printf("\tbaseline: ");
    r = read(fd, buffer, 2);
    if (r == -1 || !r) {
        if (r == -1)
            printf("Error: %s.\n", strerror(errno));
        else
            printf("Unexpected end of file.\n");

        return r;
    }
    printf("%i\n", *((short*) buffer));

    /* Offset */
    printf("\toffset: ");
    r = read(fd, buffer, 2);
    if (r == -1 || !r) {
        if (r == -1)
            printf("Error: %s.\n", strerror(errno));
        else
            printf("Unexpected end of file.\n");

        return r;
    }
    printf("%i\n", *((short*) buffer));

    /* Incby */
    printf("\tincby: ");
    r = read(fd, buffer, 2);
    if (r == -1 || !r) {
        if (r == -1)
            printf("Error: %s.\n", strerror(errno));
        else
            printf("Unexpected end of file.\n");

        return r;
    }
    printf("%i\n", *((short*) buffer));

    /* Padding */
    r = read(fd, buffer, 6);
    if (r == -1 || !r) {
        if (r == -1)
            printf("Error: %s.\n", strerror(errno));
        else
            printf("Unexpected end of file.\n");

        return r;
    }

    /* Glyph Character */
    if (isgraph(index))
        printf("\tchar: '%c' %i\n", index, index);
    else
        printf("\tchar: '%s' %i\n", "n/a", index);

    /* Glyph Data */
    int glyph_size = FONT_DATASIZE(w, h);

    printf("\tglyph data (%i bytes):", glyph_size);
    r = read(fd, buffer, glyph_size);
    if (r == -1) {
        printf("Error: %s.\n", strerror(errno));
        return r;
    }

    int _w = (w + 7) & ~7;
    int roller = 0;
    int i = 0;

    while (i < glyph_size) {

        if ((roller % _w) == 0)
            printf("\n\t%i: ", i+1);

        printf("%c", (buffer[i] & (0x80 >> (roller & 7))) ? '1' : '0');

        roller++;
        if ((roller & 7) == 0)
            i++;

    }
    printf("\n\n");

    if (index == stop && stop) {
        printf("%c", stop);
        return -2;
    }
    index++;

    return r;

}


static int check_glyphs(int fd, char stop) {

    int r;

    printf("Glyphs:\n");
    if (stop)
        printf("Stop char: '%c'\n", stop);

    while (1) {

        /* Read glyph */
        r = read_glyph(fd, stop);

        /* Error */
        if (r == -1)
            return 1;

        /* Stopped at stop char */
        if (r == -2)
            return 0;

    }

    return 0;

}

int main(int argc, char** argv) {

    if (argc < 2) {
        printf("Usage: %s <fontfile> [stop char]\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        printf("Error: %s.\n", strerror(errno));
        return 2;
    }

    char c = NULL;
    if (argc > 2)
        c = argv[2][0];

    check_header(fd);
    check_glyphs(fd, c);

    close(fd);

    return 0;

}
