#include "../include/cub3d.h"

/*
** Nettoie les ressources après le parsing
** Retourne ERROR
*/
int	cleanup_parsing(int fd)
{
	close(fd);
	get_next_line(-1);
	return (ERROR);
}

/*
** Libère la mémoire des chemins de texture
** Libère chaque pointeur et réinitialise les couleurs
** Ne retourne rien (void)
*/
void	free_textures(t_data *data)
{
	if (data->textures.north)
		free(data->textures.north);
	if (data->textures.south)
		free(data->textures.south);
	if (data->textures.west)
		free(data->textures.west);
	if (data->textures.east)
		free(data->textures.east);
	data->textures.north = NULL;
	data->textures.south = NULL;
	data->textures.west = NULL;
	data->textures.east = NULL;
	data->textures.floor = (t_rgb){-1, -1, -1};
	data->textures.ceiling = (t_rgb){-1, -1, -1};
}

void	ft_bzero(void *s, size_t n)
{
	size_t	i;

	i = 0;
	while (i < n)
	{
		((unsigned char *)s)[i] = 0;
		i++;
	}
}

/*
** Libère la mémoire allouée pour la grille de la map
** Libère chaque ligne puis le tableau principal
** Ne retourne rien (void)
*/
void	free_map(t_map *map)
{
	int	i;

	if (!map)
		return ;
	if (!map->grid)
		return ;
	i = 0;
	while (i < map->height)
	{
		if (map->grid[i])
			free(map->grid[i]);
		i++;
	}
	free(map->grid);
	map->grid = NULL;
}

/*
** Libère toute la mémoire allouée par le programme
** Libère la map, les textures et ferme les descripteurs
** Ne retourne rien (void)
*/
void	clean_all(t_data *data, int fd)
{
	if (data)
	{
		free_map(&data->map);
		free_textures(data);
	}
	if (fd >= 0)
	{
		close(fd);
		get_next_line(-1);
	}
}
