# List all return addresses for a given function
# @author Claude
# @category Analysis
# @keybinding
# @menupath
# @toolbar

from ghidra.program.model.symbol import RefType

def main():
    # Get the current function or prompt user to select one
    current_function = getFunctionContaining(currentAddress)

    if current_function is None:
        print("No function selected. Please place cursor in a function.")
        return

    function_name = current_function.getName()
    function_entry = current_function.getEntryPoint()

    print("Function: {} @ {}".format(function_name, function_entry))
    print("=" * 60)
    print("")

    # Get all references to this function
    references = getReferencesTo(function_entry)

    call_count = 0
    for ref in references:
        ref_type = ref.getReferenceType()

        # Only process CALL references
        if ref_type.isCall():
            call_count += 1
            from_address = ref.getFromAddress()
            caller_function = getFunctionContaining(from_address)

            if caller_function is not None:
                caller_name = caller_function.getName()
            else:
                caller_name = "<unknown>"

            # Get the instruction at the call site
            call_instruction = getInstructionAt(from_address)

            if call_instruction is not None:
                # Return address is the address after the call instruction
                return_address = from_address.add(call_instruction.getLength())

                print("Call Site #{}: {}".format(call_count, from_address))
                print("  Caller Function: {}".format(caller_name))
                print("  Return Address:  {}".format(return_address))
                print("")
            else:
                print("Warning: Could not find instruction at {}".format(from_address))

    print("=" * 60)
    print("Total calls found: {}".format(call_count))

if __name__ == "__main__":
    main()
