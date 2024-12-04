from pypylon import pylon as py
from pypylon import genicam as geni

tlf = py.TlFactory.GetInstance()

cam = py.InstantCamera(tlf.CreateFirstDevice())
cam.Open()

# get the root node "top of the tree"
root_node = cam.Root

indent = 0
def show_features(node):
    # categories have features below them
    global indent
    indent+=1
    for f in node.Features:
        # output if the feature is currently available
        if geni.IsAvailable(f.Node):
            print("+"*indent, f.Node.Name, type(f))
        else:
            print("-"*indent, f.Node.Name, type(f))
        # if the current node is a category itself ... render the features below
        if f.Node.GetPrincipalInterfaceType() == geni.intfICategory:
            show_features(f)
    indent -=1
            
show_features(root_node)