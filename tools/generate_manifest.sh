#!/bin/sh
# generate results/manifest.txt for a release, per the reproducibility gate.
#
# hashes only tracked artifacts (git ls-files), so the manifest is a function
# of the commit and not of whatever happens to be lying in the working tree.
# the binary is hashed explicitly because it is build output, not tracked.
#
# usage: tools/generate_manifest.sh [output]

set -eu

out=${1:-results/manifest.txt}
commit=$(git rev-parse HEAD)
short=$(git rev-parse --short HEAD)
now=$(date -u +%Y-%m-%dT%H:%M:%SZ)
cc_bin=${CC:-cc}
cc_ver=$("$cc_bin" --version 2>/dev/null | head -1 || echo unknown)
cflags=$(sed -n 's/^CFLAGS[[:space:]]*[:?+]*=[[:space:]]*//p' Makefile | head -1)

mkdir -p "$(dirname "$out")"
{
    echo "# proteus release manifest"
    echo "# generated: $now"
    echo "commit: $commit"
    echo "cc: $cc_ver"
    echo "cflags: ${cflags:-unknown}"
    if [ -f ./proteusd ]; then
        printf 'binary: '; sha256sum ./proteusd | cut -d' ' -f1
    else
        echo "binary: absent"
    fi
    echo
    echo "# tracked data, docs and tools"
    LC_ALL=C git ls-files -z -- results docs tools tests \
        | LC_ALL=C sort -z | xargs -0 -r sha256sum
} > "$out"

echo "$out: $(grep -c '^[0-9a-f]\{64\}' "$out") hashed files, commit $short"
