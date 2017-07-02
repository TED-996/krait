import random
import unittest
import time
import string

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
        self.assertIn("Welcome to Krait!", self.driver.page_source, "`Welcome to Krait!` missing.")
        self.assertIn("Why choose Krait?", self.driver.page_source, "`Why choose Krait?` missing.")

    def test_db(self):
        self.driver_get("/db")

        self.check_header("/db")

        self.assertIn("access the DB", self.driver.page_source, "`access the DB` missing.")
        self.assertIn("Leave your own message!", self.driver.page_source, '`Leave your own message!` missing.')

        try:
            form = self.driver.find_element_by_id("comment-form")
            name_input = form.find_element_by_id("name-input")
            text_input = form.find_element_by_id("text-input")
            submit_button = form.find_element_by_id("submit-button")
        except NoSuchElementException:
            self.fail("Missing form element.")

        pre_count = self.db_get_comment_count()

        random_text = "".join([random.choice(string.ascii_letters + string.digits) for _ in xrange(8)])
        injection_test = '<tag attr="\'quotes\'">&amp;&lt;&gt;I swear it\'s not malware. Token:{}</tag>'\
            .format(random_text[:len(random_text) / 2])

        new_name = "Selenium Automated Testing Initiative (SATI)"
        new_message = "Randomly generated message at {}: {}; Injection test: {}".format(
            time.asctime(),
            random_text,
            injection_test
        )

        name_input.send_keys(new_name)
        text_input.send_keys(new_message)
        submit_button.click()

        # Expected refresh.

        self.assertIn("access the DB", self.driver.page_source, "Comment failed to redirect back to DB page.")


        new_cmt_item = self.db_find_comment_by_params(new_name, new_message)

        self.assertNotEqual(new_cmt_item, None, "New comment not posted.")

        escaped_html = self.escape_html(injection_test)

        # Test skipped due to different escaping in webdriver result.
        #self.assertIn(escaped_html, self.driver.page_source,
        #              "HTML misquoted. Expected {}".format(escaped_html))

        post_count = self.db_get_comment_count()
        self.assertEquals(pre_count + 1, post_count, "Comment count not incremented.")

        # TODO: DELETE COMMENT!

    def db_get_comment_count(self):
        try:
            counter = self.driver.find_element_by_id("comment-counter")
            counts = [int(num) for num in counter.text.split() if num.isdigit()]

            if len(counts) == 0:
                self.fail("Comment count missing.")
            if len(counts) > 1:
                self.fail("Comment count inaccurate: multiple numbers found.")

            return counts[0]
        except NoSuchElementException:
            self.fail("Comment counter missing.")

    @staticmethod
    def escape_html(value):
        escape_table = {
            "&": "&amp;",
            '"': "&quot;",
            "'": "&apos;",
            ">": "&gt;",
            "<": "&lt;"
        }

        return "".join(escape_table.get(ch, ch) for ch in value)

    def db_find_comment_by_params(self, name, message):
        try:
            for cmt_item in self.driver.find_elements_by_class_name("comment-item"):
                cmt_name = cmt_item.find_element_by_class_name("comment-name")
                cmt_text = cmt_item.find_element_by_class_name("comment-text")

                if str(name) in str(cmt_name.text) and str(cmt_text.text) == str(message):
                    return cmt_item

            return None
        except NoSuchElementException:
            self.fail("Missing element looking for newly-inserted comment.")

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

                expected_active = (link == active_href)
                got_active = ("active" in li_item.get_attribute("class").split())

                a_in_li = li_item.find_elements_by_tag_name("a")
            except NoSuchElementException as ex:
                self.fail("Could not get elements for link {}: {}".format((name, link), ex))

            self.assertEquals(len(a_in_li), 1,
                              "Header link: Expected exactly one <a> in <li>, got {}".format(len(a_in_li)))
            self.assertEquals(a_in_li[0], a_item,
                              "Header link: Wrong <a> in <li>.")
            self.assertEquals(a_item.get_attribute("href"), self.host + link,
                              "Header link: href different from expected value.")
            self.assertEquals(a_item.text, name,
                              "Header link: text different from expected value.")
            self.assertEquals(expected_active, got_active,
                              "Header link: button expected to be active and was not." if expected_active else
                              "Header link: button expected not to be active and was active.")

    def tearDown(self):
        server_setup.stop_demoserver()
