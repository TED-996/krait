import unittest
import os
from selenium import webdriver
from selenium.common.exceptions import WebDriverException
import fabric.api as fab

import server_setup
import test_utils


class DocsTest(unittest.TestCase):
    def setUp(self):
        server_setup.setup(skip_build=True)

        self.docs_dir = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "docs"))
        self.make_docs()

        self.driver = webdriver.Chrome()

    def make_docs(self):
        # TODO: allow making docs remotely.

        if os.name == "nt":
            cmd = "{} html".format(os.path.join(self.docs_dir, "make.bat"))
        else:
            cmd = "make html"

        with fab.lcd(self.docs_dir), fab.settings(warn_only=True):
            make_result = fab.local(cmd)

        if make_result.failed:
            self.fail("Docs failed to build. Output: {}".format(make_result))

    def do_test_docs_at_url(self, url_base):
        files = {
            "index.html": [
                "Krait Documentation Home",
                "krait package",
                "Module contents",
                "Submodules",
                "What is Krait?"],
            "krait.html": [
                "krait package",
                "Module contents",
                "Reference",
                "Submodules",
                "krait.config module",
                "krait.mvc module",
                "krait.cookie module",
                "krait.websockets module",
                'href="krait.config.html"',
                'href="krait.mvc.html"',
                'href="krait.cookie.html"',
                'href="krait.websockets.html"'
            ],
            "krait.config.html": [
                "krait.config module",
                "Reference",
                "Route",
                "routes",
                "cache_no_store",
                "cache_private",
                "cache_public",
                "cache_long_term",
                "cache_max_age_default",
                "cache_max_age_long_term"
            ],
            "krait.cookie.html": [
                "krait.cookie module",
                "Usage",
                "Get request cookies:",
                "Set cookies:",
                "Get response cookies:",
                "Reference",
                "Cookie",
                "CookieAttribute",
                "CookieExpiresAttribute",
                "CookieMaxAgeAttribute",
                "CookiePathAttribute",
                "CookieDomainAttribute",
                "CookieHttpOnlyAttribute",
                "CookieSecureAttribute",
                "get_cookies",
                "get_cookie",
                "get_response_cookies",
                "set_cookie"
            ],
            "krait.mvc.html": [
                "krait.mvc module",
                "Usage",
                "Create a template (a view):",
                "Create a controller:",
                "Reference properties on the controller in templates:",
                "Set a controller to handle a specific URL:",
                "Reference",
                "CtrlBase",
                "SimpleCtrl",
                "get_view",
                "init_ctrl",
                "set_init_ctrl",
                "ctrl_stack",
                "curr_ctrl",
                "push_ctrl",
                "pop_ctrl",
            ],
            "krait.websockets.html": [
                "krait.websockets module",
                "Usage",
                "WebsocketsRequest",
                "WebsocketsResponse",
                "request",
                "response",
                "WebsocketsCtrlBase"
            ]
        }

        for html_file, to_search in files.iteritems():
            try:
                self.driver.get(url_base + html_file)
            except WebDriverException as ex:
                self.fail("Could not get local docs.\nError: {}".format(ex))

            for search_item in to_search:
                self.assertIn(search_item, self.driver.page_source,
                              "`{}` not found in docs page.".format(search_item))

            print "Tested docs page {} successfully.".format(url_base + html_file)

        # TODO: make better tests

    def test_krait_rtd_up(self):
        if not test_utils.check_internet_access():
            self.skipTest("ReadTheDocs test skipped: No internet access.")

        self.do_test_docs_at_url(R"https://krait.readthedocs.io/en/latest/")

    def test_new_docs_ok(self):
        url_base = R"file://{}/docs/build/html/"\
            .format(os.path.dirname(os.path.dirname(__file__)))\
            .replace('\\', '/')

        self.do_test_docs_at_url(url_base)

    def tearDown(self):
        self.driver.close()
