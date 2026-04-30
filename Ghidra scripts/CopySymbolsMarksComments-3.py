#@category Recovery

from ghidra.program.model.symbol import SourceType, SymbolType
from ghidra.program.model.listing import CodeUnit

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
dst_sym = dst.getSymbolTable()

src_listing = src.getListing()
dst_listing = dst.getListing()

src_bm = src.getBookmarkManager()
dst_bm = dst.getBookmarkManager()

def same_program_addr(addr_str):
    return dst.getAddressFactory().getAddress(str(addr_str))

def is_good_memory_addr(addr):
    try:
        return addr is not None and addr.isMemoryAddress()
    except:
        return False

def ensure_label(addr, name, namespace, make_primary):
    """
    Create or reuse a label at addr/name[/namespace] in dst.
    """
    existing = None

    try:
        syms = dst_sym.getSymbols(addr)
        for s in syms:
            try:
                if s.getName() != name:
                    continue

                s_ns = s.getParentNamespace()
                if namespace is None:
                    if s_ns is None or s_ns.isGlobal():
                        existing = s
                        break
                else:
                    if s_ns is not None and s_ns.getName() == namespace.getName():
                        existing = s
                        break
            except:
                pass
    except:
        pass

    if existing is None:
        if namespace is None:
            existing = dst_sym.createLabel(addr, name, SourceType.USER_DEFINED)
        else:
            existing = dst_sym.createLabel(addr, name, namespace, SourceType.USER_DEFINED)

    if make_primary:
        try:
            existing.setPrimary()
        except:
            pass

    return existing

def comment_iter(comment_type):
    try:
        return src_listing.getCommentCodeUnitIterator(comment_type, src.getMemory())
    except:
        # fallback for API differences
        prop = None
        if comment_type == CodeUnit.EOL_COMMENT:
            prop = CodeUnit.EOL_COMMENT
        elif comment_type == CodeUnit.PRE_COMMENT:
            prop = CodeUnit.PRE_COMMENT
        elif comment_type == CodeUnit.POST_COMMENT:
            prop = CodeUnit.POST_COMMENT
        elif comment_type == CodeUnit.PLATE_COMMENT:
            prop = CodeUnit.PLATE_COMMENT
        elif comment_type == CodeUnit.REPEATABLE_COMMENT:
            prop = CodeUnit.REPEATABLE_COMMENT
        else:
            raise
        return src_listing.getCodeUnitIterator(prop, src.getMemory(), True)

tx = dst.startTransaction("Copy labels/comments/bookmarks")
commit = True

label_count = 0
comment_count = 0
bookmark_count = 0
skipped = 0
first_error = None

try:
    #
    # 1) Global labels / globals
    #
    # We only copy symbols at memory addresses and skip functions, params, locals, namespaces, etc.
    #
    sym_iter = src_sym.getAllSymbols(False)

    while sym_iter.hasNext():
        try:
            s = sym_iter.next()
            addr = s.getAddress()

            if not is_good_memory_addr(addr):
                continue

            stype = s.getSymbolType()
            if stype not in (SymbolType.LABEL, SymbolType.GLOBAL_VAR):
                continue

            dst_addr = same_program_addr(addr)
            if dst_addr is None:
                skipped += 1
                continue

            name = s.getName()
            parent_ns = s.getParentNamespace()
            use_ns = None

            try:
                if parent_ns is not None and not parent_ns.isGlobal():
                    # Only reuse an existing namespace/class if it already exists in dst.
                    # This avoids recreating odd things unexpectedly.
                    use_ns = dst_sym.getNamespace(parent_ns.getName(), dst.getGlobalNamespace())
            except:
                use_ns = None

            ensure_label(dst_addr, name, use_ns, s.isPrimary())
            label_count += 1

        except Exception as e:
            skipped += 1
            if first_error is None:
                first_error = "Label copy failed: %s" % e

    #
    # 2) Comments
    #
    # Iterate by comment property rather than walking every address.
    #
    comment_types = [
        CodeUnit.PLATE_COMMENT,
        CodeUnit.PRE_COMMENT,
        CodeUnit.EOL_COMMENT,
        CodeUnit.POST_COMMENT,
        CodeUnit.REPEATABLE_COMMENT
    ]

    for ctype in comment_types:
        try:
            it = comment_iter(ctype)
            while it.hasNext():
                try:
                    cu = it.next()
                    addr = cu.getMinAddress()
                    if not is_good_memory_addr(addr):
                        continue

                    text = cu.getComment(ctype)
                    if text is None:
                        continue

                    dst_addr = same_program_addr(addr)
                    if dst_addr is None:
                        skipped += 1
                        continue

                    dst_listing.setComment(dst_addr, ctype, text)
                    comment_count += 1

                except Exception as e:
                    skipped += 1
                    if first_error is None:
                        first_error = "Comment copy failed: %s" % e
        except Exception as e:
            skipped += 1
            if first_error is None:
                first_error = "Comment iterator failed: %s" % e

    #
    # 3) Bookmarks
    #
    try:
        bm_iter = src_bm.getBookmarksIterator()
    except:
        bm_iter = None

    if bm_iter is not None:
        while bm_iter.hasNext():
            try:
                bm = bm_iter.next()
                addr = bm.getAddress()

                if not is_good_memory_addr(addr):
                    continue

                dst_addr = same_program_addr(addr)
                if dst_addr is None:
                    skipped += 1
                    continue

                btype = bm.getTypeString()
                cat = bm.getCategory()
                cmt = bm.getComment()

                try:
                    dst_bm.setBookmark(dst_addr, btype, cat, cmt)
                    bookmark_count += 1
                except Exception as e:
                    skipped += 1
                    if first_error is None:
                        first_error = "Bookmark set failed: %s" % e

            except Exception as e:
                skipped += 1
                if first_error is None:
                    first_error = "Bookmark copy failed: %s" % e

except Exception as e:
    commit = False
    print("Fatal error: %s" % e)
    raise

finally:
    dst.endTransaction(tx, commit)

print("Labels copied:    %d" % label_count)
print("Comments copied:  %d" % comment_count)
print("Bookmarks copied: %d" % bookmark_count)
print("Skipped:          %d" % skipped)
if first_error:
    print("First error: %s" % first_error)
print("Done.")