PACKAGE="librepo"
TARGET_DIR="."

MAJOR=$(grep MAJOR VERSION.cmake | awk -F\" '{print $2}')
MINOR=$(grep MINOR VERSION.cmake | awk -F\" '{print $2}')
PATCH=$(grep PATCH VERSION.cmake | awk -F\" '{print $2}')

NV="$PACKAGE"-"$MAJOR.$MINOR.$PATCH"
FN="$NV.tar.gz"

if [ "$#" -eq "0" ]; then
    GITREV=`git rev-parse --short HEAD`
else
    GITREV="$1"
fi

echo "Generate tarball for revision: $GITREV"

git archive "${GITREV}" --prefix="$PACKAGE-$NV"/ | gzip > "$TARGET_DIR"/"$FN"
echo "$FN"
