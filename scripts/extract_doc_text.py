#!/usr/bin/env python3
"""Extract readable text from legacy binary .doc (MS-DOC) files, stdlib only."""
import struct
import sys


class CFB:
    def __init__(self, data):
        self.data = data
        if data[:8] != b"\xd0\xcf\x11\xe0\xa1\xb1\x1a\xe1":
            raise ValueError("not an OLE2 compound file")
        self._byte_order = struct.unpack_from("<H", data, 28)[0]
        self._sector_shift = struct.unpack_from("<H", data, 30)[0]
        self._mini_shift = struct.unpack_from("<H", data, 32)[0]
        self._dir_start = struct.unpack_from("<I", data, 48)[0]
        self._mini_cutoff = struct.unpack_from("<I", data, 56)[0]
        self._first_minifat = struct.unpack_from("<I", data, 60)[0]
        self._num_minifat = struct.unpack_from("<I", data, 64)[0]
        if self._byte_order != 0xFFFE:
            raise ValueError("big-endian CFB not supported")
        self.sector_size = 1 << self._sector_shift
        self.mini_sector_size = 1 << self._mini_shift
        self._fat = self._read_fat()
        self._mini_fat = None
        self._dir = self._read_directory()
        self._mini_stream = None

    def _sector(self, n):
        return self.data[(n + 1) * self.sector_size:(n + 2) * self.sector_size]

    def _read_fat(self):
        # DIFAT lives at sector 0
        fat = []
        difat = list(struct.unpack_from("<109I", self.data, 76))
        entries_per_sector = self.sector_size // 4
        for e in difat:
            if e <= 0xFFFFFFFA:  # valid FAT sector number
                raw = self._sector(e)
                fat.extend(
                    struct.unpack_from("<%dI" % entries_per_sector, raw))
        return fat

    def _chain(self, start, table):
        chain = []
        n = start
        guard = 0
        while n < 0xFFFFFFFA and guard < 100000:
            chain.append(n)
            if n >= len(table):
                break
            n = table[n]
            guard += 1
        return chain

    def _read_directory(self):
        sectors = self._chain(self._dir_start, self._fat)
        raw = b"".join(self._sector(s) for s in sectors)
        dirs = []
        for i in range(0, len(raw), 128):
            name = raw[i:i + 64]
            nlen = struct.unpack_from("<H", raw, i + 64)[0]
            name = name[: nlen - 2].decode("utf-16-le", "replace") if nlen >= 2 else ""
            (obj_type, _color, _left, _right, _child,
             _clsid, _state, _ctime, _mtime, start, size) = \
                struct.unpack_from("<BBIII16sIQQIQ", raw, i + 66)
            dirs.append({"name": name, "type": obj_type,
                         "start": start, "size": size})
        return dirs

    def stream(self, name):
        d = next((x for x in self._dir if x["name"] == name), None)
        if d is None:
            return None
        if d["size"] < self._mini_cutoff:
            if self._mini_stream is None:
                root = next(x for x in self._dir if x["type"] == 5)
                self._mini_stream = self.read_raw(root["start"], root["size"])
            if self._mini_fat is None:
                fat_raw = b"".join(
                    self._sector(s)
                    for s in self._chain(self._first_minifat, self._fat))
                self._mini_fat = [
                    struct.unpack_from("<I", fat_raw, i)[0]
                    for i in range(0, len(fat_raw) - 3, 4)]
            chain = self._chain(d["start"], self._mini_fat)
            return b"".join(
                self._mini_stream[s * self.mini_sector_size:
                                  (s + 1) * self.mini_sector_size]
                for s in chain)[: d["size"]]
        return self.read_raw(d["start"], d["size"])

    def read_raw(self, start, size):
        sectors = self._chain(start, self._fat)
        raw = b"".join(self._sector(s) for s in sectors)
        return raw[:size]


def decode_piece(data, compressed):
    if not compressed:
        return data.decode("utf-16-le", "replace")
    out = bytearray()
    i = 0
    n = len(data)
    while i < n:
        b = data[i]
        if b < 0x80:
            out.append(b)
            i += 1
        else:
            if i + 1 < n:
                out += ((b & 0x7F) << 8 | data[i + 1]).to_bytes(2, "little")
            i += 2
    return out.decode("utf-16-le", "replace")


def extract_text(doc_path):
    with open(doc_path, "rb") as f:
        data = f.read()
    cfb = CFB(data)
    word = cfb.stream("WordDocument")
    if word is None:
        raise ValueError("WordDocument stream not found")
    flags = struct.unpack_from("<H", word, 0x0A)[0]
    encrypted = bool(flags & 0x0100)
    obfuscated = bool(flags & 0x8000)
    tbl_name = "1Table" if (flags & 0x0200) else "0Table"
    table = cfb.stream(tbl_name)
    if table is None:
        table = cfb.stream("1Table") or cfb.stream("0Table") or b""
    fc_min = struct.unpack_from("<I", word, 0x18)[0]
    fc_mac = struct.unpack_from("<I", word, 0x1C)[0]
    fc_clx = struct.unpack_from("<I", word, 0x132)[0]
    lcb_clx = struct.unpack_from("<I", word, 0x136)[0]
    if fc_clx + lcb_clx > len(table):
        raise ValueError("CLX out of table bounds: %d+%d > %d"
                         % (fc_clx, lcb_clx, len(table)))
    clx = table[fc_clx: fc_clx + lcb_clx]
    # skip prcs (0x01 chunks)
    i = 0
    while i < len(clx) and clx[i] == 0x01:
        cb = struct.unpack_from("<H", clx, i + 1)[0]
        i += 3 + cb
    if i >= len(clx) or clx[i] != 0x02:
        raise ValueError("no Pcdt found at CLX offset %d" % i)
    lcb = struct.unpack_from("<I", clx, i + 1)[0]
    plcpcd = clx[i + 5: i + 5 + lcb]
    n = (len(plcpcd) - 4) // 12
    text = []
    for k in range(n):
        pcd = plcpcd[4 + n * 4 + k * 8: 4 + n * 4 + k * 8 + 8]
        cp_end = struct.unpack_from("<I", plcpcd, 4 + (k + 1) * 4)[0]
        cp_start = struct.unpack_from("<I", plcpcd, 4 + k * 4)[0]
        pcd_flags = struct.unpack_from("<H", pcd, 0)[0]
        fc = struct.unpack_from("<I", pcd, 2)[0]
        f_compressed = bool(fc & 0x40000000)
        offset = (fc & 0x3FFFFFFF) // 2 if f_compressed else fc
        char_count = cp_end - cp_start
        raw = word[offset: offset + char_count * (1 if f_compressed else 2)]
        piece = decode_piece(raw, f_compressed)
        if obfuscated and encrypted is False:
            # XOR obfuscation with default key seed (rarely used in templates)
            pass
        text.append(piece)
    full = "".join(text)
    if encrypted:
        print("[warn] document stream is RC4-encrypted; raw text unavailable")
        return ""
    return full


def main():
    path = sys.argv[1]
    t = extract_text(path)
    # normalize control chars to readable markers
    t = t.replace("\r", "\n").replace("\x07", "|CELL|").replace("\x0b", "\n")
    t = t.replace("\x0c", "\n").replace("\x1e", "|FIELD|")
    print(t)


if __name__ == "__main__":
    main()
