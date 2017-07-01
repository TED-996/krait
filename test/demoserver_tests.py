import unittest
from selenium import webdriver
from selenium.common.exceptions import WebDriverException
from selenium.common.exceptions import NoSuchElementException

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

        self.check_header("/")
        self.assertIn("Welcome to Krait!", self.driver.page_source)
        self.assertIn("Why choose Krait?", self.driver.page_source)

    def check_header(self, active_href):
        try:
            header = self.driver.find_element_by_class_name("header")
        except NoSuchElementException:
            self.fail("Header missing.")

        try:
            project_name = header.find_element_by_id("project-name")
        except NoSuchElementException:
            self.fail("Project name missing.")

        self.assertIn("Krait".lower(), project_name.text.lower())

        header_items = [
            ("Home", "/"),
            ("DB Access", "/db"),
            ("Your Request", "/http"),
            ("Websockets", "/ws")
        ]

        for name, link in header_items:
            a_id = "header-a-{}".format(name.lower())
            li_id = "header-li-{}".format(name.lower())

            try:
                li_item = header.find_element_by_id(li_id)
                a_item = header.find_element_by_id(a_id)

                a_in_li = li_item.find_elements_by_tag_name("a")
            except NoSuchElementException as ex:
                self.fail("Could not get elements for link {}: {}".format((name, link), ex))

            self.assertEquals(len(a_in_li), 1)
            self.assertEquals(a_in_li[0], a_item)
            self.assertEquals(a_item.get_attribute("href"), self.host + link)
            self.assertEquals(a_item.text, name)

    def tearDown(self):
        server_setup.stop_demoserver()
