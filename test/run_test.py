import unittest
import build_tests
import docs_test
import demoserver_tests


def run_tests(sources=None):
    """
    Run the tests specified by sources.

    Args:
        sources (list of str, optional): A subset of ["build", "docs", "demoserver"], or None for all.

    """
    loader = unittest.TestLoader()

    modules_by_name = {
        "build": build_tests,
        "docs": docs_test,
        "demoserver": demoserver_tests
    }
    sources = sources or modules_by_name.keys()

    tests = [loader.loadTestsFromModule(modules_by_name[name]) for name in sources]
    unittest.TextTestRunner(verbosity=2).run(unittest.TestSuite(tests))
