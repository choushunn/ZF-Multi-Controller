"""
生成"电机+相机多控制器"软件的 Windows 多分辨率图标 app.ico。

设计说明：
- 左侧圆形（蓝色）：代表电机 / 转盘
- 右侧圆角矩形（绿色）：代表相机，中间带一个深色镜头圆 + 高光
- 采用 4 倍超采样做抗锯齿
- 仅使用 Python 标准库 struct + zlib 手动构造图像数据，再按 ICO 容器格式打包

格式选择：
- 16/32/48 用 PNG 格式（文件小，Windows Vista+ 原生支持）
- 256x256 用 BMP DIB 格式（ICO 的"原生"格式，最大兼容性，
  即使是只支持 BMP-in-ICO 的旧工具也能识别 256x256）

输出: app.ico，包含 16x16 / 32x32 / 48x48 / 256x256 四个分辨率。
"""

import os
import struct
import zlib


# ---------- 像素生成（共享） ----------

def _in_rounded_rect(nx, ny, x0, y0, x1, y1, r):
    """点 是否在圆角矩形内（坐标为归一化 0-1）。"""
    if not (x0 <= nx <= x1 and y0 <= ny <= y1):
        return False
    if nx < x0 + r and ny < y0 + r:
        return ((nx - (x0 + r)) ** 2 + (ny - (y0 + r)) ** 2) ** 0.5 <= r
    if nx > x1 - r and ny < y0 + r:
        return ((nx - (x1 - r)) ** 2 + (ny - (y0 + r)) ** 2) ** 0.5 <= r
    if nx < x0 + r and ny > y1 - r:
        return ((nx - (x0 + r)) ** 2 + (ny - (y1 - r)) ** 2) ** 0.5 <= r
    if nx > x1 - r and ny > y1 - r:
        return ((nx - (x1 - r)) ** 2 + (ny - (y1 - r)) ** 2) ** 0.5 <= r
    return True


def color_at(nx, ny):
    """根据归一化坐标返回 RGBA（0-255，float 以便超采样后平均）。"""
    r, g, b, a = 0.0, 0.0, 0.0, 0.0  # 默认透明背景

    # ----- 左侧：圆形电机（蓝）-----
    cx, cy, radius = 0.27, 0.50, 0.22
    dist = ((nx - cx) ** 2 + (ny - cy) ** 2) ** 0.5
    if dist <= radius:
        a = 255.0
        r, g, b = 30.0, 136.0, 229.0           # 蓝 #1E88E5
        if dist > radius - 0.035:              # 外圈描深
            r, g, b = 21.0, 101.0, 192.0       # #1565C0
        if dist <= 0.06:                        # 中心深色轴心
            r, g, b = 13.0, 71.0, 161.0         # #0D47A1
        elif dist <= 0.10:                      # 内圈浅高光
            r, g, b = 66.0, 165.0, 245.0        # #42A5F5

    # ----- 右侧：圆角矩形相机（绿）-----
    x0, y0, x1, y1, rr = 0.55, 0.28, 0.95, 0.72, 0.10
    if _in_rounded_rect(nx, ny, x0, y0, x1, y1, rr):
        a = 255.0
        r, g, b = 67.0, 160.0, 71.0            # 绿 #43A047
        if 0.30 <= ny <= 0.36:                  # 顶部反光条
            r, g, b = 102.0, 187.0, 106.0       # #66BB6A
        # 镜头（深绿圆 + 浅高光）
        lcx, lcy, lr = 0.75, 0.50, 0.15
        ldist = ((nx - lcx) ** 2 + (ny - lcy) ** 2) ** 0.5
        if ldist <= lr:
            r, g, b = 27.0, 94.0, 32.0           # 镜头深绿 #1B5E20
        if ldist <= 0.06:                        # 镜头中心反光
            r, g, b = 200.0, 230.0, 255.0        # 浅蓝白
        elif ldist <= 0.10:
            r, g, b = 129.0, 199.0, 132.0       # #81C784

    return r, g, b, a


def make_pixel_func(size, samples=4):
    """返回指定尺寸下做 samples 倍超采样的像素函数。"""
    def pixel_func(x, y, w, h):
        rs = gs = bs = as_ = 0.0
        for sy in range(samples):
            for sx in range(samples):
                nx = (x + (sx + 0.5) / samples) / size
                ny = (y + (sy + 0.5) / samples) / size
                r, g, b, a = color_at(nx, ny)
                rs += r; gs += g; bs += b; as_ += a
        n = samples * samples
        return (int(rs / n + 0.5), int(gs / n + 0.5),
                int(bs / n + 0.5), int(as_ / n + 0.5))
    return pixel_func


# ---------- PNG 构造 ----------

def _png_chunk(chunk_type, data):
    chunk = chunk_type + data
    crc = zlib.crc32(chunk) & 0xFFFFFFFF
    return struct.pack('>I', len(data)) + chunk + struct.pack('>I', crc)


def make_png(width, height, pixel_func):
    """生成 8 位 RGBA PNG。"""
    raw = bytearray()
    for y in range(height):
        raw.append(0)  # filter: None
        for x in range(width):
            r, g, b, a = pixel_func(x, y, width, height)
            raw += bytes((r, g, b, a))

    compressed = zlib.compress(bytes(raw), 9)

    sig = b'\x89PNG\r\n\x1a\n'
    ihdr = _png_chunk(b'IHDR', struct.pack('>IIBBBBB',
                                           width, height, 8, 6, 0, 0, 0))
    idat = _png_chunk(b'IDAT', compressed)
    iend = _png_chunk(b'IEND', b'')
    return sig + ihdr + idat + iend


# ---------- BMP DIB 构造（ICO 内嵌格式） ----------

def make_bmp_dib(width, height, pixel_func):
    """构造 ICO 内嵌的 BMP DIB 数据。

    格式：BITMAPINFOHEADER(40) + XOR 像素(BGRA bottom-up, 4 对齐)
          + AND mask(1bit bottom-up, 4 对齐)。
    注意：ICO 中的 biHeight = 2 * 图标高度（含 XOR + AND 两部分）。
    """
    # 1) 像素数据 (BGRA, bottom-up)
    xor_rows = []
    for y in range(height - 1, -1, -1):  # bottom-up
        row = bytearray()
        for x in range(width):
            r, g, b, a = pixel_func(x, y, width, height)
            row += bytes((b, g, r, a))  # BMP 是 BGRA
        xor_rows.append(bytes(row))
    xor_data = b''.join(xor_rows)

    # 2) AND mask (1 bit/pixel, bottom-up, 每行 4 字节对齐)
    # 对于 alpha=0 的像素置 1（透明），其余置 0。
    and_row_bytes = (width + 7) // 8
    and_row_padded = (and_row_bytes + 3) // 4 * 4
    and_mask = bytearray()
    for y in range(height - 1, -1, -1):  # bottom-up
        row = bytearray(and_row_padded)
        for x in range(width):
            _, _, _, a = pixel_func(x, y, width, height)
            if a == 0:
                row[x // 8] |= (0x80 >> (x % 8))
        and_mask += bytes(row)

    # 3) BITMAPINFOHEADER (40 bytes)
    bi_size_image = len(xor_data) + len(and_mask)
    header = struct.pack('<IiiHHIIiiII',
                         40,                       # biSize
                         width,                    # biWidth
                         2 * height,               # biHeight (XOR + AND)
                         1,                        # biPlanes
                         32,                       # biBitCount
                         0,                        # biCompression (BI_RGB)
                         bi_size_image,            # biSizeImage
                         0,                        # biXPelsPerMeter
                         0,                        # biYPelsPerMeter
                         0,                        # biClrUsed
                         0)                        # biClrImportant

    return header + xor_data + bytes(and_mask)


# ---------- ICO 容器 ----------

def make_ico(specs, output_path):
    """specs: list of (size, format) where format is 'png' or 'bmp'."""
    images = []
    for size, fmt in specs:
        print(f"生成 {size}x{size} ({fmt}) ...")
        pf = make_pixel_func(size, samples=4)
        if fmt == 'png':
            data = make_png(size, size, pf)
        elif fmt == 'bmp':
            data = make_bmp_dib(size, size, pf)
        else:
            raise ValueError(f"未知格式: {fmt}")
        images.append((size, fmt, data))
        print(f"  -> {len(data)} bytes")

    count = len(images)
    header = struct.pack('<HHH', 0, 1, count)  # ICONDIR

    entries = bytearray()
    offset = 6 + count * 16
    for size, fmt, data in images:
        w = size if size < 256 else 0
        h = size if size < 256 else 0
        entry = struct.pack('<BBBBHHII',
                            w, h, 0, 0, 1, 32, len(data), offset)
        entries += entry
        offset += len(data)

    ico_data = header + bytes(entries)
    for _, _, data in images:
        ico_data += data

    os.makedirs(os.path.dirname(output_path) or '.', exist_ok=True)
    with open(output_path, 'wb') as f:
        f.write(ico_data)

    print(f"\nICO 已写入: {output_path}")
    print(f"总大小: {len(ico_data)} bytes ({len(ico_data)/1024:.1f} KB)")


if __name__ == '__main__':
    OUT = r'd:\Multi-Controller(1)\src\resources\icons\app.ico'
    # 256 用 BMP 以保证旧工具兼容；小尺寸用 PNG 节省空间
    make_ico([(16, 'png'), (32, 'png'), (48, 'png'), (256, 'bmp')], OUT)

    if os.path.exists(OUT):
        sz = os.path.getsize(OUT)
        print(f"验证: 文件存在, {sz} bytes ({sz/1024:.1f} KB)")
        if sz < 1024:
            print("警告: 文件偏小，可能存在问题")
        else:
            print("验证通过: 文件大小合理")
    else:
        print("错误: 文件未创建")
