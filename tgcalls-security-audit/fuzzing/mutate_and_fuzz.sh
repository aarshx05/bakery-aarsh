#!/bin/bash
# Simple mutation-based fuzzer: takes seed corpus, mutates, feeds to harness
# Usage: ./mutate_and_fuzz.sh <harness> <corpus_dir> <iterations>
set +e

HARNESS="$1"
CORPUS_DIR="$2"
ITERS="${3:-1000}"
TMPDIR=$(mktemp -d)
trap "rm -rf $TMPDIR" EXIT

echo "[*] Fuzzing $HARNESS with $ITERS mutations from $CORPUS_DIR"

SEEDS=($(ls "$CORPUS_DIR"/*.bin 2>/dev/null))
if [ ${#SEEDS[@]} -eq 0 ]; then
    echo "[!] No seed files found"
    exit 1
fi

CRASHES=0
TOTAL=0

for i in $(seq 1 $ITERS); do
    # Pick random seed
    SEED=${SEEDS[$((RANDOM % ${#SEEDS[@]}))]}
    MUTATED="$TMPDIR/mut_$i.bin"

    # Random mutation strategy
    STRATEGY=$((RANDOM % 6))
    case $STRATEGY in
        0) # Bit flip
            cp "$SEED" "$MUTATED"
            FSIZE=$(stat -c%s "$MUTATED" 2>/dev/null || echo 0)
            if [ "$FSIZE" -gt 0 ]; then
                POS=$((RANDOM % FSIZE))
                printf "\\x$(printf '%02x' $((RANDOM % 256)))" | dd of="$MUTATED" bs=1 seek=$POS conv=notrunc 2>/dev/null
            fi
            ;;
        1) # Truncate
            FSIZE=$(stat -c%s "$SEED" 2>/dev/null || echo 1)
            if [ "$FSIZE" -gt 0 ] 2>/dev/null; then
                NEWSIZE=$((RANDOM % (FSIZE + 1)))
                head -c "$NEWSIZE" "$SEED" > "$MUTATED"
            else
                cp "$SEED" "$MUTATED"
            fi
            ;;
        2) # Append random bytes
            cp "$SEED" "$MUTATED"
            dd if=/dev/urandom bs=$((1 + RANDOM % 64)) count=1 >> "$MUTATED" 2>/dev/null
            ;;
        3) # Insert random bytes at random position
            FSIZE=$(stat -c%s "$SEED" 2>/dev/null || echo 0)
            POS=$((RANDOM % (FSIZE + 1)))
            head -c "$POS" "$SEED" > "$MUTATED"
            dd if=/dev/urandom bs=$((1 + RANDOM % 16)) count=1 >> "$MUTATED" 2>/dev/null
            tail -c "+$((POS + 1))" "$SEED" >> "$MUTATED"
            ;;
        4) # All zeros or all 0xFF
            FSIZE=$(stat -c%s "$SEED" 2>/dev/null || echo 16)
            if [ $((RANDOM % 2)) -eq 0 ]; then
                dd if=/dev/zero bs="$FSIZE" count=1 > "$MUTATED" 2>/dev/null
            else
                python3 -c "import sys; sys.stdout.buffer.write(b'\\xff' * $FSIZE)" > "$MUTATED"
            fi
            ;;
        5) # Random data with same size
            FSIZE=$(stat -c%s "$SEED" 2>/dev/null || echo 32)
            dd if=/dev/urandom bs="$FSIZE" count=1 > "$MUTATED" 2>/dev/null
            ;;
    esac

    # Skip if mutation produced nothing
    [ ! -s "$MUTATED" ] && continue

    TOTAL=$((TOTAL + 1))

    "$HARNESS" "$MUTATED" >/dev/null 2>&1
    RET=$?
    if [ $RET -ne 0 ]; then
        CRASHES=$((CRASHES + 1))
        cp "$MUTATED" "$CORPUS_DIR/crash_${i}.bin"
        echo "[!!] CRASH on iteration $i (strategy=$STRATEGY, exit=$RET), saved to crash_${i}.bin"
    fi
done

echo "[*] Done: $TOTAL mutations, $CRASHES crashes"
