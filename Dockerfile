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

ARG ENV_COMMIT_HASH=""
ENV ENV_COMMIT_HASH="${ENV_COMMIT_HASH}"

ARG ENV_BRANCH="dev"
ENV ENV_BRANCH="${ENV_BRANCH}"

ARG ENV_CONFIGURATION=""
ENV ENV_CONFIGURATION="${ENV_CONFIGURATION}"

ARG ENV_CLONE=""
ENV ENV_CLONE="${ENV_CLONE}"

ARG APPVEYOR_API_URL=""
ENV APPVEYOR_API_URL="${APPVEYOR_API_URL}"

RUN echo 'set number\n\
colorscheme torte' >> ~/.vimrc

RUN apt-get update \
    && apt-get install -y software-properties-common \ 
    && add-apt-repository ppa:ubuntu-toolchain-r/test \
    && apt-get update \
    && apt-get install -y gcc-5 g++-5 \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-5 60 --slave /usr/bin/g++ g++ /usr/bin/g++-5 \
    && apt-get install -y --force-yes make git autoconf libtool gyp lcov curl vim

####
    
COPY . local

RUN if [ $ENV_CLONE ] ; then git clone --recursive https://github.com/mediagoom/mg.git \
        && cd mg \
        && git checkout $ENV_BRANCH \
        ; else cp --recursive local mg \
    ; fi

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
    && if [ "$ENV_CONFIGURATION" = "Debug" -o "$ENV_CODECOV_MG" ]; then echo "DEBUG BUILD" && ./configure --enable-debug; else ./configure ; fi \
    && make check 

RUN if [ "$ENV_CONFIGURATION" = "Debug" -o "$ENV_CODECOV_MG" ]; then \
       cd mg/src/b64 \
    && find . -type f -name '*.gcda' -exec gcov {} + \
    && cd ../mg/core \
    && find . -type f -name '*.gcda' -exec gcov {} + \
    && cd ../media \
    && find . -type f -name '*.gcda' -exec gcov {} + \
    && cd ../../mgcli \
    && find . -type f -name '*.gcda' -exec gcov {} + \
    && cd ../../test \
    && find . -type f -name '*.gcda' -exec gcov {} + \
    ; fi
    
RUN curl -s https://codecov.io/bash > codecov \
    && chmod +x codecov

RUN cd mg && if [ "$ENV_CODECOV_MG" ]; then \
                if [ $ENV_COMMIT_HASH ] ; then ../codecov -t "$ENV_CODECOV_MG" -X gcov -X gcovout -C $ENV_COMMIT_HASH -B $ENV_BRANCH \
                ; else if [ $ENV_CLONE ] ; then ../codecov -t "$ENV_CODECOV_MG" -X gcov -X gcovout; fi \
                ; fi \
             ; fi

RUN if [ $ENV_GITHUB_TOKEN ] ; then mv /build/mg/src/mg/media/mp4 /build/mg/src/mg/media/mp4tmp \
&& cd /build/mg/src/mg/media \
&& msg="$(git log -1 --pretty=format:%s | sed 's/\//---/')" \
&& git clone https://github.com/mediagoom/mp4.git \
&& mv /build/mg/src/mg/media/mp4tmp/* /build/mg/src/mg/media/mp4 \
&& rm mp4tmp -r \
&& git config --global user.email "$ENV_GITHUB_USER@hotmail.com" \
&& git config --global user.name "$ENV_GITHUB_USER" \
&& cd mp4 \
&& msg2="$(git log -1 --pretty=format:%s | sed 's/\//---/')" \
&& echo "--[$msg]--[$msg2]--" \
&& sed -i "s/@mediagen/@mediagen - $msg -/" $(grep -l 'mediagen' ./*) \
&& git add *.cpp && git add *.h && git commit -m "$msg" \
&& if [ "$msg" -ne "$msg2" ] ; then git push https://$ENV_GITHUB_USER:$ENV_GITHUB_TOKEN@github.com/mediagoom/mp4.git; fi \
; fi


RUN if [ $ENV_CODECOV_MP4 ] ; then \
cd /build/mg/src/mg/media \
&& find ./mp4 -type f -name '*.gcov' -delete \
&& grep -l '0:Source:mp4/' ./*.gcov | xargs cp -t mp4 \
&& find . -type f -name '*.gcov' -exec sed -i 's/0:Source:mp4\//0:Source:/' {} + \
&& cd /build/mg/src/mg/media/mp4 \
&& ../../../../../codecov -t "$ENV_CODECOV_MP4" -X gcov -X gcovout \
&& echo "-----------" \
&& cd /build/mg/src/mgcli \
&& find ../../src/mg/media/mp4 -type f -name '*.gcov' -delete \
&& grep -l '0:Source:../../src/mg/media/mp4/' ./*.gcov | xargs cp -t ../../src/mg/media/mp4 \
&& find ../../src/mg/media/mp4 -type f -name '*.gcov' -exec sed -i 's/0:Source:..\/..\/src\/mg\/media\/mp4\//0:Source:/' {} + \
&& cd /build/mg/src/mg/media/mp4 \
&& ../../../../../codecov -t "$ENV_CODECOV_MP4" -X gcov -X gcovout \
&& echo "-----------" \
&& cd /build/mg/test \
&& find ../src/mg/media/mp4 -type f -name '*.gcov' -delete \
&& grep -l '0:Source:../src/mg/media/mp4/' ./*.gcov | xargs cp -t ../src/mg/media/mp4 \
&& find ../src/mg/media/mp4 -type f -name '*.gcov' -exec sed -i 's/0:Source:..\/src\/mg\/media\/mp4\//0:Source:/' {} + \
&& cd /build/mg/src/mg/media/mp4 \
&& ../../../../../codecov -t "$ENV_CODECOV_MP4" -X gcov -X gcovout \
; fi

RUN cd mg/test && python ./test_multiple_bitrate.py

RUN ln -s /build/mg/src/mgcli/mg /usr/local/mg

 
   

    

