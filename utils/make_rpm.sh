#!/bin/bash

PACKAGE="librepo"
RPMBUILD_DIR="${HOME}/rpmbuild/"
BUILD_DIR="$RPMBUILD_DIR/BUILD"
GITREV=`git rev-parse --short HEAD`
PREFIX=""   # Root project dir
MY_DIR=`dirname "$0"`
SRPM_ONLY=0

USAGE_STRING="Usage: `basename "$0"` <root_project_dir> [-s|--srpm-only] [--rpmbuild-options OPTIONS]"

## Arguments sanity check
if [ $# -lt "1"  -o $# -gt "4" ]
then
    echo $USAGE_STRING
    exit 1
fi

## Argument parser
while [[ $# > 0 ]]
do
key="$1"
case $key in
    -h|--help)
        echo $USAGE_STRING
        exit 0
        ;;
    -s|--srpm-only)
        SRPM_ONLY=1
        ;;
    --rpmbuild-options)
        BUILD_OPTS="$2"
        shift # past argument
        ;;
    *)
        # unknown option
        PREFIX=$1
        ;;
esac
shift # past argument or value
done


if [ ! -d "$RPMBUILD_DIR" ]; then
    echo "rpmbuild dir $RPMBUILD_DIR doesn't exist!"
    echo "init rpmbuild dir with command: rpmdev-setuptree"
    echo "(Hint: Package group @development-tools and package fedora-packager)"
    exit 1
fi

echo "Generating rpm for $GITREV"

echo "Cleaning $BUILD_DIR"
rm -rf "$BUILD_DIR"
echo "Removing $RPMBUILD_DIR/$PACKAGE.spec"
rm -f "$RPMBUILD_DIR/$PACKAGE.spec"

echo "> Making tarball .."
TARBALL=$("$MY_DIR/make_tarball.sh" "$GITREV" | grep ".tar.gz")
if [ ! $? == "0" ]; then
    echo "Error while making tarball"
    exit 1
fi
echo "Tarball done"

echo "> Copying tarball and .spec file into the $RPMBUILD_DIR .."
cp "$PREFIX/$TARBALL" "$RPMBUILD_DIR/SOURCES/"
if [ ! $? == "0" ]; then
    echo "Error while: cp $PREFIX/$TARBALL $RPMBUILD_DIR/SOURCES/"
    exit 1
fi

sed -i "s/%{\!?gitrev: %global gitrev .*/%{\!?gitrev: %global gitrev $GITREV}/g" "$PREFIX/$PACKAGE.spec"
cp "$PREFIX/$PACKAGE.spec" "$RPMBUILD_DIR/SPECS/"
if [ ! $? == "0" ]; then
    echo "Error while: cp $PREFIX/$PACKAGE.spec $RPMBUILD_DIR/SPECS/"
    exit 1
fi
echo "Copying done"

RPMBUILD_ACTION="-ba"
if [ "$SRPM_ONLY" -eq "1" ]
then
    RPMBUILD_ACTION="-bs"
    echo "> Building only srpm"
fi

echo "> Starting rpmbuild $PACKAGE.."
rpmbuild $RPMBUILD_ACTION $BUILD_OPTS "$RPMBUILD_DIR/SPECS/$PACKAGE.spec"
if [ ! $? == "0" ]; then
    echo "Error while: rpmbuild -ba $RPMBUILD_DIR/SPECS/$PACKAGE.spec"
    exit 1
fi
echo "rpmbuild done"

echo "> Cleanup .."
rpmbuild --clean "$RPMBUILD_DIR/SPECS/$PACKAGE.spec"
echo "Cleanup done"

echo "> Moving srpm .."
mv --verbose "$RPMBUILD_DIR"/SRPMS/"$PACKAGE"-*.src.rpm "$PREFIX/."
if [ "$SRPM_ONLY" -ne "1" ]
then
    echo "> Moving rpms .."
    mv --verbose "$RPMBUILD_DIR"/RPMS/*/"$PACKAGE"-*.rpm "$PREFIX/."
    mv --verbose "$RPMBUILD_DIR"/RPMS/*/python*-"$PACKAGE"-*.rpm "$PREFIX/."
fi
echo "Moving done"

echo "All done!"
