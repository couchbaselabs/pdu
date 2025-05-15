ARG ARCH=aarch64
FROM quay.io/pypa/manylinux2014_${ARCH}:latest

RUN yum install -y curl zip unzip tar make git gcc-c++ \
 && git clone https://github.com/ninja-build/ninja.git \
 && cd ninja \
 && git checkout v1.11.1 \
 && ./configure.py --bootstrap \
 && cp ninja /usr/local/bin/ \
 && cd .. \
 && rm -rf ninja

# Install CMake 3.25 or newer
RUN curl -L -o cmake.sh https://github.com/Kitware/CMake/releases/download/v3.25.3/cmake-3.25.3-linux-aarch64.sh \
 && chmod +x cmake.sh \
 && ./cmake.sh --skip-license --prefix=/usr/local \
 && rm cmake.sh

ENV VCPKG_FORCE_SYSTEM_BINARIES=1
RUN git clone https://github.com/microsoft/vcpkg.git \
 && cd vcpkg && git checkout 2024.09.30 \
 && ./bootstrap-vcpkg.sh -disableMetrics

ENV VCPKG_ROOT=/vcpkg
ENV PATH="$PATH:$VCPKG_ROOT"
