#!/bin/sh
#
#  Script to make a Keow bootstrap image.
#  Builds debian based images (eg ubuntu)
#

distribution=`lsb_release -s -i | tr '[:upper:]' '[:lower:]' | tr '[:blank:]' '-'`
codename=`lsb_release -s -c | tr '[:upper:]' '[:lower:]' | tr '[:blank:]' '-'`

archive=bootstrap-$distribution-$codename.zip
tmpdir=tmp.$distribution.$codename.$$


if [ -z "$distribution" -o -z "$codename" ]; then
    echo "Unable to determine distribution name" 1>&2
    exit 1
fi

#check dependancies that may not be installed
for cmd in zip fakeroot fakechroot debootstrap; do
    if [ -z "`which $cmd`" ]; then
        echo "Cannot find $cmd. Please install it" 1>&2
        exit 1
    fi
done

#prepare tmp - and cleanup at exit
trap "rm -rf $tmpdir" 0
mkdir -p "$tmpdir/linux"

# build minimal system
echo "Creating minimal system"
#fakeroot fakechroot debootstrap --arch=i386 --components=main --variant=fakechroot $codename $tmpdir/linux
cp -r u.tmp./* $tmpdir/linux

#remove some stuff we don't need (can be rebuilt)
echo "Pruning unnessesary files"
rm -f $tmpdir/linux/var/cache/apt/archives/*.deb
rm -f $tmpdir/linux/var/lib/apt/lists/*

#zip up the results
echo "Creating ZIP of bootstrap files"
ZIP="`pwd`/$archive"
(cd $tmpdir; zip -r -y -q "$ZIP" linux)

#see result
du -sh $archive

echo "Cleanup."
rm -rf $tmpdir
echo "Done."
