#ifndef _CODESET_H_
#define _CODESET_H_

char *string_validate(const char *string, size_t len, const char *codeset, char **encode);
char *convert_encode(char *toencode, char *fromencode, char *string, size_t len);
bool is_utf8(const char *_s, bool *is_ascii);

#endif /*_CODESET_H_*/

