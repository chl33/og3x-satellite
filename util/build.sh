#! /bin/sh

set -e
here="$(readlink -f "$(dirname "$0")")"
cd "${here}"/..
pio pkg install
if [ ! -d ./util/venv ]; then
    python3 -m venv ./util/venv
    . ./util/venv/bin/activate
    pip install protobuf grpcio-tools
else
    . ./util/venv/bin/activate
fi
mkdir -p proto-build/og3
cp ./proto/* proto-build/og3
cd proto-build
"${here}/../".pio/libdeps/native/Nanopb/generator/nanopb_generator og3/satellite.proto
cd ..
mv ./proto-build/og3/satellite.pb.h include/og3/
mv ./proto-build/og3/satellite.pb.c src/
