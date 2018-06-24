try:
    import _krait_emit
    import _krait_compile

    _emit = _krait_emit.emit
    _emit_raw = _krait_emit.emit_raw
    _compiled_convert_filename = _krait_compile.convert_filename
    _compiled_get_compiled_file = _krait_compile.get_compiled_file
    _compiled_check_tag = _krait_compile.check_tag
    _compiled_check_tag_or_reload = _krait_compile.check_tag_or_reload
    _compiled_run = _krait_compile.run
except ImportError:
    def _emit(s):
        raise RuntimeError("Not running under Krait, cannot emit.")

    def _emit_raw(s):
        raise RuntimeError("Not running under Krait, cannot emit.")

    def _compiled_convert_filename(filename):
        raise RuntimeError("Not running under Krait, cannot compile.")

    def _compiled_get_compiled_file(filename):
        raise RuntimeError("Not running under Krait, cannot compile.")

    def _compiled_check_tag(filename, tag):
        raise RuntimeError("Not running under Krait, cannot compile.")

    def _compiled_check_tag_or_reload(filename, tag):
        raise RuntimeError("Not running under Krait, cannot compile.")

    def _compiled_run(filename, tag):
        raise RuntimeError("Not running under Krait, cannot run compiled.")
