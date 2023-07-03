#!/bin/bash

set -e

# https://stackoverflow.com/a/54755784/9733262
# gets full path to directory of script for both bash and zsh
dir="$(dirname "$(readlink -f "${BASH_SOURCE[0]:-${(%):-%x}}")")"
build_dir="$dir/prebuilt"
mac_os_dir="$build_dir/bundle-apple-framework-macos"
mac_os_no_sym_dir="$build_dir/$(basename $mac_os_dir)-no-sym"

rm -rf "$mac_os_no_sym_dir"

if [ ! -d "$mac_os_dir" ]
then
    "$dir/macos.sh" "$@"
fi

for framework_dir in $(ls "$mac_os_dir")
do
    framework_dir_full="$mac_os_dir/$framework_dir"
    target_dir_full="$mac_os_no_sym_dir/$framework_dir"""

    mkdir -p "$target_dir_full"

    for framework_item in $(ls "$framework_dir_full")
    do
        if [[ "$framework_item" == "Versions" ]]
        then
            continue
        fi

        framework_item_full="$framework_dir_full/$framework_item"
        cp -rL "$framework_item_full" "$target_dir_full/$framework_item"
    done
done

echo "built do directory: $mac_os_no_sym_dir"