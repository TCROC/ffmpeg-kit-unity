#!/bin/bash

export NETTLE_CFLAGS="-I${LIB_INSTALL_BASE}/nettle/include"
export NETTLE_LIBS="-L${LIB_INSTALL_BASE}/nettle/lib -lnettle -L${LIB_INSTALL_BASE}/gmp/lib -lgmp"
export HOGWEED_CFLAGS="-I${LIB_INSTALL_BASE}/nettle/include"
export HOGWEED_LIBS="-L${LIB_INSTALL_BASE}/nettle/lib -lhogweed -L${LIB_INSTALL_BASE}/gmp/lib -lgmp"
export GMP_CFLAGS="-I${LIB_INSTALL_BASE}/gmp/include"
export GMP_LIBS="-L${LIB_INSTALL_BASE}/gmp/lib -lgmp"

export CFLAGS="$(get_cflags ${LIB_NAME}) -I${LIB_INSTALL_BASE}/gnutls/include ${NETTLE_CFLAGS} ${HOGWEED_CFLAGS} ${GMP_CFLAGS}"
export CXXFLAGS="$(get_cxxflags ${LIB_NAME}) -I${LIB_INSTALL_BASE}/gnutls/include ${NETTLE_CFLAGS} ${HOGWEED_CFLAGS} ${GMP_CFLAGS}"
export LDFLAGS="$(get_ldflags ${LIB_NAME}) -L${LIB_INSTALL_BASE}/gnutls/lib -L${LIB_INSTALL_BASE}/gmp/lib ${NETTLE_LIBS} ${HOGWEED_LIBS} ${GMP_LIBS}"

mkdir -p "${BUILD_DIR}" || return 1
cd "${BUILD_DIR}" || return 1

cmake -Wno-dev \
  -DCMAKE_VERBOSE_MAKEFILE=0 \
  -DCMAKE_C_FLAGS="${CFLAGS}" \
  -DCMAKE_CXX_FLAGS="${CXXFLAGS}" \
  -DCMAKE_EXE_LINKER_FLAGS="${LDFLAGS}" \
  -DCMAKE_SYSROOT="${SDK_PATH}" \
  -DCMAKE_FIND_ROOT_PATH="${SDK_PATH}" \
  -DCMAKE_OSX_SYSROOT="$(get_sdk_name)" \
  -DCMAKE_OSX_ARCHITECTURES="$(get_cmake_osx_architectures)" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_SYSTEM_NAME=Darwin \
  -DCMAKE_INSTALL_PREFIX="${LIB_INSTALL_PREFIX}" \
  -DCMAKE_C_COMPILER="$CC" \
  -DCMAKE_CXX_COMPILER="$CXX" \
  -DCMAKE_LINKER="$LD" \
  -DCMAKE_AR="$(xcrun --sdk $(get_sdk_name) -f ar)" \
  -DUSE_ENCLIB=gnutls \
  -DENABLE_SHARED=0 \
  -DENABLE_TESTING=0 \
  -DCMAKE_SYSTEM_PROCESSOR="$(get_target_cpu)" \
  "${BASEDIR}"/src/"${LIB_NAME}" || return 1
make -j$(get_cpu_count) || return 1
make install || return 1

cat "${LIB_INSTALL_PREFIX}"/lib/pkgconfig/srt.pc | perl -0pe "s/\nLibs.private://" | perl -pe "s/Requires.private:/Requires:/" > "${INSTALL_PKG_CONFIG_DIR}/srt.pc" || return 1
cat "${LIB_INSTALL_PREFIX}"/lib/pkgconfig/haisrt.pc | perl -0pe "s/\nLibs.private://" | perl -pe "s/Requires.private:/Requires:/" > "${INSTALL_PKG_CONFIG_DIR}/haisrt.pc" || return 1