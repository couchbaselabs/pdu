ARG ARCH=x86_64
FROM quay.io/pypa/manylinux2014_${ARCH}:latest

RUN yum install -y curl zip unzip tar make ninja-build gcc gcc-c++ cmake
RUN git clone https://github.com/microsoft/vcpkg.git
RUN cd vcpkg && git checkout 2024.09.30
RUN ./vcpkg/bootstrap-vcpkg.sh -disableMetrics
ENV VCPKG_ROOT=/vcpkg
ENV CMAKE_GENERATOR=Ninja
ENV CMAKE_MAKE_PROGRAM=/usr/bin/ninja
ENV CC=/usr/bin/gcc
ENV CXX=/usr/bin/g++
ENV PATH="$PATH:$VCPKG_ROOT"