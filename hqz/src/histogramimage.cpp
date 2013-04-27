/* XXX: Table goes here */

import numpy
from colorpy.ciexyz import _CIEXYZ_1931_table as table
from colorpy.colormodels import srgb_rgb_from_xyz_matrix as xyz2rgb

assert table[0][0] == 360
assert table[-1][0] == 830
assert len(table) == (830 - 360 + 1)

for (nm, x, y, z) in table:
    r, g, b = map(int, 8192.0 * xyz2rgb.dot([x, y, z]))
    print "    /* %snm */ %d, %d, %d," % (nm, r, g, b)

