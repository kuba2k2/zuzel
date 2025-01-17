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

typedef struct log_thread_t {
	SDL_threadID id;
	char *name;
	struct log_thread_t *next;
} log_thread_t;

typedef struct log_error_t {
	char *message;
	unsigned int len;
	struct log_error_t *next;
	struct log_error_t *prev;
} log_error_t;

static char message_buf[1024];
static bool lt_log_append_error(const char *message);
static log_thread_t *log_threads = NULL;
static log_error_t *log_errors	 = NULL;
SDL_mutex *log_mutex			 = NULL;

#if LT_LOGGER_CALLER
void lt_log(const uint8_t level, const char *caller, const unsigned short line, const char *format, ...) {
#else
void lt_log(const uint8_t level, const char *format, ...) {
#endif

	if (log_mutex == NULL)
		log_mutex = SDL_CreateMutex();

	if (level < SETTINGS->loglevel)
		return;

#if LT_LOGGER_TIMESTAMP
	struct timeval tv;
	gettimeofday(&tv, NULL);
	time_t t	 = tv.tv_sec;
	struct tm tm = *localtime(&t);
#endif

#if LT_LOGGER_TASK
	log_thread_t *thread;
	SDL_threadID thread_id = SDL_ThreadID();
	LL_SEARCH_SCALAR(log_threads, thread, id, thread_id);
#endif

#if LT_LOGGER_COLOR
	char c_bright = '0' + (colors[level] >> 4);
	char c_value  = '0' + (colors[level] & 0x7);
#endif

	SDL_LockMutex(log_mutex);

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
#if LT_LOGGER_TASK
		"[%s] "
#endif
#if LT_LOGGER_CALLER
		"%s():%hu: "
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
#if LT_LOGGER_TASK
		,
		thread == NULL ? "main" : thread->name
#endif
#if LT_LOGGER_CALLER
		,
		caller,
		line
#endif
	);

	va_list va_args;
	va_start(va_args, format);
	vsnprintf(message_buf, sizeof(message_buf), format, va_args);
	va_end(va_args);

	if (level >= LT_LEVEL_ERROR)
		lt_log_append_error(message_buf);
	printf("%s\r\n", message_buf);
	fflush(stdout);

	SDL_UnlockMutex(log_mutex);
}

void lt_log_set_thread_name(const char *name) {
	SDL_threadID thread_id = SDL_ThreadID();
	log_thread_t *log_thread, *tmp;
	LL_FOREACH_SAFE(log_threads, log_thread, tmp) {
		if (log_thread->id != thread_id)
			continue;
		LL_DELETE(log_threads, log_thread);
		free(log_thread->name);
		free(log_thread);
	}
	if (name == NULL)
		return;
	MALLOC(log_thread, sizeof(*log_thread), return);
	log_thread->id	 = thread_id;
	log_thread->name = strdup(name);
	LL_APPEND(log_threads, log_thread);
}

static bool lt_log_append_error(const char *message) {
	log_error_t *error = malloc(sizeof(*error));
	if (error == NULL)
		return false;
	memset(error, 0, sizeof(*error));
	DL_APPEND(log_errors, error);
	error->message = strdup(message);
	error->len	   = strlen(message);
	return true;
}

char *lt_log_get_errors(int wrap) {
	// count the total length of messages
	size_t errors_len = 0;
	log_error_t *error, *tmp;
	DL_FOREACH(log_errors, error) {
		errors_len += error->len + sizeof("\n\n") - 1;
		if (wrap != 0)
			errors_len += error->len / wrap;
	}
	// allocate a string
	char *errors_str;
	MALLOC(errors_str, errors_len + 1, return NULL);
	// copy each message while deleting them
	char *write_head = errors_str;
	DL_FOREACH_SAFE(log_errors, error, tmp) {
		size_t line_len = 0;
		char *message	= error->message;
		do {
			char *space = strchr(message, ' ');
			size_t word_len;
			if (space != NULL)
				word_len = space - message + 1;
			else
				word_len = strlen(message);
			if (wrap != 0 && line_len + word_len > wrap) {
				*write_head++ = '\n';
				line_len	  = 0;
			}
			memcpy(write_head, message, word_len);
			message += word_len;
			write_head += word_len;
			line_len += word_len;
		} while (*message != '\0');

		memcpy(write_head, "\n\n", sizeof("\n\n") - 1);
		write_head += sizeof("\n\n") - 1;

		DL_DELETE(log_errors, error);
		free(error->message);
		free(error);
	}
	// trim trailing newline
	while (write_head > errors_str && *(write_head - 1) == '\n')
		*(--write_head) = '\0';
	return errors_str;
}

void lt_log_clear_errors() {
	log_error_t *error, *tmp;
	DL_FOREACH_SAFE(log_errors, error, tmp) {
		DL_DELETE(log_errors, error);
		free(error->message);
		free(error);
	}
}
