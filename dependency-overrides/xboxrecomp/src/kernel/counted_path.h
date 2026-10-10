#ifndef XBOX_COUNTED_PATH_H
#define XBOX_COUNTED_PATH_H
#include <stddef.h>
#include <string.h>
/* Xbox ANSI_STRING.Length is authoritative; suffix bytes need not belong to
 * the pathname, and the source need not have a terminator. */
static int xbox_copy_counted_path(char*dest,size_t capacity,const char*source,size_t length,size_t maximum){
 if(!dest||!source||!capacity||length>=capacity||length>maximum)return 0;
 if(length&&memchr(source,0,length))return 0;
 memcpy(dest,source,length);dest[length]=0;return 1;
}
#endif
