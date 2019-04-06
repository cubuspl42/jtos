FROM ubuntu:18.04
WORKDIR /build
COPY ./build.sh .
RUN apt-get -qq update
RUN apt-get -qq install build-essential
RUN apt-get -qq install clang
RUN apt-get -qq install gnu-efi
RUN apt-get -qq install ovmf
RUN apt-get -qq install mtools
