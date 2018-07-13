FROM ubuntu:trusty AS build-env
WORKDIR /build

ARG ENV_CODECOV_MP4=""
ENV ENV_CODECOV_MP4="${ENV_CODECOV_MP4}"

ARG ENV_CODECOV_MG=""
ENV ENV_CODECOV_MG="${ENV_CODECOV_MG}"

ARG ENV_GITHUB_TOKEN=""
ENV ENV_GITHUB_TOKEN="${ENV_GITHUB_TOKEN}"

ARG ENV_GITHUB_USER=""
ENV ENV_GITHUB_USER="${ENV_GITHUB_USER}"


RUN apt-get update \
    && apt-get install -y software-properties-common \ 
    && add-apt-repository ppa:ubuntu-toolchain-r/test \
    && apt-get update \
    && apt-get install -y gcc-5 g++-5 \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-5 60 --slave /usr/bin/g++ g++ /usr/bin/g++-5 \
    && apt-get install -y --force-yes make git autoconf libtool gyp lcovcurl 
    
#python3-pip  \#&& pip3 install pyYaml 
    
#&& pip3 install -U cpp-coveralls

#RUN gcc --version

COPY . local

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

RUN cd mg/src/b64 \
    && find . -type f -name '*.gcda' -exec gcov {} + \
    && cd ../mg/core \
    && find . -type f -name '*.gcda' -exec gcov {} + \
    && cd ../media \
    && find . -type f -name '*.gcda' -exec gcov {} + \
    && cd ../../mgcli \
    && find . -type f -name '*.gcda' -exec gcov {} + \
    && cd ../../test \
    && find . -type f -name '*.gcda' -exec gcov {} +
    
RUN curl -s https://codecov.io/bash > codecov \
    && chmod +x codecov

RUN if [$ENV_CODECOV_MG]; then ./codecov -t "$ENV_CODECOV_MG" -X gcov -X gcovout



   

    

