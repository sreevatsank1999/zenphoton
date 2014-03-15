####### hqz exporter for blender V0.3.3 ##############
#
#   Damien Picard 2014

#	HQZ by M. Elizabeth Scott - http://scanlime.org/
#
# Usage:
# - install add-on to blender/2.6x/scripts/addons
# - or zip the folder and go to User Preferences -> Addons -> Install from File...
# - a wild panel appears ! in the View 3D Toolbar.
# - First, check 3D export for freestyle contour rendering.
#   This uses a line module to select lines to render from perspective camera
#   If left unchecked, the drawiwng will be 2D.
# - click "Prepare scene" (and switch to camera view)
# - change settings according to your gneeds
# - write materials to the materials list
# - if you need more materials, add sliders and corresponding stuff to the interface...
# - add mesh objects
# - in the object properties, select the material you wish to assign (default = 0)
# - change light color and intensity : set color value to 0 for white light or choose hue
#   alternatively you can also set hqz_spectral_light to 1 and choose a spectral range between 400 & 700 nm (Start/End)
# - click "Export"
# - that's pretty much it
# - if something doesn't work:
#      click "Prepare scene" again
#      check that you have at least one mesh object and one light object
#      PiOverFour on GitHub
#
#
#  ABOUT 2D MODE :
#
# - if you wish to export normals (specially for caustics), I recommand activating check_Z, getting your
#   object vertices on the XY plane and extruding all edges to Z. this makes sure that only edges
#   on the XY plane are exported, and the normals are more predictable and controlable.
# - add spot or point light objects
# - rotate spots only around the Z axis
#
#
#  ABOUT 3D MODE :
#
# - spot lights are not currently supported in 3D mode. The maths are 
#   too damn high.
#
###################################
#
# spectral color cheatsheet
#
#     400        450        500        550        600        650        700   nm -->
#    PURPLE      BLUE    CYAN    GREEN     YELLOW     ORANGE   RED
#
###################################
#
# TODO 
# exception if folder not set
# FREESTYLE : 
#             spotlights in cam space
#             ignore hidden lights
#
#             custom freestyle contour shaders?
#             option not to check light visibility? or reduce intensity?
#             
#
###################################

bl_info = {
    "name": "HQZ exporter",
    "author": "Damien Picard",
    "version": (0, 3, 2),
    "blender": (2, 6, 9),
    "location": "View3D > Tool Shelf > HQZ Exporter",
    "description": "Export scene to HQZ renderer",
    "warning": "",
    "wiki_url": "",
    "tracker_url": "",
    "category": "Import-Export"}
    
import bpy
import bpy_extras
import mathutils
from math import pi, floor, fabs
import os

debug = True #remove all newlines to read with wireframe.html


####UTILITY FUNCTIONS
    
def HSV2wavelength(color):
    '''Convert RGB color to a wavelength from 400 to 700nm (approximative).'''
    wavelength = ((0.7-color.h)-floor((0.7-color.h)))*300+400
    if color.v == 0:
        wavelength = 0
    return wavelength
    
def get_loc(context, object):
    '''Get location for object in argument.'''
    sc = context.scene
    x, y = object.matrix_world.to_translation()[0]*sc.hqz_resolution_x, object.matrix_world.to_translation()[1]*sc.hqz_resolution_x
    return x,y

def get_rot(object):
    '''Get rotation for object in argument.'''
    rot = -object.matrix_world.to_euler()[0]*180/pi
    return rot

def vector2rotation(context, vector):
    '''Convert 2D vector to signed angle between -180 and 180 degrees.'''
    sc = context.scene
    null_vec = mathutils.Vector((1,0))
    vec = vector.to_2d()
    vec_angle = vec.angle_signed(null_vec)*180/pi
    if sc.hqz_normals_invert:
        vec_angle = 180 - vec_angle
    else:
        vec_angle *= -1
    if vec_angle < -180:
        vec_angle += 360
    if vec_angle > 180:
        vec_angle -= 360
    
    return vec_angle

def check_inside(context,x,y):
    '''Check if object is inside field of view'''
    sc = context.scene
    if (sc.hqz_outside_limit < 0 \
    or (x + sc.hqz_outside_limit) > 0 \
    and (y + sc.hqz_outside_limit) > 0 \
    and (x - sc.hqz_outside_limit) < sc.render.resolution_x \
    and (y - sc.hqz_outside_limit) < sc.render.resolution_y):
        return True


#SET SCENE
def prepare(context):
    sc = context.scene
    sc.render.engine = 'BLENDER_RENDER'
    cam = sc.camera
    
    ###add object ID properties
    for obj in sc.objects:
        if obj.type == 'LAMP' and obj.is_visible(sc):
            if not "hqz_1_light_start" in obj.data:
                obj.data["hqz_1_light_start"] = 0.0
            if not "hqz_2_light_end" in obj.data:
                obj.data["hqz_2_light_end"] = 0.0
            if not "hqz_3_spectral_light" in obj.data:
                obj.data["hqz_3_spectral_light"] = True
            if not "hqz_4_spectral_start" in obj.data:
                obj.data["hqz_4_spectral_start"] = 400.0
            if not "hqz_5_spectral_end" in obj.data:
                obj.data["hqz_5_spectral_end"] = 700.0
                
        if obj.type == 'MESH' and obj.is_visible(sc):
            if not "hqz_material" in obj.data:
                obj.data["hqz_material"] = 0
    
    ###settings for freestyle export
    if sc.hqz_export_3D:
        sc.hqz_resolution_x = sc.render.resolution_x * bpy.context.scene.render.resolution_percentage / 100
        sc.hqz_resolution_y = sc.render.resolution_y * bpy.context.scene.render.resolution_percentage / 100
        
        sc.render.use_freestyle = True
        sc.render.line_thickness_mode = 'RELATIVE'
        rl = sc.render.layers.active
        rl.use_solid = False
        rl.use_halo = False
        rl.use_ztransp = False
        rl.use_sky = False
        rl.use_edge_enhance = False
        rl.use_strand = False
        rl.use_freestyle = True
        rl.freestyle_settings.mode = 'SCRIPT'
        rl.freestyle_settings.use_culling = True
        if len(rl.freestyle_settings.modules) < 5:
            ##workaround for 2.70 API change
            if bpy.app.version[1] > 69:
                freestyle_file_ver = 'gen_hqz_blen_FREESTYLE_270.py'
            else:
                freestyle_file_ver = 'gen_hqz_blen_FREESTYLE.py'
            script_path = bpy.utils.script_path_user()+"/addons/io_export_hqz/"+freestyle_file_ver
            freestyle_module_1 = bpy.data.texts.load(filepath=script_path, internal = True)
            freestyle_module_1.name = 'freestyle_module_0.py'
            bpy.ops.scene.freestyle_module_add()
            rl.freestyle_settings.modules[0].script = freestyle_module_1
            for module in range(1,5):
                module_name = 'freestyle_module_{0}'.format(module)#create new module for each material
                #print("module1 : ",freestyle_module_1)
                module_text = freestyle_module_1.copy()#duplicate module
                module_text.name = module_name+'.py'#rename module
                module_text.use_fake_user = True #save file on reloading blend
                for line in module_text.lines:#modify module
                    line.body = line.body.replace('            point=[0]', '            point=[{0}]'.format(module))
                    line.body = line.body.replace("bpy.data.groups['0'].objects", "bpy.data.groups['{0}'].objects".format(module))
                    line.body = line.body.replace("HQZWriter0", "HQZWriter{0}".format(module))
                bpy.ops.scene.freestyle_module_add()
                exec("rl.freestyle_settings.modules[{0}].script = module_text.id_data".format(module))
        
        for grp in range(5): #create groups
            if not str(grp) in bpy.data.groups:
                bpy.data.groups.new(str(grp))
                
        for obj in sc.objects:
            if obj.type == 'MESH' and obj.is_visible(sc):
                for grp in range(5): #remove mesh object from all groups
                    if obj.name in bpy.data.groups[str(grp)].objects:
                        bpy.data.groups[str(grp)].objects.unlink(obj)
                current_group = str(obj.data["hqz_material"])
                bpy.data.groups[current_group].objects.link(obj)
        
    else:
        sc.render.resolution_x = sc.hqz_resolution_x
        sc.render.resolution_y = sc.hqz_resolution_y
        cam.data.type = 'ORTHO'
        cam.data.ortho_scale = 1
        if sc.hqz_resolution_y > sc.hqz_resolution_x:
            cam.data.ortho_scale = sc.hqz_resolution_y / sc.hqz_resolution_x
        cam.rotation_euler = (0,0,0)
        cam.location = (0.5,sc.hqz_resolution_y/sc.hqz_resolution_x/2,5)
    
    

###START WRITING
def export(context):
    '''Create export text and write to file.'''
    sc = context.scene
    prepare(context)
    if sc.hqz_animation:
        start_frame = sc.hqz_start_frame
        frame_range = range(sc.hqz_start_frame, sc.hqz_end_frame+1)
    else:
        start_frame = sc.frame_current
        frame_range = start_frame,
        
    ###DIRTY LOOP FOR BATCH SCRIPT.
    if sc.hqz_batch:
        platform = os.sys.platform
        shell_path = sc.hqz_directory +'batch'
        shell_script = ''
        if 'win' in platform:
            shell_path += '.bat'
        else:
            shell_path += '.sh'
            shell_script += '#!/bin/bash\n\n'
        for frame in frame_range:
            shell_script += '"' + sc.hqz_engine + 'hqz' \
                + '" "'+ sc.hqz_directory  + sc.hqz_file \
                + '_' + str(frame).zfill(4) +'.json" "'  \
                + sc.hqz_directory  + sc.hqz_file + '_' \
                + str(frame).zfill(4) +'.png"'
            if sc.hqz_ignore:
                shell_script += ' -i'
            shell_script += '\n'
        file = open(shell_path, 'w')
        file.write(shell_script)
        file.close()
        
        

    for frame in frame_range:
        print('exporting frame',frame)
        
        ####GET FREESTYLE POINTS
        
        if sc.hqz_export_3D:
            sc['hqz_3D_objects_string'] = "["
            sc.frame_set(frame)
            bpy.ops.render.render()
            sc['hqz_3D_objects_string'] = sc['hqz_3D_objects_string'][:-2]
            sc['hqz_3D_objects_string'] += "]"
            #print(sc['hqz_3D_objects_string'])
        
        if sc.hqz_animation:
            sc.frame_set(frame)
        
        scene_code = ''

        scene_code += '{\n'#begin
        scene_code += '    "resolution": [' + str(sc.hqz_resolution_x) + ', ' + str(sc.hqz_resolution_y) + '],\n'
        scene_code += '    "viewport":  [0, 0, ' + str(sc.hqz_resolution_x) + ', ' + str(sc.hqz_resolution_y) + '],\n'
        scene_code += '    "exposure": ' + str(sc.hqz_exposure) + ',\n'
        scene_code += '    "gamma": ' + str(sc.hqz_gamma) + ',\n'
        scene_code += '    "rays": ' + str(sc.hqz_rays) + ',\n'
        if sc.hqz_time != 0:
            scene_code += '    "timelimit": ' + str(sc.hqz_time) + ',\n'
        scene_code += '    "seed": ' + str(sc.hqz_seed) + ',\n'


        #### LIGHTS

        scene_code += '    "lights": [\n'
        cam = sc.camera

        for light in sc.objects:
            if light.type == 'LAMP' and light.is_visible(sc):
                light_obstacle = False
                if sc.hqz_export_3D:
                    for obj_to_check in sc.objects:#raycast for visibility checking
                        if obj_to_check.type == 'MESH' and obj_to_check.is_visible(sc):
                            cam_loc = obj_to_check.matrix_world.inverted() * cam.matrix_world.to_translation()
                            light_loc = obj_to_check.matrix_world.inverted() * light.matrix_world.to_translation()
                            if obj_to_check.ray_cast(light_loc, cam_loc)[2] != -1:
                                light_obstacle = True
                                break
                        
                if not light_obstacle:
                    use_spectral = light.data["hqz_3_spectral_light"]
                    spectral_start = light.data["hqz_4_spectral_start"]
                    spectral_end = light.data["hqz_5_spectral_end"]
                    wav = HSV2wavelength(light.data.color)
                    if not sc.hqz_export_3D:###2D EXPORT
                        x, y = get_loc(context, light)
                        view_coords = (1,1,1)
                    else:###FREESTYLE EXPORT
                        view_coords = bpy_extras.object_utils.world_to_camera_view(sc, sc.camera, light.location)
                        #print(view_coords)
                        x, y = (view_coords[0]-cam.data.shift_x*2)*sc.hqz_resolution_x, (view_coords[1]-cam.data.shift_y*2)*sc.hqz_resolution_y
                        #print(x,y)
                    if view_coords[2] > 0 and check_inside(context,x,y):#check that light is inside camera field and not behind it
                        
                        y = sc.hqz_resolution_y-y
                        rot = get_rot(light)
                        
                        scene_code += '        [ '    
                        scene_code += str(light.data.energy) + ', '                                     #LIGHT POWER
                        scene_code += str(x) + ', '                                                   #XPOS
                        scene_code += str(y)                                                          #YPOS
                        scene_code += ', [0, 360], ['                                                 #POLAR ANGLE
                        scene_code += str(light.data["hqz_1_light_start"]) + ', ' #POLAR DISTANCE MIN
                        scene_code += str(light.data["hqz_2_light_end"]) + '], [' #POLAR DISTANCE MAX
                        if light.data.type == 'SPOT':
                            scene_code += str(rot-light.data.spot_size*90/pi) + ', '               #ANGLE
                            scene_code +=  str(rot+light.data.spot_size*90/pi) + '], '
                        else:
                            scene_code += '0, 360], '
                        if use_spectral:
                            scene_code += '[{0}, {1}] ],\n'.format(spectral_start, spectral_end)   
                        else:
                            scene_code += str(int(wav))  +' ],\n'                                         #WAVELENGTH
                
        scene_code = scene_code[:-2]#remove last comma
        scene_code += '\n    ],\n'



        scene_code += '    "objects": [\n'


        #get Blender edge list
        if not sc.hqz_export_3D:###2D EXPORT
            edge_list = []
            for obj in sc.objects:
                if obj.type == 'MESH' and obj.is_visible(sc):
                    for edge in obj.data.edges:
                        edgev = []
                        vertices = list(edge.vertices)
                        material = obj.data["hqz_material"]
                        edgev.append(material)
                        edgev.append(obj.matrix_world*obj.data.vertices[vertices[0]].co)
                        edgev.append(obj.matrix_world*obj.data.vertices[vertices[1]].co)
                        if sc.hqz_normals_export:
                            #print(vertices[0])
                            edgev.append((obj.rotation_euler[2] * 180/pi) + vector2rotation(context, obj.data.vertices[vertices[0]].normal))
                            edgev.append((obj.rotation_euler[2] * 180/pi) + vector2rotation(context, obj.data.vertices[vertices[1]].normal))
                        
                        if sc.hqz_check_Z:
                            if fabs((obj.matrix_world*obj.data.vertices[vertices[0]].co)[2]) < 0.0001 and fabs((obj.matrix_world*obj.data.vertices[vertices[1]].co)[2]) < 0.0001:
                                edge_list.append(edgev)
                        else:
                            edge_list.append(edgev)
        
        ####OBJECTS
        if not sc.hqz_export_3D:###2D EXPORT
            for edge in edge_list:
                if check_inside(context, edge[1][0]*sc.hqz_resolution_x, edge[1][1]*sc.hqz_resolution_x):
                    #print(edge)
                    scene_code += '        [ '    
                    scene_code += str(edge[0]) + ', '                                                                  #MATERIAL
                    scene_code += str(edge[1][0]*sc.hqz_resolution_x) + ', '                                           #VERT1 XPOS
                    scene_code += str(sc.hqz_resolution_y - (edge[1][1]*sc.hqz_resolution_x)) + ', '                   #VERT1 YPOS
                    if sc.hqz_normals_export:
                        scene_code += str(edge[3]) + ', '                                                              #VERT1 NORMAL
                    
                    scene_code += str(edge[2][0]*sc.hqz_resolution_x - (edge[1][0]*sc.hqz_resolution_x)) + ', '        #VERT2 DELTA XPOS 
                    scene_code += str(-1 * (edge[2][1]*sc.hqz_resolution_x - (edge[1][1]*sc.hqz_resolution_x))) + '],' #VERT2 DELTA YPOS 
                    if sc.hqz_normals_export:
                        scene_code = scene_code[:-2]#remove last comma and bracket
                        normal = (edge[4]) - (edge[3])
                        if normal < -180:
                            normal += 360
                        if normal > 180:
                            normal -= 360
                        scene_code += ',' + str(normal) + '],'                           #VERT2 NORMAL
                    scene_code += '\n'
            scene_code = scene_code[:-2]#remove last comma
            
        else: ###FREESTYLE EXPORT
            point_list = eval(sc['hqz_3D_objects_string'])
            for stroke in point_list:
                print(stroke)
                for index, point in enumerate(stroke):
                    if index != len(stroke)-1:
                        scene_code += '        [ '
                        scene_code += str(point[0]) + ', '                                #MATERIAL
                        scene_code += str(point[1]) + ', '                                #VERT1 XPOS
                        scene_code += str(point[2]) + ', '                                #VERT1 YPOS
                        if sc.hqz_normals_export:                                         #VERT1 NORMAL
                            if index == 0:
                                v1N = vector2rotation(context, mathutils.Vector((point[0], point[1])))
                                scene_code += str(v1N) + ', '
                            else:
                                v1NX = 2*point[1] - stroke[index-1][1] - stroke[index+1][1]
                                v1NY = 2*point[2] - stroke[index-1][2] - stroke[index+1][2]
                                v1N = mathutils.Vector((v1NX, v1NY))
                                v1N = vector2rotation(context, v1N)
                                scene_code += str(v1N) + ', '
                        
                        
                        scene_code += str(stroke[index+1][1] - point[1]) + ', '           #VERT2 DELTA XPOS
                        scene_code += str(stroke[index+1][2] - point[2]) + '],'           #VERT2 DELTA YPOS
                        if sc.hqz_normals_export:                                         
                            scene_code = scene_code[:-2]#remove last comma and bracket
                            if index == len(stroke)-2:
                                v2N = vector2rotation(context, mathutils.Vector((point[2], point[1])))
                            else:
                                v2NX = 2*stroke[index+1][1] - point[1] - stroke[index+2][1]
                                v2NY = 2*stroke[index+1][2] - point[2] - stroke[index+2][2]
                                v2N = mathutils.Vector((v2NX, v2NY))
                                v2N = vector2rotation(context, v2N)
                                v2N = v2N - v1N
                            if v2N < -180:
                                v2N += 360
                            if v2N > 180:
                                v2N -= 360
                            scene_code += ',' + str(v2N) + '],'                           #VERT2 NORMAL
                            
                        scene_code += '\n'
            scene_code = scene_code[:-2]#remove last comma
                
        scene_code += '\n    ],\n'
        scene_code += '    "materials": [\n'

        ###mats
        materials = []
        for mat_index in range(5):
            new_mat = []
            for component in range(3):
                new_mat.append(eval('sc.hqz_material_{0}[{1}]'.format(mat_index, component)))
            materials.append(new_mat)
        #print(materials)
        
        for mat in materials:
            scene_code += '        [ [ ' + str(mat[0]) + ', "d" ], [ ' + str(mat[1]) + ', "t" ], [ ' + str(mat[2]) + ', "r" ] ],\n'
                
        scene_code = scene_code[:-2]#remove last comma
        scene_code += '\n    ]\n'

        scene_code += '}'


        if debug:
            scene_code = scene_code.replace('\n', '')
            print(scene_code)
            
        #text.write(scene_code)

        save_path = sc.hqz_directory + sc.hqz_file + '_' + str(frame).zfill(4) + '.json'


        d = os.path.dirname(sc.hqz_directory)
        if not os.path.exists(d):
            os.makedirs(d)
            
        file = open(save_path, 'w')
        file.write(scene_code)
        file.close()



#####BLENDER STUFF

###Properties
def init_properties():
    scene_type = bpy.types.Scene
    
    scene_type.hqz_engine = bpy.props.StringProperty(
        name="Engine path",
        subtype="DIR_PATH")
        
    scene_type.hqz_directory = bpy.props.StringProperty(
        name="Export directory",
        subtype="DIR_PATH")
        
    scene_type.hqz_file = bpy.props.StringProperty(
        name="File name")
        
    scene_type.hqz_batch = bpy.props.BoolProperty(
        name="Export batch file",
        default=True)
        
    scene_type.hqz_ignore = bpy.props.BoolProperty(
        name="Ignore existing file",
        default=False)
        
        
    scene_type.hqz_resolution_x = bpy.props.IntProperty(
        name="X resolution",
        default=1980,
        min=0)
        
    scene_type.hqz_resolution_y = bpy.props.IntProperty(
        name="Y resolution",
        default=1080,
        min=0)
        
    scene_type.hqz_exposure = bpy.props.FloatProperty(
        name="Exposure",
        default=0.5,
        min=0)
        
    scene_type.hqz_gamma = bpy.props.FloatProperty(
        name="Gamma",
        default=2.2,
        min=0)
        
    scene_type.hqz_rays = bpy.props.IntProperty(
        name="Number of rays",
        default=100000,
        min=0)
        
    scene_type.hqz_seed = bpy.props.IntProperty(
        name="Seed",
        default=0,
        min=0)
        
    scene_type.hqz_time = bpy.props.IntProperty(
        name="Max render time (0 for infinity)",
        default=0,
        min=0)
        
    scene_type.hqz_animation = bpy.props.BoolProperty(
        name="Export animation",
        default=False)
        
    scene_type.hqz_start_frame = bpy.props.IntProperty(
        name="Start Frame",
        default=0)
        
    scene_type.hqz_end_frame = bpy.props.IntProperty(
        name="End Frame",
        default=5)
        
    scene_type.hqz_normals_export = bpy.props.BoolProperty(
        name="Export normals",
        default=True)
        
    scene_type.hqz_normals_invert = bpy.props.BoolProperty(
        name="Invert normals",
        default=False)
        
    scene_type.hqz_check_Z = bpy.props.BoolProperty(
        name="Check Z value",
        default=False)
        
    scene_type.hqz_export_3D = bpy.props.BoolProperty(
        name="Export 3D",
        default=True)
        
    scene_type.hqz_outside_limit = bpy.props.FloatProperty(
        name="Object outside view by (px)",
        default=(-1))
        
    scene_type.hqz_3D_objects_string = bpy.props.StringProperty(
        name="Object list",
        default="")
        
        
    scene_type.hqz_material_0 = bpy.props.FloatVectorProperty(
        name="Material 0",
        default=(0.3,0.3,0.3))
        
    scene_type.hqz_material_1 = bpy.props.FloatVectorProperty(
        name="Material 1",
        default=(0.0,0.0,0.0))
        
    scene_type.hqz_material_2 = bpy.props.FloatVectorProperty(
        name="Material 2",
        default=(0.0,0.0,0.0))
        
    scene_type.hqz_material_3 = bpy.props.FloatVectorProperty(
        name="Material 3",
        default=(0.0,0.0,0.0))
        
    scene_type.hqz_material_4 = bpy.props.FloatVectorProperty(
        name="Material 4",
        default=(0.0,0.0,0.0))
    
    



###Operator definitions

class HQZPrepare(bpy.types.Operator):
    bl_label = "Prepare scene"
    bl_idname = "view3d.hqz_prepare"
    
    def execute(self, context):
        prepare(context)
        return {'FINISHED'}
    
class HQZExport(bpy.types.Operator):
    bl_label = "Export scene"
    bl_idname = "view3d.hqz_export"

    def execute(self, context):
        export(context)
        return {'FINISHED'}
    
###UI definitions

class HQZPanel(bpy.types.Panel):
    bl_label = "HQZ Exporter"
    bl_idname = "3D_VIEW_PT_hqz"
    bl_space_type = "VIEW_3D"
    bl_region_type = "TOOLS"
    if bpy.app.version[1] > 69:
        bl_category = "Export"
        bl_context = "objectmode"
    
    
    def draw(self,context):
        sc = context.scene
        layout = self.layout
        
        col = layout.column(align=True)
        col.label("File settings")
        row = col.row(align=True)
        row.prop(sc,"hqz_export_3D")
        row = col.row(align=True)
        row.prop(sc,"hqz_engine")
        row = col.row(align=True)
        row.prop(sc,"hqz_directory")
        row = col.row(align=True)
        row.prop(sc,"hqz_file")
        row = col.row(align=True)
        row.prop(sc,"hqz_batch")
        row = col.row(align=True)
        if debug:
            row.prop(sc,"hqz_ignore")
            row = col.row(align=True)
        
        col.label(" ")
        col.label("Export settings")
        row = col.row(align=True)
        row.prop(sc,"hqz_resolution_x")
        row.prop(sc,"hqz_resolution_y")
        row = col.row(align=True)
        row.prop(sc,"hqz_exposure")
        row.prop(sc,"hqz_gamma")
        row = col.row(align=True)
        row.prop(sc,"hqz_rays")
        row = col.row(align=True)
        row.prop(sc,"hqz_seed")
        row.prop(sc,"hqz_time")
        row = col.row(align=True)
        row.prop(sc,"hqz_animation")
        row = col.row(align=True)
        row.prop(sc,"hqz_start_frame")
        row.prop(sc,"hqz_end_frame")
        row = col.row(align=True)
        row.prop(sc,"hqz_check_Z")
        row = col.row(align=True)
        row.prop(sc,"hqz_normals_export")
        row = col.row(align=True)
        row.prop(sc,"hqz_outside_limit")
        col.label("If -1, export all objects.")
        #row.prop(sc,"hqz_normals_invert")
        #row = col.row(align=True)
        
        
        row = col.row(align=True)
        col.label(" ")
        col.label("   Material settings")
        col.label("Please enter factors for")
        col.label("diffusion, transmission and")
        col.label("reflection. The sum should")
        col.label("be between 0 and 1.")
        row = col.row(align=True)
        row.prop(sc,"hqz_material_0")
        row = col.row(align=True)
        row.prop(sc,"hqz_material_1")
        row = col.row(align=True)
        row.prop(sc,"hqz_material_2")
        row = col.row(align=True)
        row.prop(sc,"hqz_material_3")
        row = col.row(align=True)
        row.prop(sc,"hqz_material_4")
        
        col.label(" ")
        row = col.row(align=True)
        row = col.row(align=True)
        
        row.operator("view3d.hqz_prepare", text="Prepare scene")
        row.operator("view3d.hqz_export", text="Export scene")



def clear_properties():
    props = ["hqz_engine", "hqz_directory", "hqz_file", 
    "hqz_batch", "hqz_resolution_x", "hqz_resolution_y",
    "hqz_exposure", "hqz_gamma", "hqz_rays",
    "hqz_seed", "hqz_time", "hqz_animation",
    "hqz_start_frame", "hqz_end_frame", "hqz_check_Z",
    "hqz_material_0", "hqz_material_1", "hqz_material_2",
    "hqz_material_3", "hqz_material_4"]
    for p in props:
        if bpy.context.window_manager.get(p) != None:
            del bpy.context.window_manager[p]
        try:
            x = getattr(bpy.types.WindowManager, p)
            del x
        except:
            pass
        
        
        

def register():
    init_properties()
    bpy.utils.register_class(HQZPrepare)
    bpy.utils.register_class(HQZExport)
    bpy.utils.register_class(HQZPanel)
    

def unregister():
    clear_properties()
    bpy.utils.unregister_class(HQZPrepare)
    bpy.utils.unregister_class(HQZExport)
    bpy.utils.unregister_class(HQZPanel)

if __name__ == "__main__":
    register()
