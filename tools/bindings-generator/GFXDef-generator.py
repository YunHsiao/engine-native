
from clang.cindex import Index, CursorKind, Config
from pprint import pprint
import os

def get_diag_info(diag):
    return {
        # 'severity' : diag.severity,
        'location' : diag.location,
        # 'ranges' : diag.ranges,
        # 'fixits' : diag.fixits,
        'spelling' : diag.spelling
    }

def get_info(cursor, depth=0):
    # if (cursor.kind != CursorKind.TRANSLATION_UNIT and
    #     cursor.kind != CursorKind.NAMESPACE and
    #     cursor.kind != CursorKind.ENUM_CONSTANT_DECL and
    #     cursor.kind != CursorKind.ENUM_DECL and
    #     cursor.kind != CursorKind.FIELD_DECL and
    #     cursor.kind != CursorKind.CLASS_DECL and
    #     cursor.kind != CursorKind.STRUCT_DECL):
    #     return None

    if depth >= 4:
        children = None
    else:
        children = [get_info(c, depth + 1)
                    for c in cursor.get_children()]

    return { 'kind' : cursor.kind,
             'usr' : cursor.get_usr(),
             'spelling' : cursor.spelling,
             'location' : cursor.location,
             'extent.start' : cursor.extent.start,
             'extent.end' : cursor.extent.end,
             'is_definition' : cursor.is_definition(),
             'children' : children }

def main():
    cur_dir = os.path.abspath(os.path.dirname(__file__))
    Config.set_library_path(os.path.join(cur_dir, "libclang"))
    index = Index.create()

    args = ["-x", "c++", "-std=c++17"]
    tu = index.parse(os.path.join(cur_dir, "GFXDef-entry.h"), args)
    # pprint(('diags', [get_diag_info(d) for d in tu.diagnostics]))
    pprint(('nodes', get_info(tu.cursor)))

if __name__ == '__main__':
    main()
