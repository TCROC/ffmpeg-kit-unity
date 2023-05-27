#!/bin/bash

get_local_asm_arch() {
  case ${ARCH} in
  armv7*)
    echo "arm"
    ;;
  arm64*)
    echo "arm64"
    ;;
  x86-64*)
    echo "x86_64"
    ;;
  *)
    echo "${ARCH}"
    ;;
  esac
}

# UPDATE BUILD FLAGS AND SET BUILD OPTIONS
ASM_OPTIONS="OS=darwin"
case ${ARCH} in
armv7 | armv7s)
  CFLAGS+=" -DHAVE_NEON"
  ;;
arm64*)
  CFLAGS+=" -DHAVE_NEON_AARCH64"
  ;;
*)
  CFLAGS+=" -DHAVE_AVX2"
  ;;
esac

# ALWAYS CLEAN THE PREVIOUS BUILD
make clean 2>/dev/null 1>/dev/null

# DISCARD APPLE WORKAROUNDS
git checkout "${BASEDIR}"/src/"${LIB_NAME}"/build || return 1
git checkout "${BASEDIR}"/src/"${LIB_NAME}"/codec || return 1

# MAKE SURE THAT ASM IS ENABLED FOR ALL IOS ARCHITECTURES - EXCEPT x86-64
${SED_INLINE} 's/arm64 aarch64/arm64% aarch64/g' ${BASEDIR}/src/${LIB_NAME}/build/arch.mk
${SED_INLINE} 's/%86 x86_64,/%86 x86_64 x86-64%,/g' ${BASEDIR}/src/${LIB_NAME}/build/arch.mk
${SED_INLINE} 's/filter-out arm64,/filter-out arm64%,/g' ${BASEDIR}/src/${LIB_NAME}/build/arch.mk
${SED_INLINE} 's/CFLAGS += -DHAVE_NEON/#CFLAGS += -DHAVE_NEON/g' ${BASEDIR}/src/${LIB_NAME}/build/arch.mk
${SED_INLINE} 's/ifeq (\$(ASM_ARCH), arm64)/ifneq (\$(filter arm64%, \$(ASM_ARCH)),)/g' ${BASEDIR}/src/${LIB_NAME}/codec/common/targets.mk

make -j$(get_cpu_count) \
  ASM_ARCH="$(get_local_asm_arch)" \
  ARCH="${ARCH}" \
  CC="${CC}" \
  CFLAGS="$CFLAGS" \
  CXX="${CXX}" \
  CXXFLAGS="${CXXFLAGS}" \
  LDFLAGS="$LDFLAGS" \
  ${ASM_OPTIONS} \
  PREFIX="${LIB_INSTALL_PREFIX}" \
  SDK_MIN="${IOS_MIN_VERSION}" \
  SDKROOT="${SDK_PATH}" \
  STATIC_LDFLAGS="-lc++" \
  install-headers openh264.pc || return 1

if [ $FFMPEG_KIT_BUILD_TYPE = ios -a ${ARCH} = arm64 ] ; then
  install -m 644 openh264.pc ${INSTALL_PKG_CONFIG_DIR}/openh264.pc || return 1

  curl -o ./libopenh264-2.3.1-ios.a.bz2 http://ciscobinary.openh264.org/libopenh264-2.3.1-ios.a.bz2 || return 1
  bunzip2 libopenh264-2.3.1-ios.a.bz2 || return 1
  mkdir -p "${LIB_INSTALL_PREFIX}"/lib || return 1
  cp libopenh264-2.3.1-ios.a "${LIB_INSTALL_PREFIX}"/lib/libopenh264.a || return 1
else
  case ${ARCH} in
    arm64*)
      ARCH_NAME=arm64
      ;;
    x86-64*)
      ARCH_NAME=x64
      ;;
  esac

  ${SED_INLINE} "s/-lopenh264/-lopenh264-${ARCH_NAME}/g" openh264.pc
  install -m 644 openh264.pc ${INSTALL_PKG_CONFIG_DIR}/openh264.pc || return 1

  curl -o ./libopenh264-2.3.1-mac-${ARCH_NAME}.dylib.bz2 http://ciscobinary.openh264.org/libopenh264-2.3.1-mac-${ARCH_NAME}.dylib.bz2 || return 1
  bunzip2 libopenh264-2.3.1-mac-${ARCH_NAME}.dylib.bz2 || return 1
  mkdir -p "${LIB_INSTALL_PREFIX}"/lib || return 1 
  cp libopenh264-2.3.1-mac-${ARCH_NAME}.dylib "${LIB_INSTALL_PREFIX}"/lib/libopenh264-${ARCH_NAME}.dylib || return 1
fi
