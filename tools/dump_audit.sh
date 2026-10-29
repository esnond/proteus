#!/bin/sh
# dump_audit: scan a core dump for material that must not be recoverable.
#
# usage: dump_audit.sh DUMP [marker ...]
#
# exit 0 = clean, 1 = a marker was found, 2 = unreadable input.
# markers are fixed strings (journal header by default). the entropy
# summary is informational: high-entropy regions exist for every process
# and only become a finding when a known marker is absent but the raw
# secret is expected there. this is stated so nobody reads the count as
# proof of absence.
set -u
D=${1:-}
[ -n "$D" ] || { echo "usage: dump_audit.sh DUMP [marker ...]"; exit 2; }
[ -r "$D" ] || { echo "dump_audit: cannot read $D"; exit 2; }
shift || true
if [ "$#" -eq 0 ]; then
    set -- "# proteus journal"
fi
leaks=0
for m in "$@"; do
    n=$(grep -a -c -- "$m" "$D" 2>/dev/null || true)
    n=${n:-0}
    echo "marker,$m,$n"
    [ "$n" -eq 0 ] || leaks=$((leaks + 1))
done
ent=$(python3 - "$D" <<'PYEOF'
import sys
data = open(sys.argv[1], "rb").read()
best = 0.0
step = 4096
for off in range(0, max(1, len(data) - 1024), step):
    win = data[off:off + 1024]
    if not win:
        break
    cnt = [0] * 256
    for b in win:
        cnt[b] += 1
    import math
    h = -sum((c / len(win)) * math.log2(c / len(win)) for c in cnt if c)
    best = max(best, h)
print(f"{best:.2f}")
PYEOF
)
echo "entropy_max_bits_per_byte,$ent"
echo "result,$([ "$leaks" -eq 0 ] && echo clean || echo leak)"
[ "$leaks" -eq 0 ]
