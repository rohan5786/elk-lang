#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "code.h"
#include "debug.h"
#include "vm.h"

// according to this reel, internal linkage better for memory
static void repl()
{
    char line[1024];
    while (1)
    {
        printf("> ");
        if (!fgets(line, sizeof(line), stdin))
        {
            printf("\n");
            break;
        }

        interpret(line);
    }
}

// h-holy safety!!
static char *read_file(const char *path)
{
    FILE *file = fopen(path, "rb"); // read binary
    if (file == NULL) 
    {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74); // err code
    }

    // jump to end, get length, then jump back to start to read raw bytes
    if (fseek(file, 0L, SEEK_END) != 0) 
    {
        fprintf(stderr, "Could not seek to end of file \"%s\".\n", path);
        exit(74);
    }

    size_t size = ftell(file);
    if (size < 0)
    {
        fprintf(stderr, "Could not read size of file \"%s\".\n", path);
        exit(74);
    }

    rewind(file);

    char *buffer = (char *)malloc(size + 1); // + 1 is for \0
    if (buffer == NULL)
    {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }


    size_t bytes = fread(buffer, sizeof(char), size, file); // to find the index
    if (bytes < size)
    {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }
    buffer[bytes] = '\0';

    fclose(file);
    return buffer;
}

static void run_file(const char *path)
{
    char *source = read_file(path);
    Result res = interpret(source);
    free(source); // freeing ptr for mem!

    // err codes
    if (res == COMPILE_ERR)
        exit(65);
    if (res == RUNTIME_ERR)
        exit(70);
}

int main(int argc, const char *argv[])
{
    init_vm();

    // only argument is .\elk
    if (argc == 1) repl();
    else if (argc == 2) run_file(argv[1]); // only argument is after .\elk
    else {
        fprintf(stderr, "Usage: elk [path]\n");
        exit(64);
    }

    free_vm();

    return 0;
}