            ####### hqz exporter for blender V0.3 ##############
#   Damien Picard 2014
#
#  Freestyle hqz export module
#
#
# Tightly based on freestyle SVG exporter by:
# Tamito KAJIYAMA <19 August 2009>
#
#  Thank you !
#
#
 
from freestyle import *
from logical_operators import *
#from chainingiterators import *
#from predicates import *
from shaders import *
#from types import *
import bpy
import os

class SVGWriter(StrokeShader):
    def __init__(self, w, h):
        StrokeShader.__init__(self)
        self.width, self.height = w, h
    def shade(self, stroke):
        points = []
        for v in stroke:
            #print(v.point)
            point=[]
            x, y = v.point
            point.append(x)
            point.append(self.height - y)
            points.append(point)
        #print(points)
        points = str(points)
        points += ', '
        bpy.context.scene['hqz_3D_objects_string'] += points
        #points = " ".join(points)
        r, g, b = v.attribute.color * 255
        color = "#%02x%02x%02x" % (r, g, b)
        width = v.attribute.thickness
        width = width[0] + width[1]
        #self.file.write(_PATH % (color, width, points))

import freestyle
#import utils
scene = freestyle.getCurrentScene()
current_frame = scene.frame_current
w = scene.render.resolution_x
h = scene.render.resolution_y

upred = QuantitativeInvisibilityUP1D(0)
Operators.select(upred)
Operators.bidirectional_chain(ChainSilhouetteIterator(), NotUP1D(upred))
writer = SVGWriter(w, h)
shaders_list = [
    ConstantThicknessShader(2),
    #pyDepthDiscontinuityThicknessShader(1, 4),
    ConstantColorShader(0, 0, 0),
    #pyMaterialColorShader(0.5),
    writer,
]
Operators.create(TrueUP1D(), shaders_list)
