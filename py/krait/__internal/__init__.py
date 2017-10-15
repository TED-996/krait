try:
    import _krait_emit

    _emit = _krait_emit.emit
    _emit_raw = _krait_emit.emit_raw
    _compiled_get_filename = _krait_emit.get_filename
    _compiled_compile = _krait_emit.get_compiled
    _compiled_check_tag = _krait_emit.compiled_check_tag
except ImportError:
    def _emit(s):
        raise RuntimeError("Not running under Krait, cannot emit.")

    def _emit_raw(s):
        raise RuntimeError("Not running under Krait, cannot emit.")

    def _compiled_get_filename(filename):
        raise RuntimeError("Not running under Krait, cannot compile.")

    def _get_compiled(filename):
        raise RuntimeError("Not running under Krait, cannot compile.")

    def _compiled_check_tag(filename, tag):
        raise RuntimeError("Not running under Krait, cannot compile.")
