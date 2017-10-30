import os
import json
import textwrap

import krait
from krait.__internal import compile_imports


_compiled_marker = compile_imports.CompiledImportHook.tag_marker
_controller_storage = []


def compile_routes(routes):
    mvc_dir = os.path.normpath(krait.get_full_path(".compiled/_krait_compiled/_mvc_compiled"))
    prepare_directory(mvc_dir)
    for idx, route in enumerate(routes):
        if route.ctrl_class is not None:
            compiled_filename = get_filename(mvc_dir, idx, route)
            compile_route(route, compiled_filename)


def push_controller(ctrl):
    _controller_storage.append(ctrl)
    return len(_controller_storage) - 1


def get_controller(tag):
    return _controller_storage[tag]


def prepare_directory(directory):
    if os.path.exists(directory):
        rm_tree(directory)

    os.makedirs(directory, 0o775)

    init_file = os.path.join(directory, "__init__.py")
    with open(init_file, "w") as f:
        f.write(_compiled_marker + json.dumps({
            "custom": True
        }))
        f.write('\n')


def rm_tree(directory):
    for entry in os.listdir(directory):
        full_path = os.path.join(directory, entry)
        if os.path.isdir(full_path):
            rm_tree(full_path)
        else:
            os.remove(full_path)

    os.rmdir(directory)


# noinspection PyUnusedLocal
def get_filename(directory, index, route):
    return os.path.join(directory, "mvc_compiled_{}.py".format(index))


def compile_route(route, filename):
    ctrl_tag = push_controller(route.ctrl_class)

    with open(filename, "w") as f:
        f.write(_compiled_marker + json.dumps({
            "custom": True
        }))
        f.write('\n')

        f.write(textwrap.dedent(
            """\
            import krait
            from krait.__internal import mvc_compiler
            from krait.__internal import _compiled_run, _compiled_convert_filename
            """
        ))
        f.write(textwrap.dedent(
            """\
            _ctrl_class = mvc_compiler.get_controller({!r})
            """.format(ctrl_tag)
        ))
        f.write(textwrap.dedent(
            """\
            def run():
                ctrl = krait.mvc.push_ctrl(_ctrl_class())
                _compiled_run(_compiled_convert_filename(krait.get_full_path(ctrl.get_view())))
            """
        ))
