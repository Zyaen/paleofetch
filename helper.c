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

void safe_pipe(int pipefd[2]) {
	int err = pipe(pipefd);
	if (err)
		halt_and_catch_fire("Could not create a pipe");
}

void *safe_malloc(size_t bytes) {
	void *retval = malloc(bytes);
	if (!retval)
		halt_and_catch_fire("Failed to allocate %lu bytes", (unsigned long) bytes);
	return retval;
}
void *safe_calloc(size_t memb_size, size_t bytes) {
	void *retval = calloc(memb_size, bytes);
	if (!retval)
		halt_and_catch_fire("Failed to allocate %lu %lu-byte words", (unsigned long) bytes, (unsigned long) memb_size);
	return retval;
}
