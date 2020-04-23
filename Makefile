DIR_O = objs

SRCS = Listener.cpp \
	Handler.cpp \
	main.cpp

OBJS = $(addprefix $(DIR_O)/,$(SRCS:.cpp=.o))

NAME = webserv

CC = clang++

CFLAGS = -Wall -Wextra -Werror

all: $(NAME)

$(DIR_O)/%.o: %.cpp
	mkdir -p objs
	$(CC) $(CFLAGS) -o $@ -c $<

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf $(DIR_O)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: clean all fclean re