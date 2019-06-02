#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        printf("Usage : back_exec <count>");
        return -1;
    }
    int times = atoi(argv[1]);

    for(int i = 1; i <= times; i++)
    {
        sleep(1);
        printf("Sleep %d\n", i);
    }
    return 0;
}