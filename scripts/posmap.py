import configparser

class PosMap:

    def __init__(self, path):
        self.cfg = configparser.ConfigParser()
        self.cfg.read(path)
    
    def set_offset(self, x, y):
        self.offset_x = x
        self.offset_y = y
    
    def get_clut_code(self, x, y):
        x += self.offset_x
        y += self.offset_y

        for section in self.cfg.sections():
            tim = self.cfg[section]

            tim_x = int(tim["X"])
            tim_y = int(tim["Y"])
            tim_w = int(tim["Width"])
            tim_h = int(tim["Height"])

            if x < tim_x or y < tim_y:
                continue
            if x >= tim_x + tim_w or y >= tim_y + tim_h:
                continue

            tim_clut_x = int(tim["ClutX"])
            tim_clut_y = int(tim["ClutY"])

            return (((tim_clut_y)<<6)|(((tim_clut_x)>>4)&0x3f))
        
        print("failed to find a TIM mapped to location {}, {}".format(x, y))
        return 0
