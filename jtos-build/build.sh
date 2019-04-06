# Build inside the docker container
cp /src/* .
make
cp disk.img /out
cp /usr/share/ovmf/OVMF.fd /out
