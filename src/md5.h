#ifndef WGM__MD5_H
#define WGM__MD5_H

#include <stdint.h>
#include <stdio.h>

int wgm_md5_file_hex(FILE *fp, char md5sum[32]);
int wgm_md5_hex(const char *data, size_t size, char md5sum[32]);

#endif  /* #ifndef WGM__MD5_H */
