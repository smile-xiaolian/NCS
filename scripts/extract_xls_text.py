#!/usr/bin/env python3
"""Dump cell text from legacy BIFF8 .xls (stdlib only)."""
import struct
import sys

sys.path.insert(0, "scripts")
from extract_doc_text import CFB  # reuse CFB parser


def _rk_value(rk):
    div = 100.0 if (rk & 0x01) else 1.0
    if rk & 0x02:  # 30-bit signed integer
        val = struct.unpack("<i", struct.pack("<I", rk & 0xFFFFFFFC))[0]
    else:  # 64-bit float (top 34 bits)
        val = struct.unpack("<d", struct.pack("<Q", (rk >> 2) << 34))[0]
    return val / div


def parse_biff(stream):
    sst = []
    rows = {}
    i = 0
    n = len(stream)
    while i + 4 <= n:
        rec_id, rec_len = struct.unpack_from("<HH", stream, i)
        body = stream[i + 4: i + 4 + rec_len]
        if rec_id == 0x00FC:  # SST（可能被 CONTINUE 拆断，先拼齐再解）
            buf = bytearray(body)
            while True:
                nid, nlen = struct.unpack_from("<HH", stream, i + 4 + rec_len)
                if nid != 0x003C:
                    break
                buf += stream[i + 8 + rec_len: i + 8 + rec_len + nlen]
                i += 4 + nlen
            _total, unique = struct.unpack_from("<II", buf, 0)
            p = 8
            for _ in range(unique):
                cch, flags = struct.unpack_from("<HB", buf, p)
                p += 3
                if flags & 0x01:
                    s = bytes(buf[p:p + cch * 2]).decode("utf-16-le", "replace")
                    p += cch * 2
                else:
                    s = bytes(buf[p:p + cch]).decode("gb18030", "replace")
                    p += cch
                if flags & 0x08:  # 富文本运行数
                    p += 2 + 4 * struct.unpack_from("<H", buf, p)[0]
                if flags & 0x04:  # 扩展信息
                    p += 4
                sst.append(s)
        elif rec_id == 0x00FD:  # LABELSST
            row, col, _xf, isst = struct.unpack_from("<HHHI", body, 0)
            rows.setdefault(row, {})[col] = sst[isst] if isst < len(sst) else f"<sst#{isst}>"
        elif rec_id == 0x0204:  # LABEL
            row, col, _xf, cch, grbit = struct.unpack_from("<HHHHB", body, 0)
            p = 9
            if grbit & 0x01:
                rows.setdefault(row, {})[col] = body[p:p + cch * 2].decode("utf-16-le", "replace")
            else:
                rows.setdefault(row, {})[col] = body[p:p + cch].decode("gb18030", "replace")
        elif rec_id == 0x0203:  # NUMBER
            row, col, _xf, val = struct.unpack_from("<HHHd", body, 0)
            rows.setdefault(row, {})[col] = str(round(val, 6)).rstrip("0").rstrip(".")
        elif rec_id == 0x027E:  # RK
            row, col, _xf, rk = struct.unpack_from("<HHHI", body, 0)
            rows.setdefault(row, {})[col] = str(round(_rk_value(rk), 6)).rstrip("0").rstrip(".")
        elif rec_id == 0x00BD:  # MULRK
            row, col_first = struct.unpack_from("<HH", body, 0)
            last, = struct.unpack_from("<H", body, rec_len - 2)
            p = 4
            for col in range(col_first, last + 1):
                _xf, rk = struct.unpack_from("<HI", body, p)
                p += 6
                rows.setdefault(row, {})[col] = str(round(_rk_value(rk), 6)).rstrip("0").rstrip(".")
        i += 4 + rec_len
    return rows, sst


def main():
    with open(sys.argv[1], "rb") as f:
        data = f.read()
    cfb = CFB(data)
    wb = cfb.stream("Workbook") or cfb.stream("Book")
    if wb is None:
        print("Workbook stream not found"); return
    rows, _sst = parse_biff(wb)
    for r in sorted(rows):
        cells = rows[r]
        line = " | ".join(f"[{c}] {cells[c]}" for c in sorted(cells))
        print(line)


if __name__ == "__main__":
    main()
