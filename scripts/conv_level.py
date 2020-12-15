import sys
import configparser
import math
import struct

from common import *
from posmap import *

# --- .lvl format v3 ---
# level_t
# part table
# bboxes
# parts {
#    part_t
#    tris
#    quads
#    pos
#    uvs
#    normals
#    col cells
#    col heap
# } * header.num_parts

HEADER_LEN = 12
PART_HEADER_LEN = 64
BBOX_LEN = 24
COLGRID_CELL_SIZE = 6
TRI_FACE_SIZE = 16
QUAD_FACE_SIZE = 20

NORMAL_SCALE = 0x3FF
MAX_PARTS = 16

cfg = configparser.ConfigParser()
cfg.read(sys.argv[1] + ".cfg")

colshift = int(cfg["default"]["colshift"])
print("colgrid cell size: {}".format(1 << colshift))

obj_file = sys.argv[1] + ".obj"
lvl_file = sys.argv[1] + ".lvl"

posmap = PosMap(cfg["default"]["posmap"])
posmap.set_offset(int(cfg["default"]["tpage_x"]), int(cfg["default"]["tpage_y"]))

parts = [None] * MAX_PARTS
bboxes = [None] * MAX_PARTS

class BoundingBox:
    
    def __init__(self, bmin, bmax):
        self.bmin = bmin
        self.bmax = bmax
    
    def serialize(self):
        out = bytearray()
        for i in self.bmin:
            out += to_long(i)
        for i in self.bmax:
            out += to_long(i)
        return out

class Part: # could also be a bounding box 

    def __init__(self, num, is_bbox):
        self.num = num

        self.positions = []
        self.uvs = []
        self.normals = []

        self.tris = []
        self.quads = []

        self.is_bbox = is_bbox
    
    def to_bounding_box(self):
        # _min = [ 32767] * 3
        # _max = [-32767] * 3

        _min = [ 99999999] * 3
        _max = [-99999999] * 3

        for p in self.positions:
            for c in range(3):
                if p[c] < _min[c]:
                    _min[c] = p[c]
                if p[c] > _max[c]:
                    _max[c] = p[c]
        
        return BoundingBox(_min, _max)
    
    def calculate_center(self):
        bbox = self.to_bounding_box()
        x = bbox.bmin[0] + bbox.bmax[0]
        y = bbox.bmin[1] + bbox.bmax[1]
        z = bbox.bmin[2] + bbox.bmax[2]
        self.offset = [-int(x / 2), -int(y / 2), -int(z / 2)]

    def translate_all_vertices(self):
        self.calculate_center()

        for i in self.positions:
            for p in range(3):
                v = int(i[p] + self.offset[p])
                #print(str(v))

                if abs(v) >= 0xFFFF:
                    print("ERROR: vertex in part {} goes out of short bounds!".format(self.num))

                i[p] = v
 
    def serialize(self, bbox: BoundingBox): # for level parts only
        print("serializing part {}".format(self.num))

        self.translate_all_vertices()
        print("offsetting vertices by {}".format(self.offset))

        # generate colgrid here
        cg_w = int((bbox.bmax[0] - bbox.bmin[0]) / (1 << colshift)) + 1
        cg_h = int((bbox.bmax[2] - bbox.bmin[2]) / (1 << colshift)) + 1
        print("colgrid size: " + str(cg_w) + ", " + str(cg_h))
        colgrid = ColGrid(bbox.bmin[0] + self.offset[0], bbox.bmin[2] + self.offset[2], cg_w, cg_h, colshift)

        # for t in range(len(self.tris)):
        #     colgrid.add_tri(self.tris[t], t)
        
        # populate colgrid
        for q in range(len(self.quads)):
            norm_y = self.normals[self.quads[q].norm_i][1]
            if norm_y < -306:
                colgrid.add_quad(self.quads[q], q, self)

        colheap_data = colgrid.serialize_heap()
        colgrid_data = colgrid.serialize_index_grid()

        # begin serialization
        # 1. struct part_t
        # 2. tris
        # 3. quads
        # 4. positions
        # 5. uvs
        # 6. normals
        # 7. col grid
        # 8. col heap
        
        assert PART_HEADER_LEN & 3 == 0
        
        # do the body first cause we need the length of it.
        body = bytearray()

        # all the following is short aligned
        # 2. tris
        pad_byte_arr(body)
        o_tris = len(body) + PART_HEADER_LEN
        for i in self.tris:
            body += i.serialize()

        # 3. quads
        pad_byte_arr(body)
        o_quads = len(body) + PART_HEADER_LEN
        for i in self.quads:
            body += i.serialize(self)
        
        # 4. positions
        pad_byte_arr(body)
        o_pos = len(body) + PART_HEADER_LEN
        for i in self.positions:
            # body += to_short(int(i[0] - center[0]))
            # body += to_short(int(i[1] - center[1]))
            # body += to_short(int(i[2] - center[2]))
            body += to_short(i[0])
            body += to_short(i[1])
            body += to_short(i[2])
            body += to_short(0) # pad

        # 5. uvs
        pad_byte_arr(body)
        o_uv = len(body) + PART_HEADER_LEN
        for i in self.uvs:
            for u in i:
                body += to_uchar(u)
        
        # 6. normals
        pad_byte_arr(body)
        o_norm = len(body) + PART_HEADER_LEN
        for i in self.normals:
            for n in i:
                body += to_short(n)
            body += to_short(0) # pad

        # 7. col grid
        pad_byte_arr(body)
        o_colcells = len(body) + PART_HEADER_LEN
        body += colgrid_data

        # 8. col heap
        pad_byte_arr(body)
        o_colheap = len(body) + PART_HEADER_LEN
        body += colheap_data

        # ... and then the head.
        head = bytearray()
        # 1. struct part_t
        byte_size = PART_HEADER_LEN + len(body)
        print("part size in bytes: " + str(byte_size))

        head += to_ushort(byte_size)
        head += to_short(colgrid.x)

        head += to_short(colgrid.z)
        head += to_ushort(colgrid.w)

        head += to_ushort(colgrid.h)
        head += to_uchar(colgrid.shift)
        head += to_uchar(0) # pad0

        head += to_ushort(len(self.tris))
        head += to_ushort(len(self.quads))

        head += to_ushort(len(self.positions))
        head += to_ushort(len(self.uvs))

        head += to_ushort(len(self.normals))
        head += to_ushort(0) # pad1

        head += to_long(self.offset[0])
        head += to_long(self.offset[1])
        head += to_long(self.offset[2])

        head += to_ulong(o_tris)
        head += to_ulong(o_quads)
        head += to_ulong(o_pos)
        head += to_ulong(o_uv)
        head += to_ulong(o_norm)
        head += to_ulong(o_colcells)
        head += to_ulong(o_colheap)
        assert len(head) == PART_HEADER_LEN

        out = head + body
        
        diff = len(out) - byte_size
        if diff != 0:
            print("There was a discrepancy when serializing part {}! Data differs by {} bytes".format(self.num, diff))
        
        if len(out) % 4 != 0:
            print("padding part {} with {} bytes".format(self.num, 4 - len(out) % 4))
        for i in range(4 - len(out) & 3):
            out += to_uchar(0)

        return out

class Face:

    def __init__(self, pos_i, uv_i, norm_i):
        self.pos_i = pos_i
        self.uv_i = uv_i
        self.norm_i = norm_i

        self.brightness = 0

    def serialize(self, part):
        # -- temporary CLUT calculation --
        # average = 0
        # for i in range(4):
        #     average += part.uvs[self.uv_i[i]][0]
        # average /= 4
        # x = int(average / 64)
        # average = 0
        # for i in range(4):
        #     average += part.uvs[self.uv_i[i]][1]
        # average /= 4
        # y = int(average / 64)
        # self.clut = int(x + y * 4)
        # --------------

        uv_x = int(part.uvs[self.uv_i[0]][0] / 4)
        uv_y = int(part.uvs[self.uv_i[0]][1])

        self.clut = posmap.get_clut_code(uv_x, uv_y)

        out = bytearray()

        for i in self.pos_i:
            out += to_ushort(i)
        
        for i in self.uv_i:
            out += to_ushort(i)

        out += to_ushort(self.norm_i)

        out += to_ushort(self.clut)
        #out += to_uchar(self.brightness)

        return out

class ColGridCell:

    def __init__(self):
        self.tris = []
        self.quads = []
    
    def serialize(self, index):
        # heap you dummy

        self.tri_index = 0xFFFF
        self.quad_index = 0xFFFF
        
        ret = bytearray()
        if len(self.tris) != 0:
            for t in self.tris:
                ret += t.to_bytes(2, byteorder="little")
            self.tri_index = index
            index += 2
        
        if len(self.quads) != 0:
            for t in self.quads:
                ret += t.to_bytes(2, byteorder="little")
            self.quad_index = index
            index += 2
        
        return ret

class ColGrid: # use bbox as grid?
    
    def __init__(self, x, z, w, h, shift):
        self.x = x
        self.z = z
        self.w = w
        self.h = h
        self.shift = shift
        self.pad = 0

        self.grid = [None] * self.w * self.h
        for i in range(self.w * self.h):
            self.grid[i] = ColGridCell()

    def serialize(self):
        ret = bytearray()
        
        heap = self.serialize_heap()

        ret += self.serialize_index_grid()

        ret += heap

        return ret
    
    def serialize_index_grid(self):
        ret = bytearray()

        # tris
        for z in range(0, self.h):
            for x in range(0, self.w):
                cell = self.grid[x + z * self.w]

                ret += len(cell.tris).to_bytes(1, byteorder="little")
                ret += len(cell.quads).to_bytes(1, byteorder="little")
                ret += cell.tri_index.to_bytes(2, byteorder="little")
                ret += cell.quad_index.to_bytes(2, byteorder="little")
        
        return ret
    
    def serialize_heap(self):
        ret = bytearray()
        
        for z in range(0, self.h):
            for x in range(0, self.w):
                ret += self.grid[x + z * self.w].serialize(int(len(ret) / 2))

        return ret
    
    def to_cell_index(self, x, z):
        x = (x - self.x) >> self.shift
        z = (z - self.z) >> self.shift
        return (x, z)
    
    def add_tri(self, tri: Face, index):
        EXTRA = 10

        min_x = min(tri.vertices[0].x, min(tri.vertices[1].x, tri.vertices[2].x)) - EXTRA
        min_z = min(tri.vertices[0].z, min(tri.vertices[1].z, tri.vertices[2].z)) - EXTRA

        max_x = max(tri.vertices[0].x, max(tri.vertices[1].x, tri.vertices[2].x)) + EXTRA
        max_z = max(tri.vertices[0].z, max(tri.vertices[1].z, tri.vertices[2].z)) + EXTRA

        min_c = self.to_cell_index(min_x, min_z)
        max_c = self.to_cell_index(max_x, max_z)

        for z in range(min_c[1], max_c[1] + 1):
            for x in range(min_c[0], max_c[0] + 1):
                arr = self.grid[x + z * self.w].tris
                if index not in arr:
                    arr.append(index)

    def add_quad(self, quad: Face, index, part):
        EXTRA = 10

        pos0 = part.positions[quad.pos_i[0]]
        pos1 = part.positions[quad.pos_i[1]]
        pos2 = part.positions[quad.pos_i[2]]
        pos3 = part.positions[quad.pos_i[3]]

        min_x = min(pos0[0], min(pos1[0], min(pos2[0], pos3[0]))) - EXTRA
        min_z = min(pos0[2], min(pos1[2], min(pos2[2], pos3[2]))) - EXTRA

        max_x = max(pos0[0], max(pos1[0], max(pos2[0], pos3[0]))) + EXTRA
        max_z = max(pos0[2], max(pos1[2], max(pos2[2], pos3[2]))) + EXTRA

        min_c = self.to_cell_index(min_x, min_z)
        max_c = self.to_cell_index(max_x, max_z)

        if min_c[0] < 0:
            print("quad outside grid bounds! x < 0")
            exit(1)
        
        if max_c[0] >= self.w:
            print("quad outside grid bounds! x > w")
            exit(1)

        if min_c[1] < 0:
            print("quad outside grid bounds! z < 0")
            exit(1)
        
        if max_c[1] >= self.h:
            print("quad outside grid bounds! z > h")
            exit(1)

        for z in range(min_c[1], max_c[1] + 1):
            for x in range(min_c[0], max_c[0] + 1):
                arr = self.grid[x + z * self.w].quads
                if index not in arr:
                    arr.append(index)

def parse_obj(): # scaling is applied here for now
    f = open(obj_file, "r")

    scale = float(float(cfg["default"]["scale"]))

    part = None

    pos_offset = 0
    uv_offset = 0
    norm_offset = 0

    while True:
        line = f.readline()
        if not line:
            break
        sp = line.split(" ")
        if len(sp) == 0:
            continue
        
        if sp[0] == "o":
            if part is not None:
                pos_offset += len(part.positions)
                uv_offset += len(part.uvs)
                norm_offset += len(part.normals)

            if sp[1][0] == "p":
                n = int(sp[1].split("_")[0][1:])
                part = Part(n, False)
                parts[n] = part
            elif sp[1][0] == "b":
                n = int(sp[1].split("_")[0][1:])
                part = Part(n, True)
                bboxes[n] = part
            else:
                print("ERROR: unknown obj object \"" + sp[1] + "\"")
                exit(1)
        elif sp[0] == "v":
            x = int(float(sp[1]) * scale)
            y = int(float(sp[2]) * scale)
            z = int(float(sp[3]) * scale)
            part.positions.append([x, y, z])
        elif sp[0] == "vt":
            u = int(float(sp[1]) * 255)
            v = 255 - int(float(sp[2]) * 255)
            part.uvs.append([u, v])
        elif sp[0] == "vn":
            x = int(float(sp[1]) * NORMAL_SCALE)
            y = int(float(sp[2]) * NORMAL_SCALE)
            z = int(float(sp[3]) * NORMAL_SCALE)
            part.normals.append([x, y, z])
        elif sp[0] == "f":
            n = len(sp) - 1

            if n != 3 and n != 4:
                print("ERROR: non-triangle or quad face found, ignoring")
                continue

            norm_i = int(sp[1].split("/")[2]) - 1 - norm_offset
            pos_i = []
            uv_i = []

            for i in range(n):
                sp2 = sp[i + 1].split("/")
                pos_i.append(int(sp2[0]) - 1 - pos_offset)
                uv_i.append(int(sp2[1]) - 1 - uv_offset)

            face = Face(pos_i, uv_i, norm_i)
            if n == 3:
                part.tris.append(face)
            else:
                part.quads.append(face)
    
    f.close()

parse_obj()

# count and write number of parts
# assumes that they are tightly packed
num_parts = 0
for i in range(len(parts)):
    if parts[i] is not None:
        num_parts += 1
    else:
        break
# convert (pre-centered) part meshes to bboxes
actual_bboxes = [None] * num_parts
for i in range(num_parts):
    actual_bboxes[i] = bboxes[i].to_bounding_box()

# ----------------------------- SERIALIZATION -----------------------------

part_table_offset = HEADER_LEN
part_table_size   = num_parts * 4

bboxes_offset = part_table_offset + part_table_size
bboxes_size   = num_parts * BBOX_LEN

out = bytearray()

# -- level_t --
out += to_ulong(num_parts)

# write pointer to part table
out += to_ulong(part_table_offset)
# write pointer to bboxes
out += to_ulong(bboxes_offset)

# -- lvl_part --

offset = bboxes_offset + bboxes_size
assert offset & 3 == 0
# serialize parts, write part table
parts_data = bytearray()
for i in range(num_parts):
    out += to_ulong(offset)
    p = parts[i].serialize(actual_bboxes[i])
    offset += len(p)
    parts_data += p

# serialize bboxes, write bbox table
for i in range(num_parts):
    out += actual_bboxes[i].serialize()

# write the parts
out += parts_data

with open(lvl_file, "wb") as f:
    f.write(out)
