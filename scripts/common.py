
# i didnt know about struct.pack...

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

def pad_byte_arr(arr):
    for i in range(4 - len(arr) & 3):
        arr += to_uchar(0)