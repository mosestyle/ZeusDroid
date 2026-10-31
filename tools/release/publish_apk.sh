#!/usr/bin/env bash
# Publishes dist/ZeusDroid-debug-arm64-v8a.apk as a GitHub release (mosestyle/ZeusDroid).
# Releases cannot be created from the agent session, so the APK goes to the "apk" branch;
# its workflow (.github/workflows/release-apk.yml) creates release v<version> with the APK,
# tagged at the current main commit. Notes: the version's section of docs/RELEASE_NOTES.md.
# usage: tools/release/publish_apk.sh            (run after pushing main)
#        REPLACES="v0.1.25-dev" tools/release/publish_apk.sh   (also delete those releases)
# Version names are plain numbers (0.1.26), the asset is ZeusDroid-<version>.apk.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"
APK=dist/ZeusDroid-debug-arm64-v8a.apk
VER=$(aapt2 dump badging "$APK" 2>/dev/null | sed -n "s/.*versionName='\([^']*\)'.*/\1/p")
[ -n "$VER" ] || { echo "cannot read versionName from $APK"; exit 1; }
SHA=$(git rev-parse HEAD)
git fetch -q origin main
git merge-base --is-ancestor "$SHA" origin/main || { echo "push main first ($SHA not on origin/main)"; exit 1; }
WT="$ROOT/build/apk-branch"
rm -rf "$WT"; git worktree prune
if git ls-remote --exit-code --heads origin apk >/dev/null; then
  git fetch -q origin apk:refs/remotes/origin/apk
  git worktree add -q -B apk "$WT" origin/apk
else
  git worktree add -q --detach "$WT"
  (cd "$WT" && git checkout -q --orphan apk && git rm -rq --cached . && git clean -fdxq)
fi
cd "$WT"
mkdir -p .github/workflows release
cp "$ROOT/tools/release/release-apk.yml" .github/workflows/release-apk.yml
rm -f release/*.apk
cp "$ROOT/$APK" "release/ZeusDroid-$VER.apk"
# Releases this one replaces (e.g. the same version with a "-dev" name), deleted by the workflow.
printf "%s\n" ${REPLACES:-} > release/replaces.txt
echo "$VER" > release/version.txt
echo "$SHA" > release/commit.txt
python3 - "$ROOT/docs/RELEASE_NOTES.md" "$VER" "$(sha256sum "$ROOT/$APK" | cut -d' ' -f1)" > release/notes.md <<'PY'
import re, sys
notes, ver, h = sys.argv[1:4]
s = open(notes).read()
m = re.search(r'^## ' + re.escape(ver) + r'\n(.*?)(?=^## |\Z)', s, re.S | re.M)
print("Native arm64 Android build of Zeus: Master of Olympus + Poseidon on the eZeus engine.\n")
print("**You need your own copy of the game** (e.g. the GOG edition): install the APK, tap "
      "*Import game folder…*, pick your \"Zeus and Poseidon\" folder, then *Start game*. "
      "The first start prepares the graphics from your game files (about a minute, once). "
      "No game files are included.\n")
print("### Changes in " + ver)
print((m.group(1).strip() if m else "See docs/RELEASE_NOTES.md.") + "\n")
print("Earlier versions: docs/RELEASE_NOTES.md. APK sha256: `" + h + "`")
PY
cat > README.md <<'MD'
# APK branch

Only release files: each push creates the GitHub release with the APK
(`.github/workflows/release-apk.yml`). The source code is on `main`.
MD
git add -A
git commit -q -m "APK $VER (source $SHA)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
git push -q origin apk
echo "pushed APK $VER to branch apk; the workflow creates release v$VER"
