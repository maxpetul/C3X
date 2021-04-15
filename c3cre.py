
import win32api as winapi
import win32process as winproc
import win32event as winev
import msvcrt

from commonre import *

go_to_conquests_directory ()

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
                

                # HERE'S WHAT I DISCOVERED AFTER A DAY OF STRUGGLE:
                # Allocating memory in the proc's address space THEN inserting the detours *doesn't work*. In fact it fails very
                # badly, resulting in strange behaviour, memory corruption, etc. I think this is a bug b/c of this behaviour but
                # it might also be antivirus interfering. Anyway, the solution seems to be to simply do the operations in reverse,
                # i.e., first insert the detours (to nowhere) then create the patched functions underneath those locations. Problem
                # is that if this is really a bug it might reappear somewhere. I suspect the bug might be in pywin32 or PyArg but
                # I would have to do more investigating to be sure. One simple way to test is to implement the same thing in C and
                # see if the bug appears there too (I guess it could also be a bug in Windows itself but I think that's pretty
                # unlikely for such a common use case).


                # Apply rush-in-disorder patch
                # bytes_written = self.write_memory (0x4B52C4, b"\xEB")
                # print ("bytes_written for rush-in-disorder patch: " + str (bytes_written))

                todet_addr = 0x4b5290
                todet_size = 0x4b5c91 - todet_addr
                MEM_RESERVE = 0x2000
                MEM_COMMIT = 0x1000
                PAGE_EXECUTE_READWRITE = 0x40

                # self.memcpy (det_loc, todet_addr, todet_size)
                # write_success = self.write_memory (todet_addr, b"\x68\x00\x00\xBC\x2A\xC3")  # push ... ret   doesn't work I think b/c of the code segment
                # clobbered = self.read_memory (todet_addr, 5)
                # print ("bytes saved before clobber: " + str (len (clobbered)))
                # towrite = b"\xAB\xCD\xEF\x12\x34"
                # bytes_written = self.write_memory (todet_addr, towrite) # b"\xE9\x6B\xAD\x70\x2A")
                # print ("bytes_written for detour: " + str (bytes_written))

                # Set up detour
                # det_loc = winproc.VirtualAllocEx (self.proc_handle, 0x2ABC0000, todet_size + 0x1000, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE)
                # print ("Got memory at: " + hex (det_loc))
                # if det_loc != 0x2ABC0000:
                #         raise Exception ("Failed to allocate memory for detoured functions")

                # self.memcpy (0x2ABC0000 + 5, todet_addr + 5, todet_size - 5)
                # ww = self.write_memory (0x2ABC0000, clobbered)
                # print ("Wrote " + str (ww) + " bytes to unclobber target")
                
                # TODO: IMPORTANT!!!
                # The write to the original function doesn't actually take for some reason. But doing it a second time works. Very strange.
                # Investigate this some more. We might have to do something stupid like unsuspend then resuspend the proc just so we can get it
                # in a state where the write will go through.

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

game_object_sizes = {b"CITY": 0x544, b"UNIT": 0x278} # TODO: Try to find the Unit's actual size by finding a call to new

# global_location is the address of the global variable that stores the pointer to the array of data objects.
# The final_name field is just where to stop reading since I don't know where to find the lengths of these arrays in most cases.
unit_template = {"global_location": 0x9C71E0, "size": 0x138, "name_offset": 8, "name_size": 32, "final_name": "Mobile SAM"}
difficulty    = {"global_location": 0x9C40C0, "size": 0x7C , "name_offset": 4, "name_size": 64, "final_name": "Sid"}

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

def read_thing (civ_proc, type_tag, address):
        tr = Unit () if type_tag == b"UNIT" else GameObject ()
        global unnamed_unit_counter
        tr.civ_proc = civ_proc
        tr.type_tag = type_tag
        tr.address = address
        tr.size = game_object_sizes[type_tag]
        tr.mem_bytes = civ_proc.read_memory (address, tr.size)
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

def read_data (civ_proc, data_type, address):
        tr = GameObject ()
        tr.civ_proc = civ_proc
        tr.address = address
        tr.size = data_type["size"]
        tr.mem_bytes = civ_proc.read_memory (address, tr.size)
        tr.name = str_from_bytes (tr.mem_bytes[data_type["name_offset"] : data_type["name_offset"] + data_type["name_size"]])
        return tr

vtable_pointers = {b"CITY": 0x66dc78, b"UNIT": 0x66DCF0}

def find_game_things (civ_proc, type_tag):
        vtbl_ptr = vtable_pointers.get (type_tag)
        if vtbl_ptr is None:
                print ("Type tag \"" + type_tag + "\" not recognized")
                return []
        tr = {}
        for p in civ_proc.find_mapped_pages ():
                mem_bytes = civ_proc.read_memory (p, 0x1000)
                mem_ints = [int.from_bytes (mem_bytes[4*n : 4*n+4], "little") for n in range (0x1000 // 4)]
                for n in range (len (mem_ints)):
                        if mem_ints[n] == vtbl_ptr:
                                addr_city = p + 4 * n
                                if type_tag != civ_proc.read_memory (addr_city + 8, 4):
                                        continue
                                go = read_thing (civ_proc, type_tag, addr_city)
                                tr[go.name] = go
        return tr

def find_data_objects (civ_proc, data_type):
        p = int.from_bytes (civ_proc.read_memory (data_type["global_location"], 4), "little")
        tr = {}
        n = 0
        while (True):
                ut = read_data (civ_proc, data_type, p + n * data_type["size"])
                tr[ut.name] = ut
                if ut.name == data_type["final_name"]:
                        break
                n += 1
        return tr

def fit_offset (game_objects, observations, size=4):
        """ Finds offsets whose values match all observations. Observations is a list of (name, value) tuples """
        go0, val0 = observations[0]
        tr = game_objects[go0].get_offsets (val0, size)
        for go, val in observations[1:]:
                tr = [v for v in game_objects[go].get_offsets (val, size) if v in tr]
        return tr
