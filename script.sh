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

dir_conf=./conf/

# list of all config files
broken_conf=(broken.conf broken2.conf broken3.conf)
config_files=(webserv.conf)

if [ "$EUID" -ne 0 ]
  then echo "Please run as root"
  exit
fi

which siege
if [[ $? != 0 ]]; then
	apt-get install siege
fi

# if nginx is running it takes port 80 (for example)
service nginx stop

if [ ! -f "webserv" ]; then
	make
fi

function check_return_not
{
	if [ $? == $1 ]; then
		printf "Wrong return"
	fi
}

printf "${_ICYAN}${_BOLD}${_GREY}  ARGS TESTS  ${_END}\n"
printf "${_GREEN}./webserv${_END}\n"
./webserv
check_return_not 0

printf "${_GREEN}./webserv ${_YELLOW}1 2${_END}\n"
./webserv 1 2
check_return_not 0

printf "${_GREEN}./webserv ${_YELLOW}aaaaa${_END}\n"
./webserv aaaaa
check_return_not 0

printf "${_ICYAN}${_BOLD}${_GREY}  SYNTAX CONFIG TESTS  ${_END}\n"

i=0;
size=${#broken_conf[@]}
while (($i < $size)); do
	printf "${_GREEN}./webserv ${_YELLOW}${dir_conf}${broken_conf[$i]}${_END}\n"
	./webserv ${dir_conf}${broken_conf[$i]}
	check_return_not 0
	i=${i}+1
done

printf "${_ICYAN}${_BOLD}${_GREY}  TESTS FOR CTRL-C  ${_END}\n"

i=0;
while (($i < 10000)); do
	./webserv ${dir_conf}${config_files[0]} &
	pid=$!
	sleep 0.05
	siege --quiet -t1s http://localhost:80 &>/dev/null &
	pid_siege=$!
	sleep 0.5
	kill -2 $pid &>/dev/null
	wait $pid
	if [ $? != 0 ]; then
		exit
	fi
	wait $pid_siege
	i=$(($i+1))
	echo $i
done
