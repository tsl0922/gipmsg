#ifndef __DEBUG_H__
#define __DEBUG_H__

#define DEBUG_FFL __FILE__, __LINE__, __FUNCTION__

struct tm* get_currenttime();
void debug_info(const char *file, int line, const char *function, const char* format , ...);
void debug_error(const char *file, int line, const char *function, const char* format , ...);

#ifdef DEBUG
  #define DEBUG_INFO(format, args...) \
	debug_info(DEBUG_FFL, format, ##args)
  #define DEBUG_ERROR(format, args...) \
	debug_error(DEBUG_FFL, format, ##args)
#else
  #define DEBUG_INFO(format, args...)
  #define DEBUG_ERROR(format, args...)
#endif

#endif