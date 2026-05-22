#@category Recovery

from ghidra.program.model.symbol import SourceType
from ghidra.program.model.listing import ParameterImpl
from ghidra.program.model.listing.Function import FunctionUpdateType
from ghidra.program.model.data import DataTypeConflictHandler, PointerDataType
from ghidra.program.model.address import AddressSet

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

dst_funcs = dst.getFunctionManager()
src_funcs = src.getFunctionManager()
dst_sym = dst.getSymbolTable()
dst_dtm = dst.getDataTypeManager()

def resolve_dt(dt):
    if dt is None:
        return None
    return dst_dtm.resolve(dt, DataTypeConflictHandler.KEEP_HANDLER)

def clone_body_to_dst(src_body):
    body = AddressSet()
    rng_iter = src_body.getAddressRanges()
    while rng_iter.hasNext():
        r = rng_iter.next()
        body.addRange(r.getMinAddress(), r.getMaxAddress())
    return body

def get_child_namespace(parent_ns, name):
    syms = dst_sym.getSymbols(name, parent_ns)
    for s in syms:
        obj = s.getObject()
        if obj is not None:
            return obj
    return None

def ensure_parent_namespace(src_ns):
    """
    Recreate the full parent chain from src in dst.
    Convert existing plain namespaces to classes when src says they are classes.
    """
    if src_ns is None or src_ns.isGlobal():
        return dst.getGlobalNamespace()

    parent_dst = ensure_parent_namespace(src_ns.getParentNamespace())
    name = src_ns.getName()
    existing = get_child_namespace(parent_dst, name)

    src_type = None
    try:
        src_type = str(src_ns.getType())
    except:
        src_type = "NAMESPACE"

    if existing is None:
        if src_type == "CLASS":
            return dst_sym.createClass(parent_dst, name, SourceType.USER_DEFINED)
        return dst_sym.createNameSpace(parent_dst, name, SourceType.USER_DEFINED)

    try:
        existing_type = str(existing.getType())
    except:
        existing_type = "NAMESPACE"

    if src_type == "CLASS" and existing_type != "CLASS":
        try:
            return dst_sym.convertNamespaceToClass(existing)
        except:
            # If conversion fails for some weird edge case, keep the existing namespace.
            return existing

    return existing

# Build a quick datatype lookup by simple name for fixing "this"
class_dt_by_name = {}
try:
    dt_iter = dst_dtm.getAllDataTypes()
    while dt_iter.hasNext():
        dt = dt_iter.next()
        try:
            n = dt.getName()
            if n not in class_dt_by_name:
                class_dt_by_name[n] = dt
        except:
            pass
except:
    pass

tx = dst.startTransaction("Recover functions/signatures/classes")
commit = True

processed = 0
created = 0
updated = 0
skipped = 0
first_error = None

try:
    func_iter = src_funcs.getFunctions(True)

    while func_iter.hasNext():
        try:
            sf = func_iter.next()
            entry = sf.getEntryPoint()
            processed += 1

            df = dst_funcs.getFunctionAt(entry)

            # Create function if missing
            if df is None:
                try:
                    body = clone_body_to_dst(sf.getBody())
                    df = dst_funcs.createFunction(sf.getName(), entry, body, SourceType.USER_DEFINED)
                    created += 1
                except Exception as e:
                    skipped += 1
                    if first_error is None:
                        first_error = "Create failed at %s (%s): %s" % (entry, sf.getName(), e)
                    continue

            # Recreate class / namespace chain
            class_name_for_this = None
            try:
                src_parent = sf.getParentNamespace()
                if src_parent is not None and not src_parent.isGlobal():
                    dst_parent = ensure_parent_namespace(src_parent)
                    try:
                        if str(src_parent.getType()) == "CLASS":
                            class_name_for_this = src_parent.getName()
                    except:
                        pass

                    base_name = sf.getName().split("::")[-1]
                    df.setName(base_name, SourceType.USER_DEFINED)
                    df.setParentNamespace(dst_parent)
                else:
                    df.setName(sf.getName(), SourceType.USER_DEFINED)
            except Exception as e:
                if first_error is None:
                    first_error = "Namespace/class placement failed at %s (%s): %s" % (entry, sf.getName(), e)

            # Calling convention
            try:
                cc = sf.getCallingConventionName()
                if cc is not None and cc != "":
                    df.setCallingConvention(cc)
            except Exception as e:
                if first_error is None:
                    first_error = "Calling convention failed at %s (%s): %s" % (entry, sf.getName(), e)

            # Build resolved parameter list
            try:
                new_params = []
                src_params = list(sf.getParameters())

                for i, sp in enumerate(src_params):
                    dt = resolve_dt(sp.getDataType())

                    # Fix "this" if it is still void* and we have a matching class datatype
                    try:
                        if (class_name_for_this is not None and
                            sp.getName() == "this" and
                            str(dt) == "void *" and
                            class_name_for_this in class_dt_by_name):
                            dt = PointerDataType(resolve_dt(class_dt_by_name[class_name_for_this]))
                    except:
                        pass

                    p = ParameterImpl(sp.getName(), dt, dst)
                    new_params.append(p)

                df.replaceParameters(
                    new_params,
                    FunctionUpdateType.DYNAMIC_STORAGE_ALL_PARAMS,
                    True,
                    SourceType.USER_DEFINED
                )
            except Exception as e:
                skipped += 1
                if first_error is None:
                    first_error = "Parameter replace failed at %s (%s): %s" % (entry, sf.getName(), e)
                continue

            # Return type last
            try:
                ret_dt = resolve_dt(sf.getReturnType())
                if ret_dt is not None:
                    df.setReturnType(ret_dt, SourceType.USER_DEFINED)
            except Exception as e:
                if first_error is None:
                    first_error = "Return type failed at %s (%s): %s" % (entry, sf.getName(), e)

            updated += 1

        except Exception as e:
            skipped += 1
            if first_error is None:
                first_error = "Top-level per-function failure: %s" % e

except Exception as e:
    commit = False
    print("Fatal error: %s" % e)
    raise

finally:
    dst.endTransaction(tx, commit)

print("Processed: %d" % processed)
print("Created:   %d" % created)
print("Updated:   %d" % updated)
print("Skipped:   %d" % skipped)
if first_error:
    print("First error: %s" % first_error)
print("Done.")