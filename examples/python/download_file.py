#!/usr/bin/env python3

"""
librepo - download a file
"""

import os
import sys
import shutil

import librepo

URL = "http://mirrors.fedoraproject.org/mirrorlist?repo=updates-released-f20&arch=x86_64"
DEST_FN_1 = "downloaded_mirrorlist_1.txt"
DEST_FN_2 = "downloaded_mirrorlist_2.txt"

def download_1():
    """Example of the simplest usage"""
    fd = os.open(DEST_FN_1, os.O_RDWR|os.O_CREAT|os.O_TRUNC, 0o0666)
    librepo.download_url(URL, fd)

def download_2():
    """Handle could be used if you need specify some network
    options like proxy server etc."""

    # NOTE: LRO_URLS and LRO_MIRRORLIST options of the handle are ignored!!

    h = librepo.Handle()
    #h.setopt(librepo.LRO_PROXY, "http://foo.proxy.bar:8080")

    fd = os.open(DEST_FN_2, os.O_RDWR|os.O_CREAT|os.O_TRUNC, 0o0666)
    librepo.download_url(URL, fd, handle=h)

if __name__ == "__main__":
    download_1()
    download_2()
