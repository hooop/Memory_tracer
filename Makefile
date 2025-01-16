# Pour activer/désactiver le debug, changez juste cette ligne :
DEBUG = no

NAME = cub3D
HEADER = ./include/
CC = gcc
CFLAGS = -Werror -Wall -Wextra -g -I $(HEADER)

ifeq ($(DEBUG),yes)
    CFLAGS += -DDEBUG_MEMORY -rdynamic -fno-omit-frame-pointer -g3
endif

SRCS = 

ifeq ($(DEBUG),yes)
    SRCS += debug/my_memory.c
endif

OBJS = $(SRCS:.c=.o)

all: $(NAME)
	@clear

$(NAME): $(OBJS) $(HEADER)
	@$(CC) $(CFLAGS) $(OBJS) -o $(NAME)
	@echo "Executable $(NAME) créé avec succès !"

%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -rf $(OBJS)
	@echo "Fichiers objets supprimés !"

fclean: clean
	@rm -rf $(NAME)
	@echo "Executable supprimé !"

re: fclean all

.PHONY: all clean fclean re print_message
