set -eu
set -o pipefail

# utilities

fix_configure() {
 sed -i 's/^\s*lt_prog_compiler_wl=$/lt_prog_compiler_wl=-Wl,/g' "$1"
 sed -i 's/^\s*lt_prog_compiler_pic=$/lt_prog_compiler_pic=-fPIC/g' "$1"
 sed -i 's/^\s*lt_prog_compiler_static=$/lt_prog_compiler_static=-static/g' "$1"
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
 github_tar curl curl curl-8_10_0
 mkdir build && cd "$_"
 cmake ../ -DCMAKE_C_FLAGS=-fPIC
 make && make test-quiet
}

test_git() {
 github_tar git git v2.47.1
 make CC="$CC" test
}

test_postgres() {
 github_tar postgres postgres REL_17_2
 sed -i 's/^\#if defined(__GNUC__) || defined(__INTEL_COMPILER)/#if 1/g' src/include/storage/s_lock.h
 sed -i 's/^\#if (defined(__x86_64__) || defined(_M_AMD64))/#if 0/g' src/include/port/simd.h
 ./configure && make && make check 
}

test_sqlite() {
 github_tar sqlite sqlite version-3.47.1
 fix_configure "./configure"
 CFLAGS=-D_GNU_SOURCE ./configure
 make tcltest
}

build_nano() {
 url_tar https://git.savannah.gnu.org/cgit/nano.git/snapshot/nano-8.2.tar.gz nano
 sed -i 's/--depth=2222/--depth=1/g' autogen.sh
 bash autogen.sh
 ./configure && make
}

# run a test

if [[ $(type -t "$1") != function ]]; then
  echo 'expected a test name'
  exit 1
fi

$1
