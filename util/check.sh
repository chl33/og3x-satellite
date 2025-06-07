#! /bin/sh
set -e
root="$(readlink -f "$(dirname "$0")"/..)"
cd "$root"
./util/license-headers.sh
./util/build.sh
pio test -e native
