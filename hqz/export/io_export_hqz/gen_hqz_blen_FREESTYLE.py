####### hqz exporter for blender V0.3.2 ##############
#   Damien Picard 2014
#
#  Freestyle hqz export module
#
#
# Heavily based on freestyle SVG exporter by:
# Tamito KAJIYAMA <19 August 2009>
#
#  Thank you !
#
#
 
from freestyle import *
#from ChainingIterators import ChainSilhouetteIterator
from logical_operators import (AndUP1D, NotUP1D)
#from shaders import (ConstantColorShader,
#    ConstantThicknessShader, StrokeShader)
#from freestyle import Operators, ChainSilhouetteIterator
import bpy
import os

class ObjectNamesUP1D(UnaryPredicate1D):
    def __init__(self, names, negative):
        UnaryPredicate1D.__init__(self)
        self._names = names
        self._negative = negative

    def __call__(self, viewEdge):
        found = viewEdge.viewshape.name in self._names
        if self._negative:
            return not found
        return found

class HQZWriter0(StrokeShader):
    def __init__(self, w, h):
        StrokeShader.__init__(self)
        self.width, self.height = w, h
    def shade(self, stroke):
        points = []
        for v in stroke:
            point=[0]
            x, y = v.point
            point.append(x)
            point.append(self.height - y)
            points.append(point)
        points = str(points)
        points += ', '
        bpy.context.scene['hqz_3D_objects_string'] += points

def join_unary_predicates(upred_list, bpred):
    if not upred_list:
        return None
    upred = upred_list[0]
    for p in upred_list[1:]:
        upred = bpred(upred, p)
    return upred

import freestyle
scene = freestyle.getCurrentScene()
current_frame = scene.frame_current
w = scene.render.hqz_resolution_x
h = scene.render.hqz_resolution_y

names={}
try:
    names = dict((ob.name, True) for ob in bpy.data.groups['0'].objects)
except:
    pass

selection_criteria = []
selection_criteria.append(QuantitativeInvisibilityUP1D(0))
selection_criteria.append(ObjectNamesUP1D(names, False))
upred = join_unary_predicates(selection_criteria, AndUP1D)
Operators.select(upred)
Operators.bidirectional_chain(ChainSilhouetteIterator(), NotUP1D(upred))
writer = HQZWriter0(w, h)
shaders_list = [
    ConstantThicknessShader(1),
    #pyDepthDiscontinuityThicknessShader(1, 4),
    ConstantColorShader(0, 0, 0),
    #pyMaterialColorShader(0.5),
    writer,
]
Operators.create(TrueUP1D(), shaders_list)
