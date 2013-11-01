# downloads latest tarball, unpacks it, then commits
# changes to ../kyotocabinet dir

for i in {32..76}; do

VERSION="1.2.${i}"
#VERSION="1.2.16"

VER_MERGED=$(git tag | grep -c "$VERSION")

if [ ! "$VER_MERGED" = "0" ]; then
  echo "Version $VERSION appears to have already been merged"
  exit 1
fi

# fetch new tarball
wget http://fallabs.com/kyotocabinet/pkg/kyotocabinet-${VERSION}.tar.gz
#wget http://fallabs.com/kyotocabinet/pkg/old/kyotocabinet-${VERSION}.tar.gz
#wget http://lil.fr.distfiles.macports.org/kyotocabinet/1.2.16_3/kyotocabinet-${VERSION}.tar.gz
#wget http://lil.fr.distfiles.macports.org/kyotocabinet/${VERSION}_0/kyotocabinet-${VERSION}.tar.gz

# unpackage
if [ ! -f kyotocabinet-${VERSION}.tar.gz ]; then
  echo "File kyotocabinet-${VERSION}.tar.gz missing (download failed?)"
  exit 1
fi

tar zxvf kyotocabinet-${VERSION}.tar.gz
rm -f kyotocabinet-${VERSION}.tar.gz
#rm -Rf kyotocabinet-${VERSION}/man
rm -Rf kyotocabinet-${VERSION}/doc/*
touch kyotocabinet-${VERSION}/doc/IntentionallyEmpty.txt

rm -Rf upstream/*
mv kyotocabinet-${VERSION}/* upstream/
rm -Rf kyotocabinet-${VERSION}

# tell git to add any new files as well as include tracked files that were deleted
git add -u
git add -A *
git commit -m "Merged from upstream v${VERSION}"
git tag upstream-version-${VERSION}
#git push -u --tags origin master

done
