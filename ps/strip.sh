git clean -x -f -d -e tests/testrun/ || exit 1
for X in $(git rev-list --reverse $1..$2); do
	git checkout $X -- . || exit 1
	git ls-files | grep -v -e '\.png' -e '\.dia' -e '\.gif' -e '\.bin' | xargs sed -i -e 's/[[:space:]]*$//'
	git add . || exit 1
	git commit --reuse-message=$X || exit 1
done
