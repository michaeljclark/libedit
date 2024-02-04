#include <stdio.h>
#include <string.h>

ssize_t getline(char **lineptr, size_t *n, FILE *stream)
{       
   size_t lim = *n;
   char *s = *lineptr;
   int c, i = 0;

   /* todo if lineptr is null then malloc/realloc and update n */

   while (--lim > 0 && (c=fgetc(stream)) != EOF && c != '\n')           
      s[i++] = c;       

   if (c == '\n')           
      s[i++] = c;       

   s[i] = '\0';       

   return i;   
}
