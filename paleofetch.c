#pragma GCC diagnostic ignored "-Wunused-function"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>

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

 //Some people might think that including a C function is evil, and to those people I say I don't want to rewrite the makefile and also who decided that anyway that's a load of hogwash because organization trumps blindly following a guideline
#include "helper.c"
#include "functions.c"

char *get_cache_file_name() {
#ifdef TEMP_CACHE_FILE
	return safe_strdup("/tmp/paleofetch-cache-file");
#endif
	char *env = getenv("XDG_CACHE_HOME");
	if (env)
		return sallocf("%s/paleofetch", env);
	return sallocf("%s/.cache/paleofetch", getenv("HOME"));
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
	char *cached_val = safe_calloc(1, end - start);
	// skip past the '=' and stop just before the ';'
	strncpy(cached_val, start + 1, end - start - 1);

	return cached_val;
}

char *get_value(struct conf c, int read_cache, char *cache_data, FILE *cache_file) {
	char *value;

	// If the user's config specifies that this value should be cached
	if (c.cached && read_cache)                  // and we have a cache to read from
		value = search_cache(cache_data, c.label); // grab it from the cache
	else {
		// Otherwise, call the associated function to get the value
		value = c.function(c.args);
		if (c.cached) { // and append it to our cache file if appropriate
			fprintf(cache_file, "%s=%s%c", c.label, value, CACHE_SEPERATOR);
		}
	}
//	fprintf(stderr, "%s\n", value);
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

	/* Had to rewrite this entire thing due to it being too convoluted */
	if (argc == 2)
		if ((!strcmp(argv[1], "-r")) || (!strcmp(argv[1], "--recache")))
			read_cache = 0;

	cache = get_cache_file_name();
	if (read_cache) {
		cache_file = fopen(cache, "r");
		read_cache = cache_file != NULL;
	}

	//read_cache is updated so check it again
	if (read_cache) {
		size_t len; /* unused */
		getdelim(&cache_data, &len, 0, cache_file);
		fclose(cache_file); // We just need the first (and only) line.
	} else {
		//if not read_cache, we print directly to cache_file, as opposed to storing a giant string and appending data to it
		cache_file = fopen(cache, "w");
	}


	/* Had to rewrite this entire thing due to it being GARBAGE */
	int offset = 0, i = 0;
	// Offset increments because we always go through one line of commands, but we don't always print anything
	for (; i < COUNT(LOGO) && i + offset < COUNT(config);) {
		// Evaluate a command[i+offset]. If we should print it, print it and the logo line i
		char *label = config[i + offset].label;
		char *value = get_value(config[i + offset], read_cache, cache_data, cache_file);
		if (!strcmp(value, "") && strcmp(label, "")) {             // If value is empty and label is not, then it is skipped
			free(value);
			offset++;
			continue;
		}
		printf(COLOR "%s"COLOR"%s\e[0m%s\n", LOGO[i], label, value);
		free(value);
		i++;
	}
	//only one of the two following for loops is possible
	//LOGO is bigger than commands
	for (; i < COUNT(LOGO); i++)
		printf(COLOR "%s\n", LOGO[i]);
	//Commands are bigger than LOGO
	int spacer_size = strlen(LOGO[i-1]);
	for (; i + offset < COUNT(config); offset++) {
		//print spaces the length of the last line of the logo
		for (int j = 0; j < spacer_size; j++)
			fputc(' ', stdout);
		char *label = config[i + offset].label;
		char *value = get_value(config[i + offset], read_cache, cache_data, cache_file);
		printf(COLOR "%s\e[0m%s\n", label, value);
		free(value);
	}
	puts("\e[0m");

	/* Write out our cache data (if we have any). */
	if (!read_cache)
		fclose(cache_file);

	free(cache);
	free(cache_data);
	if (display != NULL) {
		XCloseDisplay(display);
	}

	return 0;
}
