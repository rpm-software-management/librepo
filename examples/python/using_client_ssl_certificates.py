#!/usr/bin/env python

"""
librepo - download a package using client SSL certificate
"""
import librepo


if __name__ == "__main__":

    h = librepo.Handle()
    h.setopt(librepo.LRO_REPOTYPE, librepo.LR_YUMREPO)
    h.setopt(librepo.LRO_URLS, ["https://my-ssl-repo/fedora/20/x86_64/os/"])

    # The client certificate .pem file may include just the public certificate
    # or may include an entire certificate chain including
    # public key, private key, and root certificates.
    h.setopt(librepo.LRO_SSLCLIENTCERT, "/etc/pki/client.pem")

    # In case the .pem just contains the public key, set the private key
    h.setopt(librepo.LRO_SSLCLIENTKEY, "/etc/client.key")

    # Set the certificate authority
    h.setopt(librepo.LRO_SSLCACERT, "/etc/ca.pem")

    h.progressdata = "i2c-tools-eepromer"
    h.download("packages/i2c-tools-eepromer-24-1.x86_64.rpm")
