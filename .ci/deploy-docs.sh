
set -o errexit -o nounset

if test "$TRAVIS_OS_NAME" != "linux" -o "$CC" != "gcc" -o "$TRAVIS_SECURE_ENV_VARS" != "true"; then exit; fi

BRANCH="$(TRAVIS_BRANCH)"
if test "x$BRANCH" != xmaster; then exit; fi

TAG="$(git describe --exact-match --match "[0-9]*" HEAD 2>/dev/null)"

DOCSDIR=build-docs
REVISION=$(git rev-parse --short HEAD)

rm -rf $DOCSDIR || exit
mkdir $DOCSDIR
cd $DOCSDIR

cp ../docs/html/* .

git init
git config user.name "Travis CI"
git config user.email "travis@harfbuzz.org"
git remote add upstream "https://$GH_TOKEN@github.com/$TRAVIS_REPO_SLUG.git"
git fetch upstream
git reset upstream/gh-pages

touch .
git add -A .
git commit -m "Rebuild docs for $REVISION"
git push -q upstream HEAD:gh-pages
