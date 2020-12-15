import sys
from os import listdir
from os import path
import configparser

# typedef struct {
#     u_char u, v;
# } mdl_uv;

# typedef struct {
#     u_short num_vertices;
#     u_short num_tris;
#     u_short num_frames;
#     u_short pad;

#     SVECTOR **pos;  // [num_vertices][num_frames]
#     mdl_uv *uvs;    // [num_vertices]
#     u_short *tri_indices; // [num_tris]
# } model_t;

MODEL_HEADER_LEN = 20

# mdl file layout:
#   model_t
#   pos array
#   uv array
#   tri indices array

def to_long(x):
    return x.to_bytes(4, byteorder="little", signed=True)

def to_ulong(x):
    return x.to_bytes(4, byteorder="little", signed=False)

def to_short(x):
    return x.to_bytes(2, byteorder="little", signed=True)

def to_ushort(x):
    return x.to_bytes(2, byteorder="little", signed=False)

def to_char(x):
    return x.to_bytes(1, byteorder="little", signed=True)

def to_uchar(x):
    return x.to_bytes(1, byteorder="little", signed=False)

folder = sys.argv[1]
model_name = sys.argv[2]

scale = 200

class Model:
    num_frames = 0
    positions = []
    uvs = []
    tris = []

def find_frame_num(file):
    uscore_split = file.split("_")
    frame = uscore_split[len(uscore_split) - 1]
    frame = frame[:-4]
    return int(frame) - 1

def parse_frame(file, mdl):
    global scale

    frame = find_frame_num(file)

    f = open(file, "r")

    frame_positions = []
    
    while True:
        line = f.readline()
        if not line:
            break
        sp = line.split(" ")
        if len(sp) == 0:
            continue

        if sp[0] == "v":
            x = int(float(sp[1]) * scale)
            y = int(float(sp[2]) * scale)
            z = int(float(sp[3]) * scale)
            frame_positions.append([x, y, z])
        
        if frame == 0: # only parse faces/uvs for frame 0
            if sp[0] == "f":
                assert len(sp) == 4

                pos = []
                uv  = []

                for i in range(3):
                    sp2 = sp[i + 1].split("/")
                    pos.append(int(sp2[0]) - 1)
                    uv.append(int(sp2[1]) - 1)
                
                mdl.tris.append(pos + uv)
            elif sp[0] == "vt":
                u = int(float(sp[1]) * 255)
                v = 255 - int(float(sp[2]) * 255)
                mdl.uvs.append([u, v])

    mdl.positions.append(frame_positions)


# mdl file layout:
#   model_t
#   pos array
#   uv array
#   tri indices array
def serialize(output, mdl):
    out = bytearray()

    body = bytearray()
    # positions
    offset_pos = len(body) + MODEL_HEADER_LEN
    for frame in mdl.positions:
        for vertex in frame:
            for f in vertex:
                body += to_short(f)
            body += to_short(0) # pad

    # uvs
    offset_uv = len(body) + MODEL_HEADER_LEN
    for uv in mdl.uvs:
        for uchar in uv:
            body += to_uchar(uchar)

    # tris
    offset_tris = len(body) + MODEL_HEADER_LEN
    for tri in mdl.tris:
        for short in tri:
            body += to_short(short)

    header = bytearray()
    num_vertices = len(mdl.positions[0])
    #print(num_vertices)
    num_tris = len(mdl.tris)

    header += to_ushort(num_vertices)
    header += to_ushort(num_tris)
    header += to_ushort(mdl.num_frames)
    header += to_ushort(0)

    header += to_ulong(offset_pos)
    header += to_ulong(offset_uv)
    header += to_ulong(offset_tris)

    out = header + body

    with open(output, "wb") as f:
        f.write(out)

mdl = Model()

frame_files = []
for f in listdir(folder):
    if f.startswith(model_name) and f.endswith(".obj"):
        frame_files.append(f)

mdl.num_frames = len(frame_files)
print("num frames: {}".format(mdl.num_frames))

for f in frame_files:
    parse_frame(path.join(folder, f), mdl)

serialize(path.join(folder, model_name + ".mdl"), mdl)
