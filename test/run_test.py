import unittest
import build_tests
import docs_test
import demoserver_tests

def run_tests():
    loader = unittest.TestLoader()

    tests = [
        loader.loadTestsFromModule(build_tests),
        loader.loadTestsFromModule(docs_test),
        loader.loadTestsFromModule(demoserver_tests)
    ]

    unittest.TextTestRunner(verbosity=2).run(unittest.TestSuite(tests))
