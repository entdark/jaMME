#!/bin/bash
echo "start jaMME"
script_path=$(cd "$(dirname "$0")"; pwd -P)
exec ${script_path}/jamme.app/Contents/MacOS/jamme --args "+set fs_game \"mme\" +set fs_extraGames \"japlus japp\""