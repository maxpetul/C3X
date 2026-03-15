#@category Recovery

from ghidra.program.model.symbol import SymbolType
from ghidra.program.model.data import DataTypeConflictHandler

tool = state.getTool()
pm = tool.getService(ghidra.app.services.ProgramManager)

programs = pm.getAllOpenPrograms()
dst = currentProgram
src = None

for p in programs:
    if p != dst:
        src = p
        break

if src is None:
    raise Exception("Open the corrupted program in the same CodeBrowser tool as the clean program.")

src_sym = src.getSymbolTable()
src_listing = src.getListing()

dst_sym = dst.getSymbolTable()
dst_listing = dst.getListing()
dst_dtm = dst.getDataTypeManager()
dst_funcs = dst.getFunctionManager()

def resolve_dt(dt):
    if dt is None:
        return None
    return dst_dtm.resolve(dt, DataTypeConflictHandler.KEEP_HANDLER)

def to_dst_addr(addr):
    return dst.getAddressFactory().getAddress(str(addr))

tx = dst.startTransaction("Copy global variable data types")
commit = True

copied = 0
skipped = 0
first_error = None

try:
    sym_iter = src_sym.getAllSymbols(False)

    while sym_iter.hasNext():
        try:
            s = sym_iter.next()
            addr = s.getAddress()

            if addr is None or not addr.isMemoryAddress():
                continue

            stype = s.getSymbolType()
            if stype not in (SymbolType.GLOBAL_VAR, SymbolType.LABEL):
                continue

            src_data = src_listing.getDefinedDataAt(addr)
            if src_data is None:
                continue

            dt = src_data.getDataType()
            if dt is None:
                continue

            dst_addr = to_dst_addr(addr)
            if dst_addr is None:
                skipped += 1
                continue

            # Never touch functions/instructions
            if dst_funcs.getFunctionAt(dst_addr) is not None:
                skipped += 1
                continue
            if dst_listing.getInstructionAt(dst_addr) is not None:
                skipped += 1
                continue

            resolved = resolve_dt(dt)
            if resolved is None:
                skipped += 1
                continue

            try:
                length = src_data.getLength()
                if length > 0:
                    dst_end = dst_addr.add(length - 1)
                    clearListing(dst_addr, dst_end)
                createData(dst_addr, resolved)
                copied += 1
            except Exception as e:
                skipped += 1
                if first_error is None:
                    first_error = "createData failed at %s (%s): %s" % (addr, s.getName(), e)

        except Exception as e:
            skipped += 1
            if first_error is None:
                first_error = "Per-symbol failure: %s" % e

except Exception as e:
    commit = False
    print("Fatal error: %s" % e)
    raise

finally:
    dst.endTransaction(tx, commit)

print("Global data definitions copied: %d" % copied)
print("Skipped: %d" % skipped)
if first_error:
    print("First error: %s" % first_error)
print("Done.")