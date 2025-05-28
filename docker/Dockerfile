ARG ARCH=x86_64
FROM quay.io/pypa/manylinux2014_${ARCH}:latest

# Verify architecture
RUN echo "Building for architecture: ${ARCH}" && \
    uname -m && \
    arch

RUN yum install -y curl zip unzip tar make git gcc-c++ \
 && git clone https://github.com/ninja-build/ninja.git \
 && cd ninja \
 && git checkout v1.11.1 \
 && ./configure.py --bootstrap \
 && cp ninja /usr/local/bin/ \
 && cd .. \
 && rm -rf ninja \
 && ninja --version

# Install CMake 3.25 or newer
RUN curl -L -o cmake.sh https://github.com/Kitware/CMake/releases/download/v3.25.3/cmake-3.25.3-linux-x86_64.sh \
 && chmod +x cmake.sh \
 && ./cmake.sh --skip-license --prefix=/usr/local \
 && rm cmake.sh \
 && cmake --version

ENV VCPKG_FORCE_SYSTEM_BINARIES=1
ENV VCPKG_DEFAULT_TRIPLET=x64-linux
RUN git clone https://github.com/microsoft/vcpkg.git \
 && cd vcpkg && git checkout 2024.09.30 \
 && ./bootstrap-vcpkg.sh -disableMetrics

ENV VCPKG_ROOT=/vcpkg
ENV PATH="$PATH:$VCPKG_ROOT"

# Set CMake configuration
ENV CMAKE_TOOLCHAIN_FILE=/vcpkg/scripts/buildsystems/vcpkg.cmake
ENV CMAKE_MAKE_PROGRAM=/usr/local/bin/ninja
ENV CMAKE_GENERATOR=Ninja
ENV CMAKE_GENERATOR_PLATFORM=x64

# Install vcpkg dependencies
COPY vcpkg.json /vcpkg.json
RUN vcpkg install --triplet=x64-linux --clean-after-build
