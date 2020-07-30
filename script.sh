#!/bin/bash

# This is a minimal set of ANSI/VT100 color codes
_END='\e[0m'
_BOLD='\e[1m'
_UNDER='\e[4m'
_REV='\e[7m'

# Colors
_GREY='\e[30m'
_RED='\e[31m'
_GREEN='\e[32m'
_YELLOW='\e[33m'
_BLUE='\e[34m'
_PURPLE='\e[35m'
_CYAN='\e[36m'
_WHITE='\e[37m'

# Inverted, i.e. colored backgrounds
_IGREY='\e[40m'
_IRED='\e[41m'
_IGREEN='\e[42m'
_IYELLOW='\e[43m'
_IBLUE='\e[44m'
_IPURPLE='\e[45m'
_ICYAN='\e[46m'
_IWHITE='\e[47m'

# **************************************************************************** #

config_files=(webserv.conf)

if [ ! -f "webserv" ]; then
	make
fi

printf "${_ICYAN}${_BOLD}${_GREY}  ARGS TESTS  ${_END}\n"
./webserv
if [ $? == 0 ]; then
	printf "Wrong return"
fi
./webserv aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
if [ $? == 0 ]; then
	printf "Wrong return"
fi
