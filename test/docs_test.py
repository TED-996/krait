import unittest
import os
from selenium import webdriver
from selenium.common.exceptions import WebDriverException
import fabric.api as fab

import server_setup


class DocsTest(unittest.TestCase):
    def setUp(self):
        server_setup.setup()

        self.docs_dir = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "docs"))
        self.make_docs()

        self.driver = webdriver.Chrome()

    def make_docs(self):
        # TODO: allow making docs remotely.

        if os.name == "nt":
            args = [
                os.path.join(self.docs_dir, "make.bat"),
                "html"
            ]
        else:
            args = [
                "make",
                "html"
            ]

        with fab.lcd(self.docs_dir), fab.settings(warn_only=True):
            make_result = fab.local(args)

        if make_result.failed:
            self.fail("Docs failed to build. Output: {}".format(make_result))


    def test_krait_docs_up(self):
        url = R"https://krait.readthedocs.io/en/latest/krait.html"

        try:
            self.driver.get(url)
        except WebDriverException as ex:
            self.fail("Could not get remote docs.\nError: {}".format(ex))

        self.assertIn("krait", self.driver.title)
        self.assertIn("krait", self.driver.page_source)

    def test_new_docs_ok(self):
        url_base = R"file://{}/build/html/"
        files = ["index.html",
                 "krait.html",
                 "krait.config.html",
                 "krait.cookie.html",
                 "krait.mvc.html",
                 "krait.websockets.html"]

        for html_file in files:
            try:
                self.driver.get(url_base + html_file)
            except WebDriverException as ex:
                self.fail("Could not get remote docs.\nError: {}".format(ex))

            self.assertIn("krait", self.driver.title)
            self.assertIn("krait", self.driver.page_source)

            # TODO: make better tests

    def tearDown(self):
        self.driver.close()
