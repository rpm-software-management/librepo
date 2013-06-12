CC=gcc
CFLAGS= -Wall -Wextra -g -std=c99 -O3 -I../../
LINKFLAGS= -L../../build/librepo/ -lrepo

all: download_repo

download_repo:
	$(CC) $(CFLAGS) download_repo.c $(LINKFLAGS) -o download_repo

clean:
	rm -f download_repo

run:
	LD_LIBRARY_PATH="../../build/librepo/" ./download_repo
