#!/usr/bin/env bash

# ------ Variables ------ #
client_line="client.c:92"
server_line="server.c:217"

client_fn="client_wrap_upload"
server_fn="serv_wrap_download"
# ----------------------- #

debug_mode="fn" # fn or line

case $debug_mode in
	"fn")
		client_break="${client_fn}"
		server_break="${server_fn}"
		;;
	"line")
		client_break="${client_line}"
		server_break="${server_line}"
		;;
	*)
		echo "Invalid debug mode"
		exit 1
		;;
esac

alias gdbs="gdb --quiet -ex \"break ${server_break}\" -ex 'run' ./jalebi"
alias gdbc="gdb --quiet -ex \"break ${client_break}\" -ex 'run' ./namak-paare"
