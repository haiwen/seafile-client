#!/bin/bash

set -e

# this seems to cover the bases on OSX, and someone will
# have to tell me about the others.
get_script_path () {
  local path="$1"
  [[ -L "$path" ]] || { echo "$path" ; return; }

  local target="$(readlink "$path")"
  if [[ "${target:0:1}" == "/" ]]; then
    echo "$target"
  else
    echo "${path%/*}/$target"
  fi
}

declare -r script_path="$(get_script_path "$BASH_SOURCE")"
declare -r script_dir="${script_path%/*}"

cd $script_dir
searpc-codegen.py rpc_table.py
