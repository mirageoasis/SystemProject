#include "mycd.h"

int my_cd(char **argv)
{
    // fprintf(stdout, "here is my_cd function\n");

    if (argv[1] == NULL || strcmp(argv[1], "~") == 0 || strcmp(argv[1], "~/") == 0) // cd 만 입력이 올 경우
    {
        // fprintf(stdout, "only cd is input!\n");
        return chdir(getenv("HOME"));
    }

    //  for (int i = 0; argv[i] != NULL; i++)
    //{
    //      char *argument = argv[i];
    //  fprintf(stdout, "argument 순서대로 %s\n", argument);
    // }

    return chdir(argv[1]);
}