FROM ubuntu:24.10

ENV TZ=America/Sao_Paulo
RUN apt-get update && apt-get install -y tzdata && \
    ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && \
    echo $TZ > /etc/timezone

RUN cat <<EOF | tee /etc/apt/sources.list
deb http://archive.ubuntu.com/ubuntu/ focal main universe multiverse restricted
deb http://security.ubuntu.com/ubuntu/ focal-security main universe multiverse restricted
deb http://archive.ubuntu.com/ubuntu/ focal-updates main universe multiverse restricted
deb http://archive.ubuntu.com/ubuntu/ focal-backports main universe multiverse restricted

deb-src http://archive.ubuntu.com/ubuntu/ focal main universe multiverse restricted
deb-src http://security.ubuntu.com/ubuntu/ focal-security main universe multiverse restricted
deb-src http://archive.ubuntu.com/ubuntu/ focal-updates main universe multiverse restricted
deb-src http://archive.ubuntu.com/ubuntu/ focal-backports main universe multiverse restricted
EOF

RUN dpkg --add-architecture i386 && \
    apt update && \
    apt install -y \
    build-essential \
    binutils-x86-64-linux-gnu \
    lib32stdc++6 \
    libc6-i386 \
    libc6-dev-i386 \
    lib32z1 \
    lib32ncurses6 \
    libbz2-1.0:i386 \
    gcc-multilib \
    g++-multilib \
    bin86 \
    qemu-system-x86=1:4.2-3ubuntu6.29 \
    qemu-kvm \
    gdb \
    konsole \
    git
