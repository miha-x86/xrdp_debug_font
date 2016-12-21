#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <tesseract/capi.h>
#include <leptonica/allheaders.h>
#include <errno.h>

#define FONT_DATASIZE(_w, _h) (((_h * ((_w + 7) / 8)) + 3) & ~3)

typedef enum { false, true } bool;
typedef int               int32;
typedef unsigned int      uint32;
typedef unsigned short    ushort;
typedef unsigned char     byte;

typedef struct __attribute__((__packed__)) {
    short width;
    short height;
    short baseline;
    short offset;
    short incby;
    /* 6 bytes of padding here */
    byte* data;
    int   size;
} xrdp_glyph;

typedef struct __attribute__((__packed__)) {
    char        header[4];
    char        name[32];
    short       size;
    short       style;
    /* 8 bytes of padding here */
    xrdp_glyph* glyphs;
} xrdp_font;

#define READ_BYTES(fd, buffer, size) {          \
    int r;                                      \
    r = fdread(fd, buffer, size);               \
    if (r == 0)                                 \
        return 1;                               \
    if (r == -1) {                              \
        printf("Error: %s:%i %s\n", __FILE__, __LINE__, strerror(errno)); \
        return 1;                                                       \
    }                                                                   \
    }

int
fdread(int   fd,
       void* buffer,
       int   size)
{
    int r;
    int bytes_read;

    r = read(fd, buffer, size);
    bytes_read = r;
    while (r > 0 && bytes_read < size) {
        r = read(fd, buffer + bytes_read, size);
        bytes_read += r;
    }
    return r;
}

static void
unload_font(xrdp_font* font)
{
    int i;

    for (i = 0; i < 0x4e00 - 32; i++)
        free(font->glyphs[i].data);
    free(font->glyphs);
    free(font);
}

static int
load_glyphs(int        fd,
            xrdp_font* font)
{
    int         i, r;
    xrdp_glyph* glyph;
    byte        dummy[10];

    for (i = 0; i < 0x4e00 - 32; i++) {
        glyph = font->glyphs + i;

        /* Glyph info */
        READ_BYTES(fd, glyph, sizeof(xrdp_glyph) - sizeof(byte*) - sizeof(int));

        /* Padding */
        READ_BYTES(fd, dummy, 6);

        /* Data */
        glyph->size = FONT_DATASIZE(glyph->width, glyph->height);
        if (!glyph->size)
            continue;

        glyph->data = malloc(glyph->size);
        READ_BYTES(fd, glyph->data, glyph->size);
    }
    return 0;
}

static xrdp_font*
load_font(const char* filename)
{
    xrdp_font* font;
    int        fd;
    byte       dummy[10];

    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        printf("Error: %s.\n", strerror(errno));
        return NULL;
    }

    font = calloc(1, sizeof(xrdp_font));
    if (!font)
        return NULL;

    /* Alloc glyph space */
    font->glyphs = calloc(1, sizeof(xrdp_glyph) * (0x4e00 - 32));
    if (!font->glyphs) {
        unload_font(font);
        return NULL;
    }

    /* Font info */
    fdread(fd, font, sizeof(xrdp_font) - sizeof(xrdp_glyph*));

    /* Padding */
    fdread(fd, dummy, 8);

    if (load_glyphs(fd, font)) {
        unload_font(font);
        return NULL;
    }

    return font;
}


static void
print_glyph(xrdp_glyph* glyph)
{
    int  w, h, x, y, i;
    byte b, *data;

    w = (glyph->width + 7) & ~7;
    h = (glyph->height + 3) & ~3;
    data = glyph->data;

    for (y = 0; y < h; y++) {
        b = *data++;
        for (x = 0; x < w;) {
            for (i = 0; i < 8 && x < w; i++, x++) {
                if (b & 0x80)
                    putchar('1');
                else
                    putchar('0');
                b <<= 1;
            }
        }
        putchar('\n');
    }
}

int
main(int    argc,
     char** argv)
{
    xrdp_font* font;

    if (argc < 2) {
        printf("Usage: %s <fontfile> [stop char]\n", argv[0]);
        return 1;
    }

    printf("Loading font...\n");
    font = load_font(argv[1]);
    if (!font)
        return 1;
    printf("OK\n");

    unload_font(font);

    return 0;

}
