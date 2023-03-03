#!/bin/sh

# This (re)generates the OpenPGP signing key and signature.

USERID="repository signing key (test key) <noone@example.org>"

export GNUPGHOME=`mktemp -d`

gpg --batch --passphrase '' --quick-generate-key "$USERID" rsa2048 cert,sign never
PRIMARY_KEYID=`gpg -k --with-colons | grep pub | cut -d: -f5`
PRIMARY_FP=`gpg -k --with-colons | grep fpr | cut -d: -f10`
PRIMARY_CREATION=`gpg -k --with-colons | grep pub | cut -d: -f6`

gpg --batch --passphrase '' --quick-add-key "$PRIMARY_FP" rsa2048 encrypt never
SUBKEY_KEYID=`gpg -k --with-colons | grep sub | cut -d: -f5`
SUBKEY_FP=`gpg -k --with-colons | grep fpr | tail -n1 | cut -d: -f10`
SUBKEY_CREATION=`gpg -k --with-colons | grep sub | cut -d: -f6`

gpg --batch --passphrase '' --armor --export-secret-key "$USERID" > repomd.xml.key.secret
gpg --batch --armor --export "$USERID" > repomd.xml.key
mv repomd.xml.asc repomd.xml.asc~
gpg --batch --armor --sign --detach -o repomd.xml.asc repomd.xml

echo "Adjust test_gpg.c with these values:"
echo
echo "UserID: $USERID"
echo "Primary KeyID: $PRIMARY_KEYID"
echo "Primary Fingerprint: $PRIMARY_FP"
echo "Primary creation time: $PRIMARY_CREATION"
echo "Subkey KeyID: $SUBKEY_KEYID"
echo "Subkey Fingerprint: $SUBKEY_FP"
echo "Subkey creation time: $SUBKEY_CREATION"
echo
echo "Raw key export:"
echo
gpg --armor --export "$PRIMARY_FP" | sed -e 's/^/"/' -e 's/$/\\n"/'
