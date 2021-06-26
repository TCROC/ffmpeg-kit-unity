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
  -DCMAKE_SYSROOT="${ANDROID_SYSROOT}" \
  -DCMAKE_FIND_ROOT_PATH="${ANDROID_SYSROOT}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_SYSTEM_NAME=Generic \
  -DCMAKE_INSTALL_PREFIX="${LIB_INSTALL_PREFIX}" \
  -DCMAKE_C_COMPILER="${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/bin/$CC" \
  -DCMAKE_CXX_COMPILER="${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/bin/$CXX" \
  -DCMAKE_LINKER="${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/bin/$LD" \
  -DCMAKE_AR="${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${TOOLCHAIN}/bin/$AR" \
  -DUSE_ENCLIB=gnutls \
  -DENABLE_SHARED=0 \
  -DENABLE_TESTING=0 \
  -DCMAKE_SYSTEM_PROCESSOR=$(get_cmake_system_processor) \
  -DCMAKE_POSITION_INDEPENDENT_CODE=1 \
  -DANDROID=1 \
  "${BASEDIR}"/src/"${LIB_NAME}" || return 1
make -j$(get_cpu_count) || return 1
make install || return 1

cat "${LIB_INSTALL_PREFIX}"/lib/pkgconfig/srt.pc | perl -0pe "s/\nLibs.private://" | perl -pe "s/Requires.private:/Requires:/" > "${INSTALL_PKG_CONFIG_DIR}/srt.pc" || return 1
cat "${LIB_INSTALL_PREFIX}"/lib/pkgconfig/haisrt.pc | perl -0pe "s/\nLibs.private://" | perl -pe "s/Requires.private:/Requires:/" > "${INSTALL_PKG_CONFIG_DIR}/haisrt.pc" || return 1