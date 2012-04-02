#include <common.h>

void
debug_info(const char *file, int line, const char *function, const char* format , ...)
{
	char t_str[32] = { 0 };
	char fmt[4096] = { 0 };
	va_list ap;
	struct tm* t = get_currenttime();
	strftime(t_str , sizeof(t_str) , "%T" , t );
	sprintf(fmt , "[\e[32m\e[1m%s file:%s line: %d function:%s INFO\e[0m]\n%s\n" ,
		t_str , file, line, function, format);
	va_start(ap, format);
	vfprintf(stdout , fmt , ap);
	va_end(ap);
}
void
debug_error(const char *file, int line, const char *function, const char* format , ...)
{
	char t_str[32] = { 0 };
	char fmt[4096] = { 0 };
	va_list ap;
	struct tm* t = get_currenttime();
	strftime(t_str , sizeof(t_str) , "%T" , t );
	sprintf(fmt , "[\e[32m\e[1m%s file:%s line: %d function:%s ERROR\e[0m]\n%s\n" ,
		t_str , file, line, function, format);
	va_start(ap, format);
	vfprintf(stderr , fmt , ap);
	va_end(ap);
}
