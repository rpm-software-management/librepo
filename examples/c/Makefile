
# export LD_LIBRARY_PATH="../../build/librepo/"

CC=gcc
CFLAGS= -Wall -Wextra -g -std=c99 -O3 -I../../ `pkg-config --cflags glib-2.0`
LINKFLAGS= -L../../build/librepo/ -lrepo `pkg-config --libs glib-2.0`

all: \
     download_repo \
     download_low_level_api \
     download_packages \
     download_repo_with_callback \
     fastestmirror \
     fastestmirror_with_callback

download_repo:
	$(CC) $(CFLAGS) download_repo.c $(LINKFLAGS) -o download_repo

download_low_level_api:
	$(CC) $(CFLAGS) download_low_level_api.c $(LINKFLAGS) -o download_low_level_api

download_packages:
	$(CC) $(CFLAGS) download_packages.c $(LINKFLAGS) -o download_packages

download_repo_with_callback:
	$(CC) $(CFLAGS) download_repo_with_callback.c $(LINKFLAGS) -o download_repo_with_callback

fastestmirror:
	$(CC) $(CFLAGS) fastestmirror.c $(LINKFLAGS) -o fastestmirror

fastestmirror_with_callback:
	$(CC) $(CFLAGS) fastestmirror_with_callback.c $(LINKFLAGS) -o fastestmirror_with_callback

clean:
	rm -f \
	      download_repo \
	      download_low_level_api \
	      download_packages \
	      download_repo_with_callback \
	      fastestmirror \
	      fastestmirror_with_callback

run:
	LD_LIBRARY_PATH="../../build/librepo/" ./download_repo
