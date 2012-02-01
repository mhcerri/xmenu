#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include "complete.h"

const char *words[] = {
	"apple",
	"banana",
	"blueberry",
	"melon",
	"orange",
	"pineapple",
	"strawberry",
	"watermelon",
	NULL
};

int complete(char *buffer, size_t size)
{
	int len, i, j;
	const char sep = ':';
	char *var;
	const char *path;

	path = getenv("PATH");
	if (path == NULL)
		return 1;

	len = strlen(path);
	var = malloc(len + 1);
	if (var == NULL)
		return 1;

	while (1) {
		j = 0;
		for (; path[i]; i++, j++) {
			if (path[i] == sep) {
				break;
			} else {
				var[j] = path[i];
			}
		}
		var[j] = 0;

		if (complete_dir(buffer, size, var) == 0) {
			free(var);
			return 0;
		}

		if (path[i] == 0)
			break;
		i++;
	}
	free(var);
	return 1;
}

int complete_dir(char *buffer, size_t size, const char *dirpath)
{
	size_t len;
	struct dirent *entry;

	DIR *dir = opendir(dirpath);
	if (dir == NULL)
		return 1;

	len = strlen(buffer);
	while (entry = readdir(dir)) {
		if (strncmp(buffer, entry->d_name, len) == 0) {
			strncpy(buffer + len, entry->d_name + len, size - len);
			closedir(dir);
			return 0;
		}
	}
	closedir(dir);
	return 1;
}

