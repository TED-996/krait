import unittest
from selenium import webdriver
from selenium.common.exceptions import WebDriverException

import server_setup


class DemoserverTests(unittest.TestCase):
    def __init__(self, methodName="runTest"):
        super(DemoserverTests, self).__init__(methodName)

    def setUp(self):
        server_setup.setup()
        self.host = server_setup.start_demoserver()

        self.driver = webdriver.Chrome()

    def driver_get(self, local_url):
        # local_url should NOT be empty. Root page is at "/".
        url = self.host + local_url

        try:
            self.driver.get(url)
        except WebDriverException as ex:
            self.fail("Could not get demoserver page {}.\nError: {}".format(local_url, ex))

    def test_index(self):
        self.driver_get("/")

        self.assertIn("Welcome to Krait!", self.driver.page_source)
        self.assertIn("Why choose Krait?", self.driver.page_source)

    def tearDown(self):
        server_setup.stop_demoserver()
