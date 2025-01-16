/**
 * @file my_memory.c
 * @brief Allocateur mémoire avec traçage intégré
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <execinfo.h>
#include <unistd.h>
#include <sys/wait.h>

void	check_memory_leaks(void);

/* Désactive les macros de remplacement pour ce fichier */
#undef malloc
#undef free

/* Codes de couleur ANSI pour le formatage de la sortie */
const char	*reset = "\x1b[0m";
const char	*red = "\x1b[38;5;99m";
const char	*bright_red = "\x1b[38;5;196m";
const char	*pink = "\x1b[38;5;182m";
const char	*white = "\x1b[97m";
const char	*green = "\x1b[38;5;50m";
const char	*blue = "\x1b[38;5;214m";
const char	*gray = "\x1b[38;5;238m";

/* Structure pour le suivi des allocations mémoire */
typedef struct s_allocation
{
	void					*address;
	size_t					size;
	int						id;
	struct s_allocation		*next;
}	t_allocation;

/* Variables globales pour le suivi de l'état */
static int				alloc_id_counter = 0;
static t_allocation		*allocations = NULL;
static pthread_mutex_t	mutex = PTHREAD_MUTEX_INITIALIZER;
static size_t			total_allocated = 0;
static size_t			active_allocations = 0;
static char				*g_program_path = NULL;

__attribute__((constructor))
static void	init(void)
{
	extern char	*__progname;
	char		path[512];

	snprintf(path, sizeof(path), "./%s", __progname);
	g_program_path = strdup(path);
	atexit(check_memory_leaks);
}

/**
 * @brief Récupère le nom de la fonction depuis une adresse en utilisant addr2line
 * @param addr_str L'adresse en format hexadécimal (comme retourné par backtrace)
 * @return Le nom de la fonction ou NULL en cas d'erreur
 */
static char	*get_function_name_from_addr2line(const char *addr_str)
{
	int			pipefd[2];
	pid_t		pid;
	static char	buffer[256];
	char		*executable_path;

	if (!g_program_path)
		return (NULL);
	executable_path = realpath(g_program_path, NULL);
	if (!executable_path)
		return (NULL);
	if (pipe(pipefd) == -1)
	{
		free(executable_path);
		return (NULL);
	}
	pid = fork();
	if (pid == -1)
	{
		free(executable_path);
		close(pipefd[0]);
		close(pipefd[1]);
		return (NULL);
	}
	if (pid == 0)
	{
		close(pipefd[0]);
		dup2(pipefd[1], STDOUT_FILENO);
#ifdef __APPLE__
		execl("/usr/bin/atos", "atos", "-o",
			executable_path, addr_str, NULL);
#else
		execl("/usr/bin/addr2line", "addr2line", "-f",
			"-e", executable_path, addr_str, NULL);
#endif
		exit(1);
	}
	free(executable_path);
	close(pipefd[1]);
	ssize_t n = read(pipefd[0], buffer, sizeof(buffer) - 1);
	close(pipefd[0]);
	if (n <= 0)
		return (NULL);
	buffer[n] = '\0';
	char *newline = strchr(buffer, '\n');
	if (newline)
		*newline = '\0';
	int status;
	waitpid(pid, &status, 0);
	return (*buffer && strcmp(buffer, "??") != 0) ? buffer : NULL;
}

/**
 * @brief Récupère le nom de la fonction appelante depuis la pile d'appels
 */
static char	*get_caller_function(void)
{
	void			*callstack[6];
	char			**symbols;
	static char		name[512];
	const char		*unknown = "unknown";

	int frames = backtrace(callstack, 6);
	if (frames < 4)
		return ((char *)unknown);
	symbols = backtrace_symbols(callstack, frames);
	if (!symbols)
		return ((char *)unknown);
	name[0] = '\0';
	for (int i = 2; i <= 4 && i < frames; i++)
	{
		char	func_name[256] = {0};
		char	*p = symbols[i];

		p = strrchr(p, '(');
		if (p)
		{
			p++;
			char *plus = strchr(p, '+');
			if (plus)
			{
				strncpy(func_name, p, plus - p);
				func_name[plus - p] = '\0';
			}
		}
		if (!func_name[0] && strstr(symbols[i], "(+0x"))
		{
			char *offset_start = strstr(symbols[i], "(+0x");
			if (offset_start)
			{
				offset_start += 2;
				char *offset_end = strchr(offset_start, ')');
				if (offset_end)
				{
					char offset[32] = {0};
					strncpy(offset, offset_start, offset_end - offset_start);
					char *static_name = get_function_name_from_addr2line(offset);
					if (static_name)
						strncpy(func_name, static_name, sizeof(func_name) - 1);
				}
			}
		}
		if (!func_name[0] && symbols[i])
		{
			char *last_slash = strrchr(symbols[i], '/');
			char *open_paren = strchr(symbols[i], '(');
			if (last_slash && open_paren)
			{
				last_slash++;
				strncpy(func_name, last_slash, open_paren - last_slash);
				func_name[open_paren - last_slash] = '\0';
			}
		}
		if (func_name[0])
		{
			if (name[0] != '\0')
			{
				strcat(name, gray);
				strcat(name, " ← ");
				strcat(name, gray);
			}
			else
				strcat(name, blue);
			strcat(name, func_name);
		}
	}
	strcat(name, reset);
	free(symbols);
	return (*name ? name : (char *)unknown);
}

/**
 * @brief Ajoute une nouvelle allocation à la liste chaînée
 */
static bool	add_allocation(void *address, size_t size)
{
	t_allocation	*new_alloc;

	if (!address)
		return (false);
	new_alloc = malloc(sizeof(t_allocation));
	if (!new_alloc)
		return (false);
	new_alloc->address = address;
	new_alloc->size = size;
	new_alloc->id = ++alloc_id_counter;
	new_alloc->next = allocations;
	allocations = new_alloc;
	total_allocated += size;
	active_allocations++;
	return (true);
}

/**
 * @brief Alloue de la mémoire avec traçage
 */
void	*my_malloc(size_t size)
{
	void	*ptr;

	ptr = malloc(size);
	if (!ptr)
		return (NULL);
	pthread_mutex_lock(&mutex);
	if (!add_allocation(ptr, size))
	{
		pthread_mutex_unlock(&mutex);
		free(ptr);
		return (NULL);
	}
	printf("%s------------------------------------------------------------------------------------------------%s\n",
		gray, reset);
	fprintf(stderr, "%s├ malloc %s ⤑  %s%s%s\n",
		red, white, blue, get_caller_function(), reset);
	fprintf(stderr,
		"%s╰ %sid : %3d%s %s│%s Active allocation : %s%3zu%s %s│%s malloc size : %s%4zu%s %s│%s Total memory : %s%4zu%s %s│%s %p%s\n",
		red, pink, alloc_id_counter, white,
		gray, white, blue, active_allocations, white,
		gray, white, red, size, white,
		gray, white, red, total_allocated, white,
		gray, white, ptr, reset);
	pthread_mutex_unlock(&mutex);
	return (ptr);
}

/**
 * @brief Retire une allocation de la liste chaînée
 */
static t_allocation	*remove_allocation(void *address)
{
	t_allocation	*prev;
	t_allocation	*current;

	if (!address)
		return (NULL);
	prev = NULL;
	current = allocations;
	while (current)
	{
		if (current->address == address)
		{
			if (prev)
				prev->next = current->next;
			else
				allocations = current->next;
			total_allocated -= current->size;
			active_allocations--;
			return (current);
		}
		prev = current;
		current = current->next;
	}
	return (NULL);
}

/**
 * @brief Libère la mémoire avec traçage
 */
void	my_free(void *ptr)
{
	t_allocation	*alloc;

	if (!ptr)
		return ;
	pthread_mutex_lock(&mutex);
	alloc = remove_allocation(ptr);
	if (alloc)
	{
		printf("%s------------------------------------------------------------------------------------------------%s\n",
			gray, reset);
		fprintf(stderr, "%s├ free %s   ⤑  %s%s%s\n",
			green, white, blue, get_caller_function(), reset);
		fprintf(stderr,
			"%s╰ %sid : %3d%s %s│%s Active allocation : %s%3zu%s %s│%s freed size  : %s%4zu%s %s│%s Total memory : %s%4zu%s %s│%s %p%s\n",
			green, pink, alloc->id, white,
			gray, white, blue, active_allocations, white,
			gray, white, green, alloc->size, white,
			gray, white, red, total_allocated, white,
			gray, white, ptr, reset);
		free(alloc);
	}
	else
	{
		fprintf(stderr, "%s---------------------------------------------------------------\n",
			red);
		fprintf(stderr, "%sError :%s Invalid or double free detected : %p\n",
			red, reset, ptr);
		fprintf(stderr, "%s---------------------------------------------------------------%s\n",
			red, reset);
	}
	pthread_mutex_unlock(&mutex);
	free(ptr);
}

/**
 * @brief Affiche les fuites de mémoire
 */
void	check_memory_leaks(void)
{
	t_allocation	*current;
	size_t		leak_count;
	size_t		total_leak_size;

	if (!allocations)
	{
		fprintf(stderr, "\n%sNo memory leaks detected%s\n", green, reset);
		return ;
	}
	current = allocations;
	leak_count = 0;
	total_leak_size = 0;
	while (current)
	{
		leak_count++;
		total_leak_size += current->size;
		current = current->next;
	}
	printf("%s------------------------------------------------------------------------------------------------%s\n",
		gray, reset);
	fprintf(stderr, "\n%sMemory leaks detected:%s\n", bright_red, reset);
	fprintf(stderr, "Total leaks: %zu\n", leak_count);
	fprintf(stderr, "Total memory leaked: %zu bytes\n\n", total_leak_size);
	current = allocations;
	while (current)
	{
		fprintf(stderr, "Leak at [%p] - %zu bytes (id: %d)\n",
			current->address, current->size, current->id);
		printf("%s------------------------------------------------------------------------------------------------%s\n",
			gray, reset);
		current = current->next;
	}
}