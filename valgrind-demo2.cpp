#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    char *p = (char *)malloc(1);
    *p = 'a';

    char c = *p;

    printf("\n [%c]\n",c);

    free(p);
    c = *p;
    return 0;
}
