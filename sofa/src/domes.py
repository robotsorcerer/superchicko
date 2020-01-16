
import Sofa

import os

path = '/root/superchicko/ros/srs_traj_opt/patient_description/meshes/dome/'
tetramesh = "dome_ring.stl"


def createScene(rootNode):
    rootNode.createObject('VisualStyle', displayFlags='showVisualModels showBehaviorModels hideCollisionModels hideBoundingCollisionModels hideForceFields hideInteractionForceFields hideWireframe')
    rootNode.createObject('RequiredPlugin', pluginName='CGALPlugin')
    rootNode.createObject('RequiredPlugin', pluginName='SofaExporter')

    rootNode.createObject('BackgroundSetting', color='0 0.168627 0.211765')
    rootNode.createObject('OglSceneFrame', style="Arrows", alignment="TopRight")

    ##########################################
    # Generation                             #
    ##########################################
    node = rootNode.createChild('node')
    #1- Specify the input 3D surfacic mesh
    node.createObject('MeshSTLLoader', name='mesh', filename=path+tetramesh)
    node.createObject('MeshGenerationFromPolyhedron', name='gen', template='Vec3d', inputPoints='@mesh.position', inputTriangles='@mesh.triangles', drawTetras='1',
           # This parameter controls the size of mesh tetrahedra. It provides an upper bound on the circumradius of the mesh tetrahedra.
           cellSize="1",
           facetAngle="10",
           # This parameter provides an upper bound for the radii of the surface Delaunay ball; a larger value may lead to larger tetrahedra.
           facetSize=".1",

           # This parameter controls the shape of mesh cells. Actually, it is an upper bound for the ratio between the circumradius of a mesh
           # tetrahedron and its shortest edge. There is a theoretical bound for this parameter: the Delaunay refinement process is guaranteed
           # to terminate for values larger than 2.
           cellRatio="2",   #Convergence problem if < 2

           # The approximation error between the boundary and the subdivision surface. It provides an upper bound for the distance
           # between the circumcenter of a surface facet and the center of a surface Delaunay ball of this facet.
           facetApproximation="1"
           )
    node.createObject('Mesh', position='@gen.outputPoints', tetrahedra='@gen.outputTetras')
    #2- Export the output 3D tetrahedras mesh
    node.createObject('VTKExporter', filename=path+tetramesh.split(".")[0]+".vtk", edges='0', tetras='1', exportAtBegin='1')

    ##########################################
    # Visualization                          #
    ##########################################
    node.createObject('OglModel', src="@mesh", color="0.0 0.7 0.7 0.5")

    return rootNode
