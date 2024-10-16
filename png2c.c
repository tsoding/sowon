#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define STB_IMAGE_IMPLEMENTATION
#include "./stb_image.h"

const char *shift(int *argc, char ***argv)
{
    assert(*argc > 0);
    const char *result = *argv[0];
    *argc -= 1;
    *argv += 1;
    return result;
}

int main(int argc, char *argv[])
{
    shift(&argc, &argv);        // skip program name

    if (argc <= 1) {
        fprintf(stderr, "Usage: png2c <filepath.png> <name>\n");
        fprintf(stderr, "ERROR: expected file path and name\n");
        exit(1);
    }

    const char *filepath = shift(&argc, &argv);
    const char *name = shift(&argc, &argv);

    int x, y, n;
    uint32_t *data = (uint32_t *)stbi_load(filepath, &x, &y, &n, 4);

    if (data == NULL) {
        fprintf(stderr, "Could not load file `%s`\n", filepath);
        exit(1);
    }

    printf("#ifndef PNG_%s_H_\n", name);
    printf("#define PNG_%s_H_\n", name);
    printf("size_t %s_width = %d;\n", name, x);
    printf("size_t %s_height = %d;\n", name, y);
    printf("uint32_t %s_data[] = {", name);
    for (size_t i = 0; i < (size_t)(x * y); ++i) {
        printf("0x%x, ", data[i]);
    }
    printf("};\n");
    printf("#endif // PNG_%s_H_\n", name);

    return 0;
}
