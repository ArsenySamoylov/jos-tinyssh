#!/bin/sh -e

top="`pwd`"
build="`pwd`/build"
bin="${build}/bin"
man="${build}/man"
lib="${build}/lib"
precompiled="${top}/pre-compiled"
include="${build}/include"
log="${build}/log"
work="${build}/work"
contribdir="`pwd`/build-contrib"

rm -rf "${build}"
mkdir -p "${build}"
mkdir -p "${bin}"
mkdir -p "${man}"
mkdir -p "${lib}"
mkdir -p "${include}"

exec 5>"${log}"
exec 2>&5
exec </dev/null

log0() {
  echo "=== `date` === $@" >&5
}

log1() {
  echo "=== `date` === $@"
  echo "=== `date` === $@" >&5
}

log2() {
  echo "=== `date` ===   $@"
  echo "=== `date` ===   $@" >&5
}

version=`head -1 "${top}/debian/changelog"  | cut -d '(' -f2 | cut -d ')' -f1` #XXX
if [ x"${version}" = x ]; then
  version=noversion
fi

LANG=C
export LANG

log0 "uname -a: `uname -a || :`"
log0 "uname -F: `uname -F || :`"
log0 "uname -M: `uname -M || :`"
log0 "ulimit -a: `echo; ulimit -a || :`"

log1 "obtaining compiler"
rm -rf "${work}"
mkdir -p "${work}"
(
  cd "${work}"
  (
    if [ x"${CC}" != x ]; then
      echo "${CC} "
    fi
    cat "${top}/conf-cc"
  ) | while read compiler
  do
    echo 'int main(void) { return 0; }' > try.c
    ${compiler} -o try try.c || { log2 "${compiler} failed"; continue; }
    log2 "${compiler} ok"
    echo "${compiler}" > compiler
    break
  done
)
compiler=`head -1 "${work}/compiler"`
log1 "finishing"

log1 "checking compiler options"
rm -rf "${work}"
mkdir -p "${work}"
(
  cd "${work}"
  cflags=`cat "${top}/conf-cflags" || :`
  cflags="${CPPFLAGS} ${CFLAGS} ${LDFLAGS} ${cflags}"

  for i in ${cflags}; do
    echo 'int main(void) { return 0; }' > try.c
    ${compiler} ${options} "${i}" -o try try.c || { log2 "${i} failed"; continue; }
    options="${options} ${i}"
    log2 "${i} ok"
  done
  echo ${options} > options
)
compilerorig=${compiler}
compiler="${compiler} `cat ${work}/options`"
log2 "${compiler}"
log1 "finishing"

log1 "checking libs"
rm -rf "${work}"
mkdir -p "${work}"
(
  cd "${work}"
  (
    cat "${top}/conf-libs" || :
  ) | (
    exec 9>syslibs
    while read i; do
      echo 'int main(void) { return 0; }' > try.c
      ${compiler} ${i} -o try try.c || { log2 "${i} failed"; continue; }
      echo "${i}" >&9
      log2 "${i} ok"
    done
  )
)
libs=`cat "${work}/syslibs"`
log1 "finishing"

log1 "checking \$LIBS"
if [ x"${LIBS}" != x ]; then
  rm -rf "${work}"
  mkdir -p "${work}"
  (
    cd "${work}"
    for i in ${LIBS}; do
      echo 'int main(void) { return 0; }' > try.c
      ${compiler} ${i} -o try try.c || { log2 "${i} failed"; continue; }
      syslibs="${syslibs} ${i}"
      log2 "${i} ok"
    done
    echo ${syslibs} > syslibs
 )
fi
libsorig=${libs}
libs="${libs} `cat ${work}/syslibs`"
log1 "finishing"

origlibs="${precompiled}/libtinysshcrypto.a ${origlibs}"
libs="${precompiled}/libtinysshcrypto.a ${libs}"

rm -rf "${work}"
mkdir -p "${work}"
cp -pr tinyssh/* "${work}"
cp -pr tinyssh-tests/*test.c "${work}"
cp -pr tinyssh-tests/*.h "${work}"
cp -pr _tinyssh/* "${work}" 2>/dev/null || :
(
  cd "${work}"
  log1 "starting tinyssh objects"
  touch SOURCES TARGETS _TARGETS
  cat SOURCES TARGETS _TARGETS\
  | while read x
  do
    ${compiler} "-DCOMPILER=\"${compilerorig}\"" "-DVERSION=\"${version}\"" -I"${include}" -c "${x}.c" || { log2 "${x}.o failed ... see the log ${log}"; exit 111; }
    log2 "${x}.o ok"
  done || exit 111
  ar cr libtinyssh.a `cat LIBS` || exit 111
  log1 "finishing"

  log1 "starting tinyssh"
  cat TARGETS \
  | while read x
  do
    ${compiler} -fno-pic ../../../obj/lib/entry.o -T../../../bin/bin.ld -I"${include}" -o "${x}" "${x}.o" libtinyssh.a ${libs} || { log2 "${x} failed ... see the log ${log}"; exit 111; }
    log2 "${x} ok"
    cp -p "${x}" "${bin}/${x}";
  done || exit 111
  log1 "finishing"

) || exit 111

log1 "starting manpages"
cp -pr man/* "${man}"
log1 "finishing"

exit 0
