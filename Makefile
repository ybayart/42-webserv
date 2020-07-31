# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: lmartin <lmartin@student.42.fr>            +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2020/06/30 21:31:46 by lmartin           #+#    #+#              #
#    Updated: 2020/07/25 03:29:33 by lmartin          ###   ########.fr        #
#                                                                              #
# **************************************************************************** #
#
## PIMPED MAKEFILE ##

# COLORS #

# This is a minimal set of ANSI/VT100 color codes
_END		=	\e[0m
_BOLD		=	\e[1m
_UNDER		=	\e[4m
_REV		=	\e[7m

# Colors
_GREY		=	\e[30m
_RED		=	\e[31m
_GREEN		=	\e[32m
_YELLOW		=	\e[33m
_BLUE		=	\e[34m
_PURPLE		=	\e[35m
_CYAN		=	\e[36m
_WHITE		=	\e[37m

# Inverted, i.e. colored backgrounds
_IGREY		=	\e[40m
_IRED		=	\e[41m
_IGREEN		=	\e[42m
_IYELLOW	=	\e[43m
_IBLUE		=	\e[44m
_IPURPLE	=	\e[45m
_ICYAN		=	\e[46m
_IWHITE		=	\e[47m

# **************************************************************************** #

## VARIABLES ##

# COMPILATION #

CC			=	clang++

CC_FLAGS	=	-Wall -Wextra -Werror

# COMMANDS #

RM			=	rm -rf

PWD			=	$(shell pwd)


# DIRECTORIES #

DIR_HEADERS =	./includes/

DIR_SRCS	=	./srcs/

DIR_OBJS	=	./compiled_srcs/

DIR_CONFIGS =	./conf/


# FILES #

SRC			=	Client.cpp \
				Config.cpp \
				Handler.cpp \
				HandlerMethods.cpp \
				Helper.cpp \
				HelperStatusCode.cpp \
				Logger.cpp \
				Server.cpp \
				utils.cpp \
				main.cpp 


SRCS		=	$(SRC)

# COMPILED_SOURCES #

OBJS 		=	$(SRCS:%.cpp=$(DIR_OBJS)%.o)

NAME 		=	webserv

CONFIG		=	webserv.conf


# **************************************************************************** #

## RULES ##

all:			$(NAME) $(DIR_CONFIGS)$(CONFIG)

# VARIABLES RULES #

$(NAME):		$(OBJS)
				@printf "\033[2K\r$(_GREEN) All files compiled into '$(DIR_OBJS)'. $(_END)✅\n"
				@$(CC) $(CC_FLAGS) -I $(DIR_HEADERS) $(OBJS) -o $(NAME)
				@printf "$(_GREEN) Executable '$(NAME)' created. $(_END)✅\n"

# COMPILED_SOURCES RULES #

$(OBJS):		| $(DIR_OBJS)


$(DIR_OBJS)%.o: $(DIR_SRCS)%.cpp
				@$(CC) $(CC_FLAGS) -I $(DIR_HEADERS) -c $< -o $@
				@printf "\033[2K\r $(_YELLOW)Compiling $< $(_END)⌛"

$(DIR_OBJS):
				@mkdir $(DIR_OBJS)

$(DIR_CONFIGS)$(CONFIG):
				@cat $(DIR_CONFIGS)webserv_model.conf | sed 's=PWD=$(PWD)=g' > $(DIR_CONFIGS)$(CONFIG)
				@printf "\033[2K\r$(_GREEN) Default config file '$(DIR_CONFIGS)$(CONFIG)' created. $(_END)✅\n"


# OBLIGATORY PART #

clean:
				@$(RM) $(DIR_OBJS)
				@printf "$(_RED) '"$(DIR_OBJS)"' has been deleted. $(_END)🗑️\n"

fclean:			clean
				@$(RM) $(NAME)
				@printf "$(_RED) '"$(NAME)"' has been deleted. $(_END)🗑️\n"
				@$(RM) $(DIR_CONFIGS)$(CONFIG)
				@printf "$(_RED) '"$(DIR_CONFIGS)$(CONFIG)"' has been deleted. $(_END)🗑️\n"

re:				fclean all

# BONUS #

bonus:			all

re_bonus:		fclean bonus

# PHONY #

.PHONY:			all clean fclean re bonus re_bonus
