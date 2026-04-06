NAME    = ft_ping
CC      = cc
CFLAGS  = -Wall -Wextra -Werror -Iinclude
SRC_DIR = src
OBJ_DIR = obj
SRCS    = $(shell find $(SRC_DIR) -name "*.c")
OBJS    = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(OBJS) -o $(NAME)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re