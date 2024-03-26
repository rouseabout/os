#include <stdlib.h>
#include <stdio.h>

int main()
{
    for (unsigned int i = 0; environ[i]; i++)
        printf("%s\n", environ[i]);

    return 0;
}
