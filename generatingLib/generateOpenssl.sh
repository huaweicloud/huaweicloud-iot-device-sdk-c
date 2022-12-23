tar -zxvf openssl-1.1.1h.tar.gz
cd openssl-1.1.1h
./config shared
make depend
make
make install
