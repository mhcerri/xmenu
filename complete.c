#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "complete.h"


void free_list(struct list *list)
{
	if (list == NULL)
		return;
	struct node *it = list->head;
	while (it) {
		struct node *next = it->next;
		free(it->name);
		free(it);
		it = next;
	}
	free(list);
}

static const char *replace_cmd(const char *pattern, const char *placeholder,
		const char *str)
{
	/* TODO improve */
	static char cmd[1024];
	size_t len = 0, placeholder_len = strlen(placeholder);
	const char *tok = pattern;

	cmd[0] = '\0';
	while (tok && *tok) {
		const char *end = strstr(tok, placeholder);
		if (end == NULL) {
			snprintf(cmd + len, sizeof(cmd) - len, "%s", tok);
			break;
		}
		len += snprintf(cmd + len, sizeof(cmd) - len, "%.*s%s",
				(int) (end - tok), tok, str);
		tok = end + placeholder_len;
	}
	return cmd;
}


struct list *complete(const char *cmd, const char *input)
{
	/* resouces that must be released */
	char *line = NULL;
	struct list *list = NULL;
	FILE *fh = NULL;

	int fds[2];
	if (pipe(fds))
		return NULL;

	pid_t pid = fork();
	if (pid < -1)
		goto error;

	/* child */
	if (pid == 0) {
		dup2(fds[1], 1);
		close(fds[0]);
		execl("/bin/sh", "sh", "-c", replace_cmd(cmd, "%s", input),
					NULL);
		exit(1);
	}

	/* parent */
	close(fds[1]);

	list = malloc(sizeof(struct list));
	if (list == NULL)
		goto error;
	list->head = list->tail = list->cur = NULL;

	fh = fdopen(fds[0], "r");
	if (fh == NULL)
		goto error;

	ssize_t len;
	size_t size = 0;
	while ((len = getline(&line, &size, fh)) > 0) {
		struct node *node = malloc(sizeof(struct node));
		if (node == NULL)
			goto error;
		if (isspace(line[len - 1]))
			line[len - 1] = 0;
		node->name = strdup(line);
		node->next = NULL;
		if (list->head == NULL) {
			list->head = list->tail = node;
		} else {
			list->tail->next = node;
			list->tail = node;
		}
	}

	int status = 0;
	waitpid(pid, &status, 0);
	if (WEXITSTATUS(status)) {
		free_list(list);
		list = NULL;
	}

exit:
	if (fh)
		fclose(fh);
	close(fds[0]);
	free(line);
	return list;
error:
	printf("Error\n");
	free_list(list);
	list = NULL;
	goto exit;
}


