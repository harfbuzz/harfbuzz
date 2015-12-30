
set -o errexit -o nounset

if [ "$TRAVIS_OS_NAME" != "linux" -o "$CC" != "gcc" -o "$TRAVIS_SECURE_ENV_VARS" != "true" ]; then
	exit
fi

BRANCH="$(TRAVIS_BRANCH)"
TAG="$(git describe --exact-match --match "[0-9]*" HEAD 2>/dev/null)"

if [ "x$TAG" == x ]; then
	REVISION=$(git rev-parse --short HEAD)
else
	REVISION=$TAG
fi

DOCSDIR=build-docs

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
