import unittest
import server_setup

class BuildTests(unittest.TestCase):
    def test_build(self):
        server_setup.build()
