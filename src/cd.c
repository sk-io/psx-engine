#include "cd.h"

#include <sys/types.h>

#include <libcd.h>
#include <malloc.h>
#include <stdio.h>

u_char *cd_read_file(char *path, u_long *size) {
    u_char* ret;
    CdlFILE file;
    int len;

    if (CdSearchFile(&file, path)) {
        // to be safe, readfile reads in 2048 byte chunks
        len = (file.size / 2048 + 1) * 2048;
        printf("reading %s with size %u\n", path, len);

        ret = (u_char*) malloc3(len);
        printf("malloc at %p\n", ret);
        if (ret == NULL) {
            printf("cd_read_file failed to allocate memory!\n");
        }
        
        CdReadFile(path, (u_long*) ret, len);
        CdReadSync(0, 0);

        if (size != NULL)
            *size = file.size;
        
        return ret;
    } else {
        printf("couldnt find file %s\n", path);
    }

    if (size != NULL)
        *size = 0;
    return NULL;
}
