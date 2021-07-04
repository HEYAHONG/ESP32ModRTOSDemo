#!bin/bash

if [ -r ~/esp/esp-idf/export.sh ]
then
source ~/esp/esp-idf/export.sh
else
echo "Error,esp-idf in not installed!"
fi

# PROJECT_PATH not set in the environment.
# If using bash or zsh, try to guess PROJECT_PATH from script location.
self_path=""

# shellcheck disable=SC2128  # ignore array expansion warning
if [ -n "${BASH_SOURCE-}" ]
then
self_path="${BASH_SOURCE}"
elif [ -n "${ZSH_VERSION-}" ]
then
self_path="${(%):-%x}"
else
echo "Could not detect PROJECT_PATH. Please set it before sourcing this script:"
echo "  export PROJECT_PATH=(add path here)"
return 1
fi

# shellcheck disable=SC2169,SC2169,SC2039  # unreachable with 'dash'
if [[ "$OSTYPE" == "darwin"* ]]; then
# convert possibly relative path to absolute
script_dir="$(realpath_int "${self_path}")"
# resolve any ../ references to make the path shorter
script_dir="$(cd "${script_dir}" || exit 1; pwd)"
else
# convert to full path and get the directory name of that
script_name="$(readlink -f "${self_path}")"
script_dir="$(dirname "${script_name}")"
fi
export PROJECT_PATH="${script_dir}"
echo "Setting PROJECT_PATH to '${PROJECT_PATH}'"




