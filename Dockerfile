FROM alpine AS bazel

RUN apk add build-base curl git bash perl-utils openjdk11 lld llvm15 llvm15-dev llvm-libunwind-static clang15 clang15-dev libc++ libc++-static libc++-dev libtool musl-dev musl-libintl zip unzip

ENV EXTRA_BAZEL_ARGS --tool_java_runtime_version=local_jdk
ENV CFLAGS="$CFLAGS -D_LARGEFILE64_SOURCE"

WORKDIR /bazel

RUN <<EOF
#!/bin/bash
curl -L -s -o /bazel/bazel.zip https://github.com/bazelbuild/bazel/releases/download/6.5.0/bazel-6.5.0-dist.zip
unzip bazel.zip
rm bazel.zip
# NOTE (k1): Required for x86_64
sed -i'' -e 's/"BLAZE_OPENSOURCE",/\0\n"_LARGEFILE64_SOURCE",/' src/main/cpp/util/BUILD
# NOTE (k1): numbers.h is missing this line - it's present in later versions of Bazel
sed -i'' -e 's/include <string>/\0\n#include <cstdint>/' src/main/cpp/util/numbers.h
./compile.sh
EOF

FROM alpine AS build

RUN echo "ID_LIKE=alpine-linux-musl" >> /etc/os-release

RUN <<EOF
apk add bash binutils build-base curl git libc6-compat perl-utils openjdk11 lld llvm15 llvm15-dev clang15 clang15-dev libc++ libc++-static libc++-dev libtool musl-dev musl-libintl bison m4 sed
apk add clang15-extra-tools --repository=https://dl-cdn.alpinelinux.org/alpine/edge/main
apk add llvm-libunwind-static --repository=https://dl-cdn.alpinelinux.org/alpine/v3.17/main
apk add lld --repository=https://dl-cdn.alpinelinux.org/alpine/v3.17/community
apk add compiler-rt=15.0.7-r1 --repository=https://dl-cdn.alpinelinux.org/alpine/v3.17/main
apk add libexecinfo-dev libexecinfo-static --repository=https://dl-cdn.alpinelinux.org/alpine/v3.16/main/
EOF

COPY --link --from=bazel /bazel/output/bazel /usr/local/bin/bazel

ENV PATH=/usr/lib/llvm15/bin:$PATH
ENV LD_LIBRARY_PATH=/usr/lib/jvm/java-11-openjdk/lib/server

COPY ./ /sorbet

WORKDIR /sorbet

RUN <<EOF
#!/bin/bash

# NOTE (k1): For some reason, this flag isn't respected when doing a --release-linux build and must be set explicitly.
find . -name 'BUILD' | xargs sed -z -i'' -e 's/linkstatic = s[^)]*),\?/linkstatic = 1,/g'
# NOTE (k1): The bundled builds of jemalloc and mimalloc do not work with aarch64
find . -name 'BUILD' | xargs sed -z -i'' -e 's/malloc = s[^)]*),\?//g'
# NOTE (k1): On x86_64, we need _LARGEFILE64_SOURCE for fstat64
sed -i'' -e 's/"SPDLOG_COMPILED_LIB",/\0\n"_LARGEFILE64_SOURCE",/' third_party/spdlog.BUILD
# NOTE (k1): We want a fully statically-linked binary. libexecinfo is used on alpine to provide backtraces.
sed -i'' -e 's/"-lm"/"-lm","-static","\/usr\/lib\/libexecinfo.a"/' common/BUILD
# NOTE (k1): backtrace does not have set_debug_level with libexecinfo
sed -i'' -e 's/..set_debug_level.*//' parser/parser/cc/driver.cc

if [[ "$(arch)" == "aarch64" ]]; then
  bazel build //main:sorbet --config=release-linux-aarch64 --verbose_failures
else
  bazel build //main:sorbet --config=release-linux --verbose_failures
fi

strip --strip-unneeded /sorbet/bazel-bin/main/sorbet
EOF

FROM alpine

COPY --from=build /sorbet/bazel-bin/main/sorbet /sorbet
