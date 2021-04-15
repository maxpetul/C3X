from gimpfu import *

def civ3_stack_buttons (image, drawable):
    for x in range (8):
        for y in range (10):
            pdb.gimp_image_select_rectangle (image, 2, 32*x, 32*y, 32, 32)
            floated_layer = pdb.gimp_selection_float (drawable, 3, 3)
            pdb.gimp_layer_scale (floated_layer, 26, 26, True)
            pdb.gimp_floating_sel_anchor (floated_layer)
            
register (
    "python-fu-civ3-stack-buttons",
    "short description",
    "long description",
    "Flintlock", "Flintlock", "2021",
    "Civ3 Stack Buttons",
    "", # type of image (RGB, RGBA, GRAY, etc)
    [
        (PF_IMAGE, "image", "takes current image", None),
        (PF_DRAWABLE, "drawable", "Input layer", None)
    ],
    [],
    civ3_stack_buttons,
    menu="<Image>/File"
)

main ()
