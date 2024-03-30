#!/bin/bash

set -e
set -u

if [ ! -e components ]; then
  echo "Missing 'components' directory."
  exit 1
fi

srcdir="${PWD}/components"
bindir="${PWD}/ci-build"
outdir="${PWD}/artifacts"

mkdir -p artifacts
mkdir -p ci-build && cd ci-build
mkdir -p dashboard
mkdir -p server

emcmake cmake "${srcdir}/dashboard" -B "${bindir}/dashboard" -DCMAKE_BUILD_TYPE=Release
cmake --build "${bindir}/dashboard"

cmake "${srcdir}/server" \
  "-DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" \
  -B "${bindir}/server" \
  "-DCMAKE_BUILD_TYPE=Release" \
  "-DBUNDLE_PATH=${bindir}/dashboard"

cd "${bindir}/server"
cmake --build .
cpack -G ZIP
cpack -G DEB

cp ${bindir}/server/*.zip ${bindir}/server/*.deb "${outdir}"
