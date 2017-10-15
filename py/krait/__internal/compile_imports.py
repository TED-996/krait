import sys
import os
import imp
import krait
from krait import __internal


class CompiledFinder(object):
    def __init__(self):
        self.compiled_dir = os.path.normpath(krait.get_full_path(".compiled"))

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
        if fullname == "_krait_compiled_dir":
            self.ensure_compiled_init()
            # Use a simple loader.
            CompiledFinder.Loader(fullname, path, os.path.join(self.compiled_dir, "__init__.py"))
        elif path is not None\
                and len(path) == 1\
                and os.path.normpath(path[0]) == self.compiled_dir:
            return CompiledFinder.Loader(fullname, path, self.get_compile(fullname))
        else:
            return None

    class Loader(object):
        def __init__(self, fullname, path, filename):
            self.fullname = fullname
            self.path = path
            self.filename = filename

        def load_module(self, fullname):
            if fullname != self.fullname:
                raise ValueError("CompiledLoader reused for other modules.")

            if fullname in sys.modules:
                return sys.modules[fullname]

            code = self.get_code()

            mod = sys.modules.setdefault(fullname, imp.new_module(fullname))
            mod.__file__ = self.filename
            mod.__loader__ = self

            is_package = (os.path.basename(self.filename).splitext()[0] == "__init__")
            if is_package:
                mod.__path__ = [os.path.dirname(self.filename)]
                mod.__package__ = fullname
            else:
                mod.__package__ = fullname.rpartition('.')[0]

            exec(code, mod.__dict__);
            return mod

        def get_code(self):
            # TODO: load code from pyc! Both here and in loader.
            with open(self.filename, "rb") as f:
                return f.read()

