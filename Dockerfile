FROM ubuntu:trusty AS build-env
WORKDIR /build

#$tag = "mgtmp"
#docker build --build-arg HTTP_PROXY=http://192.168.75.1:8888 --build-arg HTTPS_PROXY=http://192.168.75.1:8888 --build-arg http_proxy=http://192.168.75.1:8888 --build-arg https_proxy=http://192.168.75.1:8888 -t $tag .

RUN apt-get update \
    && apt-get install -y software-properties-common \ 
    && add-apt-repository ppa:ubuntu-toolchain-r/test \
    && apt-get update \
    && apt-get install -y gcc-5 g++-5 \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-5 60 --slave /usr/bin/g++ g++ /usr/bin/g++-5 \
    && apt-get install -y make git autoconf libtool gyp lcov python3-pip \
    && pip3 install pyYaml 
    
#&& pip3 install -U cpp-coveralls


RUN gcc --version



RUN git clone --recursive https://github.com/mediagoom/mg.git \
    && cd mg \
    && git checkout dev 


RUN cd mg/deps/libuv/ \
    && ./autogen.sh \
    && ./configure \
    && make \
    && make install \
    && cd ../..

RUN cd mg/deps/AES/ \
    && ./configure \
    && make CC=/usr/bin/gcc \ 
    && make install \
    && cd ../..

RUN cd mg/deps/flavor/ \
    && ./configure \
    && make CC=/usr/bin/gcc \ 
    && make install \
    && cd ../..

ENV LD_LIBRARY_PATH=/usr/local/lib

RUN cd mg \
    && ./bootstrap \
    && ./configure --enable-debug \
    && make \
    && make check 

RUN cd mg/src/mgcli \
    && gcov mp4info.gcda \
    && cd ../../test \
    && gcov test.gcda
    
RUN git clone https://github.com/aseduto/cpp-coveralls.git \
    && cd mg \
   

    

