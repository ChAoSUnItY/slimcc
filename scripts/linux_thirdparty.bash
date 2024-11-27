set -eu
set -o pipefail

# utilities

fix_configure() {
 sed -i 's/^\s*lt_prog_compiler_wl=$/lt_prog_compiler_wl=-Wl,/g' "$1"
 sed -i 's/^\s*lt_prog_compiler_pic=$/lt_prog_compiler_pic=-fPIC/g' "$1"
 sed -i 's/^\s*lt_prog_compiler_static=$/lt_prog_compiler_static=-static/g' "$1"
}

replace_line() {
 sed -i s/^"$1"$/"$2"/g "$3"
}

github_tar() {
  mkdir -p "$2"
  curl -fL https://github.com/"$1"/"$2"/archive/refs/tags/"$3".tar.gz | tar xz -C "$2" --strip-components=1
  cd "$2"
}

github_clone() {
  git clone --depth 1 --branch "$3" https://github.com/"$1"/"$2"
  cd "$2"
}

url_tar() {
  mkdir -p "$2"
  curl -fL "$1" | tar xz -C "$2" --strip-components=1
  cd "$2"
}


# tests

test_curl() {
 github_tar curl curl curl-8_10_1
 sed -i 's/^if(MSVC OR CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")$/if (TRUE)/g' tests/CMakeLists.txt
 mkdir build && cd "$_"
 cmake ../ -DCMAKE_C_FLAGS=-fPIC
 make && make test-quiet
}

test_doom() {
 github_tar fuhsnn PureDOOM 20240331
 mkdir -p examples/Tests/build && cd "$_"
 replace_line "project(pd_tests)" "project(pd_tests C)" ../CMakeLists.txt
 cmake ../ && make
 cd ../../../ && examples/Tests/build/pd_tests
}

test_git() {
 github_tar git git v2.47.1
 make CC="$CC" test
}

test_libpng() {
 github_tar pnggroup libpng v1.6.44
 fix_configure ./configure
 ./configure
 make test
}

test_openssh() {
 github_tar openssh openssh-portable V_9_8_P1
 ./configure
 make tests
}

test_php() {
 github_tar php php-src php-8.1.31
 replace_line "#elif (defined(__i386__) || defined(__x86_64__)) && defined(__GNUC__)" "#elif 1" Zend/zend_multiply.h
 replace_line "#elif defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__SUNPRO_C)" "#elif 1" ext/pcre/pcre2lib/sljit/sljitNativeX86_common.c

 # don't work in CI https://github.com/php/php-src/blob/17187c4646f3293e1de8df3f26d56978d45186d6/.github/actions/test-linux/action.yml#L40
 export SKIP_IO_CAPTURE_TESTS=1

 ./buildconf --force
 fix_configure ./configure
 ./configure --disable-opcache
 make test NO_INTERACTION=1
}

test_postgres() {
 github_tar postgres postgres REL_17_2
 replace_line "#if defined(__GNUC__) || defined(__INTEL_COMPILER)" "#if 1" src/include/storage/s_lock.h
 replace_line "#if (defined(__x86_64__) || defined(_M_AMD64))" "#if 0" src/include/port/simd.h
 ./configure && make && make check
}

test_python() {
 github_tar python cpython v3.13.0
 replace_line "#if defined(__GNUC__) || defined(__clang__)" "#if 1" Include/pyport.h
 ./configure && make

 skip_tests=(
  test_external_inspection # https://github.com/fuhsnn/slimcc/issues/105

  # don't work in CI https://github.com/python/cpython/blob/6d3b5206cfaf5a85c128b671b1d9527ed553c930/.github/workflows/build.yml#L408
  test_asyncio test_socket
 )
 ./python -m test --exclude "${skip_tests[@]}"
}

test_sqlite() {
 github_tar sqlite sqlite version-3.47.1
 fix_configure ./configure
 CFLAGS=-D_GNU_SOURCE ./configure
 make tcltest
}

test_zlib() {
 github_tar madler zlib v1.3.1
 ./configure
 make test
}

test_zstd() {
 github_tar facebook zstd v1.5.6
 replace_line "#if defined(__ELF__) && defined(__GNUC__)" "#if 1" lib/decompress/huf_decompress_amd64.S
 make check
}

build_gcc() {
 url_tar https://ftp.gnu.org/gnu/gcc/gcc-4.7.4/gcc-4.7.4.tar.gz gcc47
 export -f fix_configure
 find . -name 'configure' -exec bash -c 'fix_configure "$0"' {} \;
 sed -i 's/^\s*struct ucontext/ucontext_t/g' ./libgcc/config/i386/linux-unwind.h
 mkdir buildonly && cd "$_"
 export MAKEINFO=missing
 ../configure --enable-languages=c,c++ --disable-multilib --disable-bootstrap
 make
}

build_musl() {
 url_tar https://git.musl-libc.org/cgit/musl/snapshot/musl-1.2.5.tar.gz musl
 rm -rf src/complex/
 AR=ar RANLIB=ranlib ../musl/configure --target=x86_64-linux-musl
 make
}

build_nano() {
 url_tar https://git.savannah.gnu.org/cgit/nano.git/snapshot/nano-8.2.tar.gz nano
 sed -i 's/--depth=2222/--depth=1/g' autogen.sh
 bash autogen.sh
 ./configure && make
}

build_sdl() {
 github_tar libsdl-org SDL release-2.30.9
 fix_configure ./configure
 replace_line "#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))" "#elif 1" src/atomic/SDL_spinlock.c
 ./configure
 make
}

# run a test

if [[ $(type -t "$1") != function ]]; then
  echo 'expected a test name'
  exit 1
fi

$1
