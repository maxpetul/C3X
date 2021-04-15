
import os

def go_to_conquests_directory ():
        wd = os.getcwd ()
        i = wd.rfind ("\\Conquests")
        if i >= 0:
                os.chdir (wd[:i] + "\\Conquests\\")
        else:
                raise Exception ("Script must be launched from a subfolder of the \"Conquests\" folder")

def str_from_bytes (b):
        n = b.find (0) # Find the null terminator
        if n < 0:
                n = len (b)
        return b[:n].decode ("cp1252")
