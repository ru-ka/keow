#!/bin/sh
#
# make a release of keow
# (currently does not run IN keow because we don't currently
#  support mounting other host directories within the tree)
# using cygwin currently.

# use zip so windows users can access easily

rm -f keow.zip
[ ! -d keow ] && mkdir keow
cp Release/*.exe Release/*.dll keow
zip -r9 keow.zip keow

rm -f bootstrap-linux.zip
(cd bootstrap; find linux | grep -v '/CVS' | zip -9y ../bootstrap-linux.zip -@ )
