import sys
import os

krait_py_dir = os.path.join(root_dir, "py")
del root_dir

sys.path.append(krait_py_dir)

class IteratorWrapper(object):
    """
    Wraps an iterator in a way useful to the classic for(init; condition; increment) paradigm
    """

    def __init__(self, collection):
        self.iterator = iter(collection)
        self.over = False
        self.value = self.next()

    def next(self):
        if self.over:
            return None
        try:
            self.value = self.iterator.next()
            return self.value
        except StopIteration:
            self.over = True
            return None
