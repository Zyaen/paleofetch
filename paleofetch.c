#pragma GCC diagnostic ignored "-Wunused-function"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>

#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>

#include <pci/pci.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "paleofetch.h"
#include "config.h"

#define BUF_SIZE 128
#define COUNT(x) (int)(sizeof x / sizeof *x)

#define halt_and_catch_fire(fmt, ...) \
	do { \
		if(status != 0) { \
			fprintf(stderr, "paleofetch: " fmt "\n", ##__VA_ARGS__); \
			exit(status); \
		} \
	} while(0)

struct conf {
	char *label, *(*function)(void*);
	bool cached;
	void *args;
} config[] = CONFIG;

struct {
	char *substring;
	char *repl_str;
	size_t length;
	size_t repl_len;
} cpu_config[] = CPU_CONFIG, gpu_config[] = GPU_CONFIG;

Display *display;
struct statvfs file_stats;
struct utsname uname_info;
struct sysinfo my_sysinfo;
int title_length, status;

/*
 * Replaces the first newline character with null terminator
 */
void remove_newline(char *s) {
	while (*s != '\0' && *s != '\n')
		s++;
	*s = '\0';
}

/*
 * Replaces the first newline character with null terminator
 * and returns the length of the string
 */
int remove_newline_get_length(char *s) {
	int i;
	for (i = 0; *s != '\0' && *s != '\n'; s++, i++)
		;
	*s = '\0';
	return i;
}

/*
 * Cleans up repeated spaces in a string
 * Trim spaces at the front of a string
 */
void truncate_spaces(char *str) {
	int src = 0, dst = 0;
	while (*(str + dst) == ' ')
		dst++;

	while (*(str + dst) != '\0') {
		*(str + src) = *(str + dst);
		if (*(str + (dst++)) == ' ')
			while (*(str + dst) == ' ')
				dst++;

		src++;
	}

	*(str + src) = '\0';
}

/*
 * Removes the first len characters of substring from str
 * Assumes that strlen(substring) >= len
 * Returns index where substring was found, or -1 if substring isn't found
 */
void remove_substring(char *str, const char *substring, size_t len) {
	/* shift over the rest of the string to remove substring */
	char *sub = strstr(str, substring);
	if (sub == NULL)
		return;

	int i = 0;
	do
		*(sub + i) = *(sub + i + len);
	while (*(sub + (++i)) != '\0');
}

/*
 * Replaces the first sub_len characters of sub_str from str
 * with the first repl_len characters of repl_str
 */
void replace_substring(char *str, const char *sub_str, const char *repl_str, size_t sub_len, size_t repl_len) {
	char buffer[BUF_SIZE / 2];
	char *start = strstr(str, sub_str);
	if (start == NULL)
		return; // substring not found

	/* check if we have enough space for new substring */
	if (strlen(str) - sub_len + repl_len >= BUF_SIZE / 2) {
		status = -1;
		halt_and_catch_fire("new substring too long to replace");
	}

	strcpy(buffer, start + sub_len);
	strncpy(start, repl_str, repl_len);
	strcpy(start + repl_len, buffer);
}

#include "functions.c" //Some people might think that including a C function is evil, and to those people I say I don't want to rewrite the makefile and also who decided that anyway that's a load of hogwash because organization trumps blindly following a guideline

char *get_cache_file_name() {
	char *cache_file = malloc(BUF_SIZE);
#ifdef TEMP_CACHE_FILE
	strcpy(cache_file, "/tmp/paleofetch-cache-file");
	return cache_file;
#endif
	char *env = getenv("XDG_CACHE_HOME");
	if (env == NULL)
		snprintf(cache_file, BUF_SIZE, "%s/.cache/paleofetch", getenv("HOME"));
	else
		snprintf(cache_file, BUF_SIZE, "%s/paleofetch", env);
	return cache_file;
}

/* This isn't especially robust, but as long as we're the only one writing
 * to our cache file, the format is simple, effective, and fast. One way
 * we might get in trouble would be if the user decided not to have any
 * sort of sigil (like ':') after their labels. */
#define CACHE_SEPERATOR '\n'
char *search_cache(char *cache_data, char *label) {
	char *start = strstr(cache_data, label);
	if (start == NULL) {
		status = ENODATA;
		halt_and_catch_fire("cache miss on key '%s'; need to --recache?", label);
	}
	start += strlen(label);
	char *end = strchr(start, CACHE_SEPERATOR);
	char *buf = calloc(1, BUF_SIZE);
	// skip past the '=' and stop just before the ';'
	strncpy(buf, start + 1, end - start - 1);

	return buf;
}

char *get_value(struct conf c, int read_cache, char *cache_data) {
	char *value;

	// If the user's config specifies that this value should be cached
	if (c.cached && read_cache)                  // and we have a cache to read from
		value = search_cache(cache_data, c.label); // grab it from the cache
	else {
		// Otherwise, call the associated function to get the value
		value = c.function(c.args);
		if (c.cached) { // and append it to our cache data if appropriate
			char *buf = malloc(BUF_SIZE);
			sprintf(buf, "%s=%s%c", c.label, value, CACHE_SEPERATOR);
			strcat(cache_data, buf);
			free(buf);
		}
	}

	return value;
}

int main(int argc, char *argv[]) {
	char *cache, *cache_data = NULL;
	FILE *cache_file;
	int read_cache = 1;

	status = uname(&uname_info);
	halt_and_catch_fire("uname failed");
	status = sysinfo(&my_sysinfo);
	halt_and_catch_fire("sysinfo failed");
	display = XOpenDisplay(NULL);

	if (argc == 2 && strcmp(argv[1], "--recache") == 0)
		if (!strcmp(argv[1], "-r") || !strcmp(argv[1], "--recache"))
			read_cache = 0;

	cache = get_cache_file_name();
	if (read_cache) {
		cache_file = fopen(cache, "r");
		read_cache = cache_file != NULL;
	}

	if (!read_cache)
		cache_data = calloc(4, BUF_SIZE * 4); // this is a monumentally stupid way to do this
	else {
		size_t len; /* unused */
		getdelim(&cache_data, &len, 0, cache_file);
		fclose(cache_file); // We just need the first (and only) line.
	}

	int offset = 0;

	for (int i = 0; i < COUNT(LOGO); i++) {
		// If we've run out of information to show...
		if (i >= COUNT(config) - offset) // just print the next line of the logo
			printf(COLOR "%s\n", LOGO[i]);
		else {
			// Otherwise, we've got a bit of work to do.
			char *label = config[i + offset].label, *value = get_value(config[i + offset], read_cache, cache_data);
			if (strcmp(value, "") != 0) {                           // check if value is an empty string
				printf(COLOR "%s%s\e[0m%s\n", LOGO[i], label, value); // just print if not empty
			} else {
				if (strcmp(label, "") != 0) {       // check if label is empty, otherwise it's a spacer
					++offset;                         // print next line of information
					free(value);                      // free memory allocated for empty value
					label = config[i + offset].label; // read new label and value
					value = get_value(config[i + offset], read_cache, cache_data);
				}
				printf(COLOR "%s%s\e[0m%s\n", LOGO[i], label, value);
			}
			free(value);
		}
	}
	puts("\e[0m");

	/* Write out our cache data (if we have any). */
	if (!read_cache && *cache_data) {
		cache_file = fopen(cache, "w");
		fprintf(cache_file, "%s", cache_data);
		fclose(cache_file);
	}

	free(cache);
	free(cache_data);
	if (display != NULL) {
		XCloseDisplay(display);
	}

	return 0;
}
