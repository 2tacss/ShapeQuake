#!/bin/zsh
trap 'kill $(jobs -p) 2>/dev/null' EXIT

$SQ_SERVER/sq_server &
sleep 1

$SQ_SHELL/sq_shell_test
