DIR_O = objs

SRCS = Listener.cpp \
	Client.cpp \
	Handler.cpp \
	HandlerMethods.cpp \
	Config.cpp \
	main.cpp

OBJS = $(addprefix $(DIR_O)/,$(SRCS:.cpp=.o))

NAME = webserv

CONFIG = webserv.conf

CC = clang++

CFLAGS = -Wall -Wextra -Werror

all: $(NAME)

$(DIR_O)/%.o: %.cpp
	mkdir -p objs
	$(CC) $(CFLAGS) -o $@ -c $<

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

test:
	$(CC) $(SRCS) -o $(NAME)
	./$(NAME) $(CONFIG)

clean:
	rm -rf $(DIR_O)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: clean all fclean re
