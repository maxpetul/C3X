# ExportProgForPython.py
# Exports all functions from a Ghidra project to a JSON file for easy consumption by Python scripts
#@author Claude
#@category Export
#@keybinding ctrl shift E
#@menupath Tools.Export.Functions For Python
#@toolbar 

import json
import os
from ghidra.program.model.listing import CodeUnit
from ghidra.util.task import TaskMonitor

def get_function_bytes(func):
    """Gets the raw bytes of a function"""
    result = []
    minAddr = func.getEntryPoint()
    maxAddr = func.getBody().getMaxAddress()
    
    currentAddr = minAddr
    while currentAddr <= maxAddr and currentAddr is not None:
        if func.getBody().contains(currentAddr):
            byte_value = getByte(currentAddr)
            result.append(byte_value & 0xff)  # Convert to unsigned byte
        currentAddr = currentAddr.next()
    
    return result

def get_function_disassembly(func):
    """Gets the disassembled instructions of a function"""
    listing = currentProgram.getListing()
    disasm = []
    
    minAddr = func.getEntryPoint()
    maxAddr = func.getBody().getMaxAddress()
    
    currentAddr = minAddr
    while currentAddr <= maxAddr and currentAddr is not None:
        if func.getBody().contains(currentAddr):
            instr = listing.getCodeUnitAt(currentAddr)
            if instr is not None:
                disasm.append({
                    "address": str(instr.getAddress()),
                    "instruction": instr.toString()
                })
                if instr.getLength() > 1:
                    currentAddr = currentAddr.add(instr.getLength() - 1)
        currentAddr = currentAddr.next()
    
    return disasm

def count_references(func):
    """Counts the references to a function"""
    ref_count = 0
    refManager = currentProgram.getReferenceManager()
    references = refManager.getReferencesTo(func.getEntryPoint())
    
    for ref in references:
        if ref.getReferenceType().isCall() or ref.getReferenceType().isJump():
            ref_count += 1
    
    return ref_count

def run():
    output_file = askFile("Select Output File", "Save")
    
    monitor.setMessage("Exporting functions...")
    
    # Get all functions
    function_manager = currentProgram.getFunctionManager()
    functions = function_manager.getFunctions(True)  # True to get all functions
    
    results = []
    
    for func in functions:
        monitor.setMessage("Processing " + func.getName())
        
        # Get function details
        try:
            function_data = {
                "name": func.getName(),
                "address": str(func.getEntryPoint()),
                "raw_bytes": get_function_bytes(func),
                "disassembly": get_function_disassembly(func),
                "reference_count": count_references(func)
            }
        except:
            print("Error processing function at {}".format(func.getEntryPoint()))
            continue
        
        results.append(function_data)
        
        # Check if the task has been cancelled
        if monitor.isCancelled():
            break
    
    # Write the results to a JSON file
    with open(output_file.getAbsolutePath(), 'w') as f:
        json.dump(results, f, indent=2)
    
    print("Exported {} functions to {}".format(len(results), output_file.getAbsolutePath()))

# Run the script
run()
