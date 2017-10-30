import sys
import os
import imp
import json

import krait
from krait import __internal as krait_internal


class CompiledImportHook(object):
    tag_marker = "# [TAGS]: "

    def __init__(self):
        self.compiled_dir = os.path.normpath(krait.get_full_path(".compiled/_krait_compiled"))
        self.init_version = None
        self.compiler_version = 1

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
        if fullname == "_krait_compiled":
            # Is the main package.
            filename = os.path.join(self.compiled_dir, "__init__")
            self.ensure_compiled_init(fullname, filename + ".py")

            return CompiledImportHook.Loader(self, fullname, self.find_module_from_package(filename))
        elif path is not None\
                and len(path) == 1\
                and os.path.normpath(path[0]) == self.compiled_dir:
            # Is something under the main package

            _, _, mod_name = fullname.rpartition('.')
            if os.path.isdir(os.path.join(self.compiled_dir, mod_name)):
                # Is a package inside _krait_compiled
                filename = os.path.join(self.compiled_dir, mod_name, "__init__")
                self.ensure_compiled_init(fullname, filename + ".py")

                return CompiledImportHook.Loader(self, fullname, self.find_module_from_package(filename))
            else:
                # Is a module that maybe needs to be compiled
                filename = self.get_compile(fullname)
                return CompiledImportHook.Loader(self, fullname, self.find_module_from_filename(filename))
        else:
            return None

    @staticmethod
    def find_module_from_package(init_filename):
        # init_filename MUST be 1) absolute 2) in form {dir}/__init__
        return [None, os.path.dirname(init_filename), ('', '', imp.PKG_DIRECTORY)]

    @staticmethod
    def find_module_from_filename(mod_filename):
        # mod_filename MUST be absolute! And NOT contain .py
        mod_path, mod_name = os.path.split(mod_filename)
        return imp.find_module(mod_name, [mod_path])

    def ensure_compiled_init(self, fullname, filename):
        # If the file exists, and the version checks
        # (either from an earlier check, or check now)
        if os.path.exists(filename) and \
                (self.init_version == self.compiler_version or
                 self.get_compiler_version(filename) == self.compiler_version):
            # Save the checked version
            self.init_version = self.compiler_version
            # Everything good
            return

        if os.path.exists(filename) and self.get_compiled_tag(filename).get("custom"):
            # Is custom.
            return


        # Create the file.
        # First check if the directory exists
        dir_name = os.path.dirname(filename)
        if not os.path.exists(dir_name):
            os.makedirs(dir_name, 0o775)

        with open(filename, "w") as f:
            f.write(self.tag_marker + json.dumps({
                "c_version": self.compiler_version
            }))
            f.write('\n')

            # Add more things to write here.

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
        return os.path.splitext(krait_internal._compiled_get_compiled_file(mod_filename))[0]

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
            finally:
                if self.file is not None:
                    self.file.close()
                    self.file = None
            mod.__loader__ = self

        def check_tag_or_reload(self):
            # noinspection PyProtectedMember
            krait_internal._compiled_check_tag_or_reload(
                self.fullname.rpartition('.')[2],
                self.hooker_object.get_compiled_etag(self.pathname))

            # This raises an exception if the module was reloaded, or continues otherwise


def register():
    sys.meta_path.append(CompiledImportHook())
