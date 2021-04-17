
import win32api as winapi
import win32process as winproc
import win32event as winev
import msvcrt

from commonre import *

go_to_conquests_directory ()

game_object_sizes = {b"CITY": 0x544, b"UNIT": 0x404}
vtable_pointers = {b"CITY": 0x66dc78, b"UNIT": 0x66DCF0}

# global_location is the address of the global variable that stores the pointer to the array of data objects.
# The final_name field is just where to stop reading since I don't know where to find the lengths of these arrays in most cases.
unit_type  = {"global_location": 0x9C71E0, "size": 0x138, "name_offset": 8, "name_size": 32, "final_name": "Mobile SAM"}
difficulty = {"global_location": 0x9C40C0, "size": 0x7C , "name_offset": 4, "name_size": 64, "final_name": "Sid"}

unnamed_unit_counter = 1

class GameObject:
        def reread (self):
                self.mem_bytes = self.civ_proc.read_memory (self.address, self.size)
                return self

        def get_int (self, offset, size=4, unsigned=False):
                return int.from_bytes (self.mem_bytes[offset:offset+size], byteorder = "little", signed = not unsigned)

        def get_offsets (self, value, size=4, unsigned=False):
                """ Like a reverse of get_int, returns the offsets at which 'value' appears """
                tr = []
                ints = [int.from_bytes (self.mem_bytes[size*n : size*(n+1)], byteorder = "little", signed = not unsigned) for n in range (self.size // size)]
                for n in range (len (ints)):
                        if ints[n] == value:
                                tr.append (size * n)
                return tr

unit_fields = {"ID"        : 0x1C + 0x4,
               "X"         : 0x1C + 0x8,
               "Y"         : 0x1C + 0xC,
               "CivID"     : 0x1C + 0x18,
               "RaceID"    : 0x1C + 0x1C,
               "UnitTypeID": 0x1C + 0x24,
               "Status"    : 0x1C + 0x2C,
               "Job_Value" : 0x1C + 0x38,
               "Job_ID"    : 0x1C + 0x3C,
               "UnitState" : 0x1C + 0x48}

class Unit(GameObject):
        def get (self, field_name):
                return self.get_int (unit_fields[field_name])
        
        def get_location (self):
                return (self.get_int (0x1C + 0x8), self.get_int (0x1C + 0xC))

def fit_offset (game_objects, observations, size=4):
        """ Finds offsets whose values match all observations. Observations is a list of (name, value) tuples """
        go0, val0 = observations[0]
        tr = game_objects[go0].get_offsets (val0, size)
        for go, val in observations[1:]:
                tr = [v for v in game_objects[go].get_offsets (val, size) if v in tr]
        return tr

class CivProc:
        def __init__ (self, start_suspended = False):
                startup_info = winproc.STARTUPINFO ()
                proc_handles = winproc.CreateProcess (
                        None,
                        "Civ3Conquests.exe",
                        None,
                        None,
                        False,
                        winproc.CREATE_SUSPENDED,
                        None,
                        None,
                        startup_info)
                self.proc_handle, self.thread_handle, self.proc_id, self.thread_id = proc_handles
                self.units = None
                self.cities = None
                self.unit_types = None
                self.difficulties = None
                if not start_suspended:
                        self.unsuspend ()

        def unsuspend (self):
                winproc.ResumeThread (self.thread_handle)

        def read_memory (self, address, size):
                return winproc.ReadProcessMemory (self.proc_handle, address, size)

        def write_memory (self, address, buf):
                return winproc.WriteProcessMemory (self.proc_handle, address, buf)

        def memcpy (self, dest_addr, source_addr, num_bytes):
                read = self.read_memory (source_addr, num_bytes)
                num_written = self.write_memory (dest_addr, read)
                print ("memcpy read " + str (len (read)) + " bytes and wrote " + str (num_written) + " bytes")

        def find_mapped_pages (self):
                # Scan through memory area and identify pages that have been mapped
                # There's a Win32 function to make this easy called VirtualQuery but annoyingly it's missing from pywin32.
                tr = []
                addr = 0xCE0000 # Below this address is the EXE file
                while addr < 0x21100000: # Above this address are just system DLLs 
                        try:
                                self.read_memory (addr, 1)
                                tr.append (addr)
                        except:
                                pass
                        finally:
                                addr += 0x1000
                return tr

        def read_thing (self, type_tag, address):
                tr = Unit () if type_tag == b"UNIT" else GameObject ()
                global unnamed_unit_counter
                tr.civ_proc = self
                tr.type_tag = type_tag
                tr.address = address
                tr.size = game_object_sizes[type_tag]
                tr.mem_bytes = self.read_memory (address, tr.size)
                if type_tag == b"CITY":
                        tr.name = str_from_bytes (tr.mem_bytes[0x1E0 : 0x1E0+24])
                elif type_tag == b"UNIT":
                        tr.name = str_from_bytes (tr.mem_bytes[0x74 : 0x74+24])
                        if tr.name == "":
                                tr.name = "unnamed_" + str (unnamed_unit_counter)
                                unnamed_unit_counter += 1
                else:
                        raise Exception ("name detection not implemented for type \"" + type_tag + "\"")
                return tr

        def read_data (self, data_type, address):
                tr = GameObject ()
                tr.civ_proc = self
                tr.address = address
                tr.size = data_type["size"]
                tr.mem_bytes = self.read_memory (address, tr.size)
                tr.name = str_from_bytes (tr.mem_bytes[data_type["name_offset"] : data_type["name_offset"] + data_type["name_size"]])
                return tr

        def find_game_things (self, type_tag):
                vtbl_ptr = vtable_pointers.get (type_tag)
                if vtbl_ptr is None:
                        print ("Type tag \"" + type_tag + "\" not recognized")
                        return []
                tr = {}
                for p in self.find_mapped_pages ():
                        mem_bytes = self.read_memory (p, 0x1000)
                        mem_ints = [int.from_bytes (mem_bytes[4*n : 4*n+4], "little") for n in range (0x1000 // 4)]
                        for n in range (len (mem_ints)):
                                if mem_ints[n] == vtbl_ptr:
                                        addr_city = p + 4 * n
                                        if type_tag != self.read_memory (addr_city + 8, 4):
                                                continue
                                        go = self.read_thing (type_tag, addr_city)
                                        tr[go.name] = go
                return tr

        def find_data_objects (self, data_type):
                p = int.from_bytes (self.read_memory (data_type["global_location"], 4), "little")
                tr = {}
                n = 0
                while (True):
                        ut = self.read_data (data_type, p + n * data_type["size"])
                        tr[ut.name] = ut
                        if ut.name == data_type["final_name"]:
                                break
                        n += 1
                return tr

        def find_everything (self):
                print ("Reading units...")
                self.units = self.find_game_things ( b"UNIT")
                print ("Reading cities...")
                self.cities = self.find_game_things (b"CITY")
                print ("Reading unit types...")
                self.unit_types = self.find_data_objects (unit_type)
                print ("Reading difficulties...")
                self.unit_types = self.find_data_objects (difficulty)
