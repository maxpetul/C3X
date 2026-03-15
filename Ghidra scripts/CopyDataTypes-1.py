#@category Recovery

from ghidra.program.model.data import DataTypeConflictHandler

tool = state.getTool()
pm = tool.getService(ghidra.app.services.ProgramManager)

programs = pm.getAllOpenPrograms()

dst = currentProgram
src = None

for p in programs:
    if p != dst:
        src = p

if src is None:
    print("Need both programs open.")
    exit()

srcDTM = src.getDataTypeManager()
dstDTM = dst.getDataTypeManager()

print("Copying datatypes...")

tx = dstDTM.startTransaction("datatype copy")

count = 0

for dt in srcDTM.getAllDataTypes():

    try:
        dstDTM.addDataType(
            dt,
            DataTypeConflictHandler.REPLACE_HANDLER
        )
        count += 1

    except Exception as e:
        print("Skipped:", dt.getName())

dstDTM.endTransaction(tx, True)

print("Copied", count, "datatypes")
print("Done.")