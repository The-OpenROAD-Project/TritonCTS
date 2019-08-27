FROM centos:centos6 AS builder

# install gcc 6
RUN yum -y install centos-release-scl && \
    yum -y install devtoolset-6 devtoolset-6-libatomic-devel
ENV CC=/opt/rh/devtoolset-6/root/usr/bin/gcc \
    CPP=/opt/rh/devtoolset-6/root/usr/bin/cpp \
    CXX=/opt/rh/devtoolset-6/root/usr/bin/g++ \
    PATH=/opt/rh/devtoolset-6/root/usr/bin:$PATH \
    LD_LIBRARY_PATH=/opt/rh/devtoolset-6/root/usr/lib64:/opt/rh/devtoolset-6/root/usr/lib:/opt/rh/devtoolset-6/root/usr/lib64/dyninst:/opt/rh/devtoolset-6/root/usr/lib/dyninst:/opt/rh/devtoolset-6/root/usr/lib64:/opt/rh/devtoolset-6/root/usr/lib:$LD_LIBRARY_PATH

RUN yum install -y wget git ghostscript glpk-devel bison bison-devel zlib-devel

# Installing newer version of cmake
RUN wget https://cmake.org/files/v3.9/cmake-3.9.0-Linux-x86_64.sh && \
    chmod +x cmake-3.9.0-Linux-x86_64.sh  && \
    ./cmake-3.9.0-Linux-x86_64.sh --skip-license --prefix=/usr/local

# Installing lemon
RUN wget http://lemon.cs.elte.hu/pub/sources/lemon-1.3.1.tar.gz && \
    tar -xf lemon-1.3.1.tar.gz

RUN mkdir lemon-1.3.1/build  && \
    cd lemon-1.3.1/build && \
    cmake -DCMAKE_INSTALL_PREFIX=/lemon_build .. && \
    make && \
    make install

COPY . /TritonCTS
WORKDIR /TritonCTS
RUN ./compileAll.sh /lemon_build $(nproc)
WORKDIR /
RUN git clone https://github.com/cmatsuoka/figlet.git
RUN cd figlet && \
    make

FROM centos:centos6 AS runner
RUN yum update -y && yum install -y tcl-devel libgomp
COPY --from=builder /TritonCTS/bin/genHtree /build/genHtree
COPY --from=builder /TritonCTS/third_party/lefdef2cts /build/lefdef2cts
COPY --from=builder /TritonCTS/src/scripts /build/scripts
COPY --from=builder /TritonCTS/src/tech /build/tech
COPY --from=builder /TritonCTS/runTritonCTS.tcl /build/runTritonCTS.tcl
COPY --from=builder /figlet/figlet /build/figlet
COPY --from=builder /figlet/fonts /build/fonts

RUN useradd -ms /bin/bash openroad
USER openroad
WORKDIR /home/openroad
