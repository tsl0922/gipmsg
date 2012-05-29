#include <common.h>

/*
 * try to convert the given string's encode to utf-8 use the given codeset(seprated by ",")
 * @param string
 * @param len
 * @param codeset
 * @param encode (out)
 */
char *string_validate(const char *string, size_t len, const char *codeset, char **encode)
{
	const char *pptr, *ptr;
	char *tstring, *cset;
	bool is_ascii;

	if(!string || !codeset || !encode) return;
	
	tstring = NULL;
	*encode = NULL;
	if(!is_utf8(string, &is_ascii) && codeset) {
		cset = NULL;
		ptr = codeset;
		do {
				if (*(pptr = ptr + strspn(ptr, ",;\x20\t")) == '\0')
					break;
				if (!(ptr = strpbrk(pptr, ",;\x20\t")))
					ptr = pptr + strlen(pptr);
					g_free(cset);
					cset = g_strndup(pptr, ptr - pptr);
				if (strcasecmp(cset, "utf-8") == 0)
					continue;
		} while (!(tstring = convert_encode("utf-8", cset, (char *)string, len)));
		if(tstring) *encode = cset;
	}

	if(!*encode) *encode = strdup("utf-8");
	if(!tstring) tstring = strdup(string);
	
	return tstring;
}

/*
 * convert string from one encode to another
 * @param toencode
 * @param fromencode
 * @param string
 * @param len
 */
char *convert_encode(char *toencode, char *fromencode, char *string, size_t len)
{
#define OUTBUG_SIZE (3*MAX_UDPBUF)
	char *inbuf = string;
	size_t inlen = len;
	iconv_t cd;
	char *outbuf;
	char *outptr;
	size_t outlen;
	size_t n;

	if(!strcasecmp(toencode, fromencode))
		return NULL;
	outlen = OUTBUG_SIZE;
	outbuf = (char *)malloc(outlen);
	cd = iconv_open(toencode, fromencode);
	if(cd == (iconv_t)-1) {
		DEBUG_ERROR("iconv_open fail!\n");
		return NULL;
	}
	outptr = outbuf;
	n = iconv(cd, &inbuf, &inlen, &outptr, &outlen);
	if(n == (size_t)-1) {
		DEBUG_ERROR("iconv fail!\n");
		return NULL;
	}
	iconv_close(cd);

	if(outptr != outbuf) {
		*outptr = '\0';
	}

	return outbuf;
}

/*
 * check whether the given string's encode is utf-8
 * @param _s
 * @param is_asscii (out)
 */
bool is_utf8(const char *_s, bool *is_ascii)
{
	const unsigned char *s = (const unsigned char *)_s;
	bool		tmp;

	if (!is_ascii) is_ascii = &tmp;

	*is_ascii = true;

	while (*s) {
		if (*s <= 0x7f) {
		}
		else {
			*is_ascii = false;
			if (*s <= 0xdf) {
				if ((*++s & 0xc0) != 0x80) return false;
			}
			else if (*s <= 0xef) {
				if ((*++s & 0xc0) != 0x80) return false;
				if ((*++s & 0xc0) != 0x80) return false;
			}
			else if (*s <= 0xf7) {
				if ((*++s & 0xc0) != 0x80) return false;
				if ((*++s & 0xc0) != 0x80) return false;
				if ((*++s & 0xc0) != 0x80) return false;
			}
			else if (*s <= 0xfb) {
				if ((*++s & 0xc0) != 0x80) return false;
				if ((*++s & 0xc0) != 0x80) return false;
				if ((*++s & 0xc0) != 0x80) return false;
				if ((*++s & 0xc0) != 0x80) return false;
			}
			else if (*s <= 0xfd) {
				if ((*++s & 0xc0) != 0x80) return false;
				if ((*++s & 0xc0) != 0x80) return false;
				if ((*++s & 0xc0) != 0x80) return false;
				if ((*++s & 0xc0) != 0x80) return false;
				if ((*++s & 0xc0) != 0x80) return false;
			}
		}
		s++;
	}
	return	true;
}
