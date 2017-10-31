from __future__ import print_function
import sys
import os
import imp
import json

import krait
from krait import __internal as krait_internal


def print_flush(*args, **kwargs):
    print(*args, **kwargs)
    sys.stdout.flush()


class CompiledImportHook(object):
    tag_marker = "# [TAGS]: "

    def __init__(self):
        self.compiled_dir = os.path.normpath(krait.get_full_path(".compiled/_krait_compiled"))
        self.init_version = None
        self.compiler_version = 1

    def prepare(self):
        init_file = os.path.join(self.compiled_dir, "__init__.py")
        if os.path.exists(init_file):
            os.remove(init_file)

        dir_name = os.path.dirname(init_file)
        if not os.path.exists(dir_name):
            os.makedirs(dir_name, 0o775)

        with open(init_file, "w") as f:
            f.write(self.tag_marker + json.dumps({
                "c_version": self.compiler_version
            }))
            f.write('\n')

    def find_module(self, fullname, path=None):
        """
        Start the import machinery to get compiled Python module objects.
        This finds the modules, compiles them if they don't exist, and kicks off the import machinery.
        Also, reloads the modules if those have changed.

        Only works if the fullname is the .compiled directory, or the path is.

        Args:
            fullname: The __converted__ path to the desired file, or ``_krait_compiled_dir``.
            path: Either None (for ``_krait_compiled_dir``, or the path of that directory.

        Returns:
            :class:`CompiledLoader`, optional: The loader for the module, or None.
        """

        print_flush("-------------------in find_module; fullname={}, path={}, self.compiled_dir={}".format(
            fullname, path, self.compiled_dir
        ))

        if fullname == "_krait_compiled":
            print_flush("--OK for _krait_compiled package")
            # Is the main package.
            filename = os.path.join(self.compiled_dir, "__init__.py")
            # Should already be package.

            return CompiledImportHook.Loader(self, fullname, self.find_module_from_package(filename))
        elif path is not None\
                and len(path) == 1\
                and os.path.normpath(path[0]) == self.compiled_dir:
            # Is something under the main package

            _, _, mod_name = fullname.rpartition('.')
            if CompiledImportHook.is_package(os.path.join(self.compiled_dir, mod_name)):
                print_flush("--OK for package {} (full {})".format(mod_name, fullname))

                # Is a package inside _krait_compiled
                filename = os.path.join(self.compiled_dir, mod_name, "__init__.py")

                return CompiledImportHook.Loader(self, fullname, self.find_module_from_package(filename))
            else:
                print_flush("--OK for module {} (full {})".format(mod_name, fullname))

                # Is a module that maybe needs to be compiled
                filename = self.get_compile(fullname)
                return CompiledImportHook.Loader(self, fullname, self.find_module_from_filename(filename))
        else:
            print_flush("---Not found.")

            return None

    @staticmethod
    def find_module_from_package(init_filename):
        # init_filename MUST be 1) absolute 2) in form {dir}/__init__.py
        return [None, os.path.dirname(init_filename), ('', '', imp.PKG_DIRECTORY)]

    @staticmethod
    def find_module_from_filename(mod_filename):
        # mod_filename MUST be absolute! And NOT contain .py
        mod_path, mod_name = os.path.split(mod_filename)
        return imp.find_module(mod_name, [mod_path])

    @staticmethod
    def is_package(directory):
        return os.path.isdir(directory) and \
               os.path.exists(os.path.join(directory, "__init__.py"))

    def get_compiler_version(self, filename):
        tag = self.get_compiled_tag(filename)
        # Tag may be None, short-circuit this situation
        return tag and tag.get("c_version")

    def get_compiled_etag(self, filename):
        tag = self.get_compiled_tag(filename)
        # Tag may be None, short-circuit this situation
        return tag and tag.get("etag")

    def get_compiled_tag(self, filename):
        if not os.path.exists(filename):
            return None

        with open(filename, "r") as f:
            line = f.readline()

        if not line.startswith(self.tag_marker):
            raise ValueError("Missing tag on compiled filename.")

        return json.loads(line[len(self.tag_marker):])

    def get_compile(self, fullname):
        pk_name, _, mod_name = fullname.partition('.')
        if pk_name != "_krait_compiled" or '.' in mod_name:
            raise ValueError("Non-compile fullname in CompiledImportHook.get_compile")

        mod_filename = os.path.join(self.compiled_dir, mod_name)
        py_filename = mod_filename + ".py"

        # noinspection PyProtectedMember
        if os.path.exists(py_filename) and\
                krait_internal._compiled_check_tag(mod_name, self.get_compiled_etag(py_filename)):
            # Everything up to date
            return mod_filename

        # Create the file, but don't return the extension
        # noinspection PyProtectedMember
        return os.path.splitext(krait_internal._compiled_get_compiled_file(mod_name))[0]

    class Loader(object):
        def __init__(self, hooker_object, fullname, find_module_result):
            self.hooker_object = hooker_object
            self.fullname = fullname
            self.file, self.pathname, self.description = find_module_result

        def load_module(self, fullname):
            if fullname != self.fullname:
                raise ValueError("CompiledImportHook.Loader reused.")

            try:
                mod = imp.load_module(fullname, self.file, self.pathname, self.description)
                mod.__loader__ = self
                return mod
            except StandardError as ex:
                print_flush("---Error!:", ex)
                raise
            finally:
                if self.file is not None:
                    self.file.close()
                    self.file = None

        def check_tag_or_reload(self):
            # noinspection PyProtectedMember
            krait_internal._compiled_check_tag_or_reload(
                self.fullname.rpartition('.')[2],
                self.hooker_object.get_compiled_etag(self.pathname))

            # This raises an exception if the module was reloaded, or continues otherwise


def register():
    hook = CompiledImportHook()
    hook.prepare()
    sys.meta_path.append(hook)

