#!/bin/sh

CONFIGURE_APPEND=""
EMUL_TARGET_LIST=""
VIRTIOGL_EN=""
YAGL_EN=""
YAGL_STATS_EN=""
VIGS_EN=""

usage() {
    echo "usage: build.sh [options] [target]"
    echo ""
    echo "target"
    echo "    emulator target, one of: [x86|i386|i486|i586|i686|arm|all]. Defaults to \"all\""
    echo ""
    echo "options:"
    echo "-d, --debug"
    echo "    build debug configuration"
    echo "-vgl|--virtio-gl"
    echo "    enable virtio GL support"
    echo "-yagl|--yagl-device"
    echo "    enable YaGL passthrough device"
    echo "-ys|--yagl-stats"
    echo "    enable YaGL stats"
    echo "-vigs|--vigs-device"
    echo "    enable VIGS device"
    echo "-e|--extra"
    echo "    extra options for QEMU configure"
    echo "-u|-h|--help|--usage"
    echo "    display this help message and exit"
}

virtgl_enable() {
  case "$1" in
  0|no|disable)
    VIRTIOGL_EN="no"
  ;;
  1|yes|enable)
    VIRTIOGL_EN="yes"
  ;;
  *)
    usage
    exit 1
  ;;
  esac
}

yagl_enable() {
  case "$1" in
  0|no|disable)
    YAGL_EN="no"
  ;;
  1|yes|enable)
    YAGL_EN="yes"
  ;;
  *)
    usage
    exit 1
  ;;
  esac
}

yagl_stats_enable() {
  case "$1" in
  0|no|disable)
    YAGL_STATS_EN="no"
  ;;
  1|yes|enable)
    YAGL_STATS_EN="yes"
  ;;
  *)
    usage
    exit 1
  ;;
  esac
}

vigs_enable() {
  case "$1" in
  0|no|disable)
    VIGS_EN="no"
  ;;
  1|yes|enable)
    VIGS_EN="yes"
  ;;
  *)
    usage
    exit 1
  ;;
  esac
}

set_target() {
  if [ ! -z "$EMUL_TARGET_LIST" ] ; then
      usage
      exit 1
  fi

  case "$1" in
  x86|i386|i486|i586|i686)
    EMUL_TARGET_LIST="i386-softmmu"
    if [ -z "$VIRTIOGL_EN" ] ; then
      virtgl_enable yes
    fi
    if [ -z "$YAGL_EN" ] ; then
      yagl_enable yes
    fi
    if [ -z "$VIGS_EN" ] ; then
      vigs_enable yes
    fi
  ;;
  arm)
    EMUL_TARGET_LIST="arm-softmmu"
    if [ -z "$YAGL_EN" ] && [ "$targetos" != "Darwin" ] ; then
      yagl_enable yes
    fi
    if [ -z "$VIGS_EN" ] && [ "$targetos" != "Darwin" ] ; then
      vigs_enable yes
    fi
  ;;
  all)
#    EMUL_TARGET_LIST="i386-softmmu,arm-softmmu"
    EMUL_TARGET_LIST="i386-softmmu"
    if [ -z "$VIRTIOGL_EN" ] ; then
      virtgl_enable yes
    fi
    if [ -z "$YAGL_EN" ] ; then   
        yagl_enable yes
    fi
    if [ -z "$VIGS_EN" ] ; then
      vigs_enable yes
    fi
  ;;
  esac
}


# OS specific
targetos=`uname -s`
echo "##### checking for os... targetos $targetos"

echo "$*"

while [ "$#" -gt "0" ]
do
    case $1 in
    x86|i386|i486|i586|i686|arm|all)
        set_target $1
    ;;
    -d|--debug)
        CONFIGURE_APPEND="$CONFIGURE_APPEND --enable-debug"
    ;;
    -e|--extra)
        shift
        CONFIGURE_APPEND="$CONFIGURE_APPEND $1"
    ;;
    -vgl|--virtio-gl)
        virtgl_enable 1
    ;;
    -yagl|--yagl-device)
        yagl_enable 1
    ;;
    -ys|--yagl-stats)
        yagl_stats_enable 1
    ;;
    -vigs|--vigs-device)
        vigs_enable 1
    ;;
    -u|-h|--help|--usage)
        usage
        exit 0
    ;;
    *)
        echo "Syntax Error"
        usage
        exit 1
    ;;
    esac
    shift
done

if [ -z "$EMUL_TARGET_LIST" ] ; then
  set_target all
fi

CONFIGURE_APPEND="--target-list=$EMUL_TARGET_LIST $CONFIGURE_APPEND"

if test "$VIRTIOGL_EN" = "yes" ; then
  CONFIGURE_APPEND="$CONFIGURE_APPEND --enable-gl"
else
  CONFIGURE_APPEND="$CONFIGURE_APPEND --disable-gl"
fi

if test "$YAGL_EN" = "yes" ; then
  CONFIGURE_APPEND="$CONFIGURE_APPEND --enable-yagl"
else
  CONFIGURE_APPEND="$CONFIGURE_APPEND --disable-yagl"
fi

if test "$YAGL_STATS_EN" = "yes" ; then
  CONFIGURE_APPEND="$CONFIGURE_APPEND --enable-yagl-stats"
else
  CONFIGURE_APPEND="$CONFIGURE_APPEND --disable-yagl-stats"
fi

if test "$VIGS_EN" = "yes" ; then
  CONFIGURE_APPEND="$CONFIGURE_APPEND --enable-vigs"
else
  CONFIGURE_APPEND="$CONFIGURE_APPEND --disable-vigs"
fi

case $targetos in
Linux*)
cd ..
echo ""
echo "##### QEMU configuring for emulator"
echo "##### QEMU configure append:" $CONFIGURE_APPEND
export PKG_CONFIG_PATH=${HOME}/tizen-sdk-dev/distrib/lib/pkgconfig:${PKG_CONFIG_PATH}
exec ./configure \
 --enable-werror \
 --audio-drv-list=alsa \
 --enable-maru \
 --disable-vnc \
 --disable-pie $1 \
 --enable-virtfs \
 --disable-xen \
 $CONFIGURE_APPEND \
;;
MINGW*)
cd ..
echo ""
echo "##### QEMU configuring for emulator"
echo "##### QEMU configure append:" $CONFIGURE_APPEND
# We add CFLAGS '-fno-omit-frame-pointer'.
# A GCC might have a bug related with omitting frame pointer. It generates weird instructions.
exec ./configure \
 --extra-cflags=-fno-omit-frame-pointer \
 --extra-ldflags=-Wl,--large-address-aware \
 --cc=gcc \
 --audio-drv-list=winwave \
 --enable-hax \
 --enable-maru \
 --disable-vnc $1 \
 $CONFIGURE_APPEND \
;;
Darwin*)
cd ..
echo ""
echo "##### QEMU configuring for emulator"
echo "##### QEMU configure append:" $CONFIGURE_APPEND
./configure \
 --extra-cflags=-mmacosx-version-min=10.4 \
 --audio-drv-list=coreaudio \
 --enable-maru \
 --enable-shm \
 --enable-hax \
 --disable-vnc \
 --disable-cocoa \
 --enable-gl \
 --disable-sdl $1 \
 $CONFIGURE_APPEND \
;;
esac
