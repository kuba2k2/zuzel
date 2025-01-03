// Copyright (c) Kuba Szczodrzyński 2024-12-8.

#include "logger.h"

#define COLOR_FMT			 "\e[0;30m"
#define COLOR_BLACK			 0x00
#define COLOR_RED			 0x01
#define COLOR_GREEN			 0x02
#define COLOR_YELLOW		 0x03
#define COLOR_BLUE			 0x04
#define COLOR_MAGENTA		 0x05
#define COLOR_CYAN			 0x06
#define COLOR_WHITE			 0x07
#define COLOR_BRIGHT_BLACK	 0x10
#define COLOR_BRIGHT_RED	 0x11
#define COLOR_BRIGHT_GREEN	 0x12
#define COLOR_BRIGHT_YELLOW	 0x13
#define COLOR_BRIGHT_BLUE	 0x14
#define COLOR_BRIGHT_MAGENTA 0x15
#define COLOR_BRIGHT_CYAN	 0x16
#define COLOR_BRIGHT_WHITE	 0x17

static const char levels[] = {'V', 'D', 'I', 'W', 'E', 'F'};

#if LT_LOGGER_COLOR
static const uint8_t colors[] = {
	COLOR_BRIGHT_CYAN,
	COLOR_BRIGHT_BLUE,
	COLOR_BRIGHT_GREEN,
	COLOR_BRIGHT_YELLOW,
	COLOR_BRIGHT_RED,
	COLOR_BRIGHT_MAGENTA,
};
#endif

static char message_buf[1024];
static bool lt_log_append_error(const char *message);

#if LT_LOGGER_CALLER
void lt_log(const uint8_t level, const char *caller, const unsigned short line, const char *format, ...) {
#else
void lt_log(const uint8_t level, const char *format, ...) {
#endif
	if (level < SETTINGS->loglevel)
		return;

#if LT_LOGGER_TIMESTAMP
	struct timeval tv;
	gettimeofday(&tv, NULL);
	time_t t	 = tv.tv_sec;
	struct tm tm = *localtime(&t);
#endif

#if LT_LOGGER_TASK
#error "Not implemented"
	char task_colon = ':';
	char *task_name;
	if (!task) {
		task_name  = "";
		task_colon = '-';
	}
#endif

#if LT_LOGGER_COLOR
	char c_bright = '0' + (colors[level] >> 4);
	char c_value  = '0' + (colors[level] & 0x7);
#endif

	printf(
	// format:
#if LT_LOGGER_COLOR
		"\e[%c;3%cm"
#endif
		"%c "
#if LT_LOGGER_TIMESTAMP
		"[%04d-%02d-%02d %02d:%02d:%02d.%03ld] "
#endif
#if LT_LOGGER_COLOR
		"\e[0m"
#endif
#if LT_LOGGER_CALLER
		"%s():%hu: "
#endif
#if LT_LOGGER_TASK
		"%s%c "
#endif
		,
	// arguments:
#if LT_LOGGER_COLOR
		c_bright, // whether text is bright
		c_value,  // text color
#endif
		levels[level]
#if LT_LOGGER_TIMESTAMP
		,
		tm.tm_year + 1900,
		tm.tm_mon + 1,
		tm.tm_mday,
		tm.tm_hour,
		tm.tm_min,
		tm.tm_sec,
		tv.tv_usec / 1000
#endif
#if LT_LOGGER_CALLER
		,
		caller,
		line
#endif
#if LT_LOGGER_TASK
		,
		task_name,
		task_colon // printing outside of tasks
#endif
	);

	va_list va_args;
	va_start(va_args, format);
	vsnprintf(message_buf, sizeof(message_buf) - 1, format, va_args);
	va_end(va_args);

	if (level >= LT_LEVEL_ERROR)
		lt_log_append_error(message_buf);
	printf("%s\r\n", message_buf);
	fflush(stdout);
}

typedef struct log_error_t {
	char *message;
	unsigned int len;
	struct log_error_t *next;
	struct log_error_t *prev;
} log_error_t;

static log_error_t *errors = NULL;

static bool lt_log_append_error(const char *message) {
	log_error_t *error = malloc(sizeof(*error));
	if (error == NULL)
		return false;
	memset(error, 0, sizeof(*error));
	DL_APPEND(errors, error);
	error->message = strdup(message);
	error->len	   = strlen(message);
	return true;
}

char *lt_log_get_errors(const char *prefix) {
	// count the total length of messages
	size_t prefix_len = strlen(prefix);
	size_t errors_len = prefix_len;
	log_error_t *error, *tmp;
	DL_FOREACH(errors, error) {
		errors_len += error->len + sizeof("\n") - 1;
	}
	// allocate a string
	char *errors_str;
	MALLOC(errors_str, errors_len + 1, return NULL);
	// copy the prefix message
	memcpy(errors_str, prefix, prefix_len + 1);
	// copy each message while deleting them
	char *write_head = errors_str + prefix_len;
	DL_FOREACH_SAFE(errors, error, tmp) {
		strcpy(write_head, error->message);
		write_head += error->len;
		strcpy(write_head, "\n");
		write_head += sizeof("\n") - 1;
		DL_DELETE(errors, error);
		free(error->message);
		free(error);
	}
	if (write_head[-1] == '\n')
		write_head[-1] = '\0';
	return errors_str;
}
