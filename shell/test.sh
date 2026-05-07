#!/bin/zsh
# スクリプト終了時にバックグラウンドジョブをすべて殺す
trap 'kill $(jobs -p) 2>/dev/null' EXIT

# サーバー起動
$SQ_SERVER/sq_server &
sleep 1

# テスト実行
$SQ_SHELL/sq_shell_test
