
import os
from datetime import datetime
from itertools import chain
from random import shuffle

mode = "show_parameters" # must equal "record_object_types" or "show_parameters"

gdb.execute ("set disassembly-flavor intel")
gdb.execute ("set pagination off")
gdb.execute ("set print thread-events off")

# class GameObject:
#         def __init__ (self, vptr, type_tag):
#                 self.vptr = vptr
#                 self.type_tag = type_tag
#                 self.encoded_tag = int.from_bytes (type_tag.encode ("cp1252"), byteorder="little")

# game_objects = [
#         GameObject (0x66DCF0, "UNIT"),
#         GameObject (0x66CB38, "LEAD"),
#         GameObject (0x668F74, "DATE"),
#         GameObject (0x668F8C, "CTPG"),
#         GameObject (0x669044, "BINF"),
#         GameObject (0x66902C, "POPD"),
#         GameObject (0x669018, "BITM"),
#         GameObject (0x66DC78, "CITY")
#         ]

def read_locations_from_file (filename):
        with open (filename, "r") as f:
                lines = f.read ().split ("\n")
        return [L for L in lines if len (L) > 0] # Drop empty lines

def save_memo ():
        f.seek (0)
        f.write ("probe_results = {\n" + ",\n\t".join ([L + ": " + str (memo[L]) for L in locations]) + "\n}")

class HitRateTracker:
        def __init__ (self):
                self.hits = []

        def hit_now (self):
                self.hits.append (datetime.now ())

        def compute_hit_rate (self):
                if len (self.hits) == 0:
                        return 0

                # Compute rate, simply num hits per second
                now = datetime.now ()
                tr = len (self.hits) / max (1, (now - self.hits[0]).seconds)

                # Drop hits recorded more than one minute ago
                i_first_within_min = 0
                while True:
                        if i_first_within_min >= len (self.hits):
                                break
                        elif (now - self.hits[i_first_within_min]).seconds <= 60:
                                break
                        else:
                                i_first_within_min += 1
                self.hits = self.hits[i_first_within_min:]

                return tr

if mode == "record_object_types":
        f = open ("C3X\\param_types_" + datetime.now ().strftime ("%d-%m_%M-%H") + ".csv", "w+")

        # Sentry breakpoint is located on check_hurry_production  b/c that often gets called for the AI during its turn
        # The purpose of the sentry is to ensure that on_break gets called on a regular basis regardless of what
        # other breakpoints are enabled.
        sentry_location = "4b5290"
        sentry_breakpoint = gdb.Breakpoint ("*0x" + sentry_location)
        sentry_breakpoint.commands = "silent\ncont"

        known_locations = read_locations_from_file ("C3X\\type_known_locations.txt")
        locations = [L for L in read_locations_from_file ("C3X\\type_probe_locations.txt") if L != sentry_location and L not in known_locations]

        memo = {}
        for L in locations:
                memo[L] = []

        # Create breakpoints, all disabled to start
        disabled_locations = {}
        for L in locations:
                b = gdb.Breakpoint ("*0x" + L)
                b.commands = "silent\ncont"
                b.enabled = False
                disabled_locations[L] = b

        time_of_last_save = datetime.now ()

        hrt = HitRateTracker ()

def record_object_type (event):
        global hrt, disabled_locations, time_of_last_save
        
        # Check that we actually hit a breakpoint b/c this function will get called for any "stop event"
        if not hasattr (event, "breakpoint"):
                return

        location = event.breakpoint.location[3:] # Discard the first three chars b/c they're "*0x"
        
        if location != sentry_location:
                # Detect and record the type of whatever ECX points to
                try:
                        type_tag = int (gdb.parse_and_eval ("*(int *)($ecx + 8)")).to_bytes (4, "little")
                        found_it = False
                        for m in memo[location]:
                                if m[0] == type_tag:
                                        memo[location][1] += 1
                                        found_it = True
                                        break
                        if not found_it:
                                memo[location].append ((type_tag, 1))
                except:
                        pass
                        
                # tag_index = len (game_objects) # Default type is "N/A" indicated by index beyond the array
                # try:
                #         vtbl = int (gdb.parse_and_eval ("*(int *)$ecx"))
                #         for n in range (len (game_objects)):
                #                 g = game_objects[n]
                #                 if vtbl == g.vptr:
                #                         type_tag = int (gdb.parse_and_eval ("*(int *)($ecx + 8)"))
                #                         if type_tag == g.encoded_tag:
                #                                 tag_index = n
                #                                 break
                # except:
                #         pass
                # memo[location][tag_index] += 1
                
                # Record hit, disable this breakpoint
                hrt.hit_now ()
                event.breakpoint.enabled = False
                disabled_locations[location] = event.breakpoint

        # Save data so far
        now = datetime.now ()
        if ((now - time_of_last_save).seconds > 60):
                save_memo ()
                time_of_last_save = now

        # Enable additional breakpoints if the hitrate is too low
        if hrt.compute_hit_rate () < 1.0:
                disableds = list (disabled_locations.keys ())
                shuffle (disableds)
                for L in disableds[:10]:
                        disabled_locations[L].enabled = True
                        disabled_locations.pop (L)

known_sigs = {"5bb662": {"name": "is_visible_to_civ", "convention": "thiscall", "stack_params": 2}}

def show_parameters (event):
        global known_sigs
        if not hasattr (event, "breakpoint"):
                return
        location = event.breakpoint.location[3:]
        if location not in known_sigs:
                return

        sig = known_sigs[location]
        if not ((sig["convention"] == "thiscall") or (sig["convention"] == "cdecl")):
                raise Exception ("calling convention not implemented")
        ecx = hex (int (gdb.parse_and_eval ("*(int *)$ecx"))) if sig["convention"] == "thiscall" else "\t"
        cols = [sig["name"], ecx]
        for n in range (sig["stack_params"]):
                cols.append (str (int (gdb.parse_and_eval ("*(int *)($esp + " + str (4*n + 4) + ")"))))
        print ("\t".join (cols))

if mode == "record_object_types":
        gdb.events.stop.connect (record_object_types)
elif mode == "show_parameters":
        print ("name\tecx\tstack")
        b = gdb.Breakpoint ("*0x5bb662")
        b.commands = "silent\ncont"
        gdb.events.stop.connect (show_parameters)
