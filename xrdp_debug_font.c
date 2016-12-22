/**
 *   \file xrdp_debug_font.c
 *   \brief Debug xrdp fv1 fonts
 *
 *  Simple tool for debugging xrdp fv1 fonts.
 *
 */

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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

#define READ_BYTES(fd, buffer, size) {                                  \
    int r;                                                              \
    r = fdread(fd, buffer, size);                                       \
    if (r == 0)                                                         \
        return 1;                                                       \
    if (r == -1) {                                                      \
        printf("Error: %s:%i %s\n", __FILE__, __LINE__, strerror(errno)); \
        return 1;                                                       \
    }                                                                   \
    }

#define READ_BYTES2(fd, buffer, size) {                                 \
        int r;                                                          \
        r = fdread(fd, buffer, size);                                   \
        if (r == 0)                                                     \
            return NULL;                                                \
        if (r == -1) {                                                  \
            printf("Error: %s:%i %s\n", __FILE__, __LINE__, strerror(errno)); \
            return NULL;                                                \
        }                                                               \
    }

/**
 * @brief fdread
 *
 * Generic reading routine
 *
 * @param fd fd
 * @param buffer buffer
 * @param size size
 * @return int
 */
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

/**
 * @brief unload_font
 *
 * @param font font
 */
static void
unload_font(xrdp_font* font)
{
    int i;

    for (i = 0; i < 0x4e00 - 32; i++)
        free(font->glyphs[i].data);
    free(font->glyphs);
    free(font);
}

/**
 * @brief load_glyphs
 *
 * Loads glyph information from fv1 font file
 *
 * @param fd fd
 * @param font font
 * @return int
 */
static int
load_glyphs(int        fd,
            xrdp_font* font)
{
    int         i;
    xrdp_glyph* glyph;
    byte        padding[6];

    for (i = 0; i < 0x4e00 - 32; i++) {
        glyph = font->glyphs + i;

        /* Glyph info */
        READ_BYTES(fd, glyph, sizeof(xrdp_glyph) - sizeof(byte*) - sizeof(int));

        if (!glyph->width || !glyph->height)
            printf("Warning: Invalid glyph detected at index %i.\n", i + 32);

        /* Padding */
        READ_BYTES(fd, padding, 6);

        /* Data */
        glyph->size = FONT_DATASIZE(glyph->width, glyph->height);
        if (!glyph->size)
            continue;

        glyph->data = malloc(glyph->size);
        READ_BYTES(fd, glyph->data, glyph->size);
    }
    return 0;
}

/**
 * @brief load_font
 *
 * Loads a xrdp fv1 file
 *
 * @param filename filename
 * @return xrdp_font
 */
static xrdp_font*
load_font(const char* filename)
{
    xrdp_font* font;
    int        fd;
    byte       padding[8];

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
    READ_BYTES2(fd, font, sizeof(xrdp_font) - sizeof(xrdp_glyph*));

    /* Padding */
    READ_BYTES2(fd, padding, 8);

    if (load_glyphs(fd, font)) {
        unload_font(font);
        return NULL;
    }

    return font;
}

/**
 * @brief print_glyph
 *
 * Prints glyph information
 *
 * @param glyph glyph
 */
static void
print_glyph(xrdp_glyph* glyph)
{
    int  w, h, x, y, i;
    byte b, *data;

    printf("Glyph information:\n"
           "width: %-3i\n"
           "height: %-3i\n"
           "baseline: %-3i\n"
           "offset: %-3i\n"
           "incby: %-3i\n"
           "data:\n",
           glyph->width, glyph->height, glyph->baseline,
           glyph->offset, glyph->incby);

    w = (glyph->width + 7) & ~7;
    h = (glyph->height + 3) & ~3;
    data = glyph->data;
    if (!data)
        return;

    /* Print ruler */
    printf("    ");
    for (x = 0; x < w;) {
        for (i = 1; i < 9; i++, x++) {
            printf("%i", i);
        }
    }
    printf("\n    ");
    for (x = 0; x < w; x++)
        putchar('-');
    printf("\n");

    /* Print glyph */
    for (y = 1; y < h+1; y++) {
        printf("%2i: ", y);
        for (x = 0; x < w;) {
            b = *data++;
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

#if 0
/**
 * @brief print_copies
 *
 * Print any other glyph that has the exactly same data
 *
 * @param glypha glypha
 * @param glyph glyph
 */
static void
print_copies(xrdp_glyph* glypha,
             xrdp_glyph* glyph)
{
    int i;
    xrdp_glyph* g;

    for (i = 0; i < 0x4e00 - 32; i++) {
        g = glypha + i;

        if (g == glyph || g->size != glyph->size
            || g->width != glyph->width || g->height != glyph->height)
            continue;

        if (!memcmp(g->data, glyph->data, glyph->size))
            print_glyph(g);
    }
}
#endif

/**
 * @brief print_all_glyphs
 *
 * Prints all the glyphs on the glyph array
 *
 * @param glypha glypha
 */
static void
print_all_glyphs(xrdp_glyph* glypha)
{
    int i;
    xrdp_glyph* glyph;

    for (i = 0; i < 0x4e00 - 32; i++) {
        glyph = glypha + i;
        if (glyph->width <= 1)
            continue;

        printf("Index: %i\n", i + 32);
        print_glyph(glypha + i);
    }
}

/**
 * @brief main
 *
 * @param argc argc
 * @param argv argv
 * @return int
 */
int
main(int    argc,
     char** argv)
{
    xrdp_font* font;
    int        i;

    if (argc < 2) {
        printf("Usage: %s <fontfile> [codepoint]\n", argv[0]);
        return 1;
    }

    printf("Loading font...\n");
    font = load_font(argv[1]);
    if (!font) {
        printf("Error loading font file.\n");
        return 1;
    }
    printf("OK\n");

    /* Print glyph */
    if (argc > 2) {
        i = (byte) argv[2][0];
        if (i < 32 || i >= 0x4e00) {
            printf("Invalid character %i.\n", i);
            unload_font(font);
            return 1;
        }

        printf("Codepoint: %i\n", i);
        print_glyph(&font->glyphs[i - 32]);
    }
    else {
        print_all_glyphs(font->glyphs);
    }

    unload_font(font);

    return 0;
}
