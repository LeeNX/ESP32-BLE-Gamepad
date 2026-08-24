#!/usr/bin/env bash
# Bumps library.properties/library.json to a new version, commits, and tags
# it - the local half of the process documented in RELEASE.md. Requires a
# clean working tree so the version-bump commit only ever contains the
# version bump.
set -euo pipefail

usage() {
  cat <<'EOF'
Usage: scripts/release.sh [VERSION] [--push] [--dry-run]

Bump library.properties and library.json to VERSION, commit "Bump version
to VERSION", and create annotated tag vVERSION. Requires a clean git
working tree to start.

VERSION can be given as the first argument or via the RELEASE_VERSION
environment variable (the argument wins if both are set). Format:
MAJOR.MINOR.PATCH, optionally with a semver pre-release suffix, e.g.
0.7.7 or 0.7.7-rc0. A suffixed version tags a GitHub prerelease.

Options:
  --push       Also push the current branch and the new tag to "origin".
               Default: left for you to run yourself (printed at the end).
  --dry-run    Show what would happen without changing anything.
  -h, --help   Show this help.
EOF
}

push=false
dry_run=false
version="${RELEASE_VERSION:-}"

while [ $# -gt 0 ]; do
  case "$1" in
    --push) push=true; shift ;;
    --dry-run) dry_run=true; shift ;;
    -h|--help) usage; exit 0 ;;
    -*)
      echo "error: unknown option: $1" >&2
      usage >&2
      exit 1
      ;;
    *) version="$1"; shift ;;
  esac
done

if [ -z "$version" ]; then
  echo "error: no version given - pass it as an argument or set RELEASE_VERSION" >&2
  usage >&2
  exit 1
fi

if ! [[ "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+(-[0-9A-Za-z.-]+)?$ ]]; then
  echo "error: version '$version' doesn't look like MAJOR.MINOR.PATCH or MAJOR.MINOR.PATCH-prerelease (e.g. 0.7.7 or 0.7.7-rc0)" >&2
  exit 1
fi

repo_root="$(git rev-parse --show-toplevel)"
cd "$repo_root"

if [ -n "$(git status --porcelain)" ]; then
  echo "error: working tree is not clean - commit, stash, or discard changes first:" >&2
  git status --short >&2
  exit 1
fi

tag="v$version"

if git rev-parse "$tag" >/dev/null 2>&1; then
  echo "error: tag $tag already exists" >&2
  exit 1
fi

props_file="library.properties"
json_file="library.json"

current_props_version=$(grep -m1 '^version=' "$props_file" | cut -d= -f2)
current_json_version=$(python3 -c "import json; print(json.load(open('$json_file'))['version'])")

echo "Current version: library.properties=$current_props_version, library.json=$current_json_version"
echo "New version:     $version"

if $dry_run; then
  echo "(dry run) would update $props_file and $json_file, commit, and tag $tag"
  exit 0
fi

sed -i.bak "s/^version=.*/version=$version/" "$props_file" && rm -f "$props_file.bak"

python3 - "$json_file" "$version" <<'PYEOF'
import json
import sys

path, version = sys.argv[1], sys.argv[2]
with open(path) as f:
    data = json.load(f)
data["version"] = version
with open(path, "w") as f:
    json.dump(data, f, indent=2)
    f.write("\n")
PYEOF

git add "$props_file" "$json_file"
git commit -m "Bump version to $version"
git tag -a "$tag" -m "$tag"

branch="$(git rev-parse --abbrev-ref HEAD)"

echo
echo "Committed and tagged $tag locally on $branch."

if $push; then
  git push origin "$branch"
  git push origin "$tag"
  echo "Pushed $branch and $tag."
else
  echo "Next steps:"
  echo "  git push origin $branch"
  echo "  git push origin $tag"
fi
