#!/bin/bash
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
$SCRIPT_DIR/host.exe $WSL_DISTRO_NAME $LOGNAME "$(pwd)" "$@"
