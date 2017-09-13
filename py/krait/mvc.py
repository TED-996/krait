"""
This module interfaces Krait's MVC support with your Python backend.
MVC support is implemented by having a Python script act as a route target, that creates and specifies
a controller object that holds the properties used in the finished page. This page's code is provided
by a template file, that the controller requests. Templates are files with Pyml syntax embedded inside
that accesses properties on the controller. The rest of the code is usually HTML, although it can be JSON,
JavaScript, or anything else.

Usage
=====

See the tutorial (TODO) for a more detailed explanation.

1. Create a template (a view):
    Create a file with the ``.html`` or ``.pyml`` extension in a hidden directory in your site root
    (typically ``.view``) with your template code.
2. Create a controller:
    Create a subclass of :class:`CtrlBase` and override its :obj:`CtrlBase.get_view` method
    to return the **relative** path of a template. To add properties to the controller, simply set them
    in its ``__init__`` method.
3. Reference properties on the controller in templates:
    In template files the ``ctrl`` variable refers to the active controller.
    Use Pyml syntax to access its members and use them on the page.
4. Set a controller to handle a specific URL:
    There are two alternatives:
    1. Use the routing engine, as seen in :obj:`krait.config` (**recommended**).
    2. Use a Python script as a normal target:
    Create a Python script with the appropriate location and name to be reached by the URL.
    Import ``krait.mvc``, then call :obj:`set_init_ctrl`, passing as an argument an object
    of your controller type. Krait will then call its overridden :obj:`CtrlBase.get_view` method
    and render the template.

Reference
=========
"""
import config
import importlib
import os
import re


class CtrlBase(object):
    """
    The base of a MVC controller.
    Abstract, implementations have to override the :obj:`get_view` method.
    """

    def get_view(self):
        """
        Get the view that renders the response.

        Returns:
            str: a **relative** path
        """
        raise NotImplementedError("CtrlBase.get_view()")


class SimpleCtrl(CtrlBase):
    """
    A simple controller wrapper. Best not to use, controllers should set their views themselves.

    Args:
        view (str): The view that this controller renders with.
        members (dict): The attributes of the controller. These will be set dynamically as instance attributes.

    Attributes:
        view (str): The view that this controller renders with.

    Apart from the view, other attributes exist, specified in the :obj:`members` dictionary.
    """
    def __init__(self, view, members):
        super(SimpleCtrl, self).__init__()
        self.view = view
        for name, value in members.iteritems():
            self.__setattr__(name, value)

    def get_view(self):
        """
        Get the selected view.

        Returns:
            str: :obj:`view`
        """
        return self.view


def route_ctrl_decorator(verb="GET", url=None, regex=None):
    """
    Get a decorator that adds an MVC controller as a route target

    Args:
        verb (str, optional): The routing verb (most HTTP verbs, plus ``ANY``. ``WEBSOCKET`` not supported.)
            See :obj:`krait.config.Route.route_verbs` for options; GET by default
        url (str, optional): The URL to match, or None to skip
        regex (str, optional): The regex to match, or None to skip

    Returns:
        A decorator that adds the MVC controller as a route target.
    """
    def decorator(class_type):
        if config.routes is None:
            config.routes = []

        config.routes.append(config.Route(verb=verb, url=url, regex=regex, ctrl_class=class_type))

        return class_type

    return decorator


def import_ctrls_from(package_name):
    """
    Scans the package recursively and imports all modules inside it.
    This 1) speeds up the following requests, as all modules are already imported
    and 2) allows the controllers to add themselves to the routing system
    (using the :obj:`krait.mvc.route_ctrl_decorator` decorator)

    Args:
        package_name (str): the name of the package.
            This should be already accessible as ``import <package_name>``.
    """

    _recurse_import_package(package_name, True)


def _recurse_import_package(name, is_first):
    # No problem if we keep recompiling it, there may be some caching and it's mostly one-time anyway.
    init_regex = re.compile("^__init__\.py[cdo]?$")
    py_regex = re.compile("^(.*)\.py[cdo]?$")

    try:
        package_or_module = importlib.import_module(name)
    except ImportError:
        # Not a module / package.
        if is_first:
            # We expect at least the first import to succeed.
            raise
        else:
            return

    try:
        submodules = package_or_module.__all__
    except AttributeError:
        # Doesn't contain '__all__' (not a package, or not custom __all__)
        dir_name, file_name = os.path.split(package_or_module.__file__)
        if not init_regex.match(file_name):
            return

        submodules = set()
        for entry in os.listdir(dir_name):
            entry_path = os.path.join(dir_name, entry)
            # Add *files* that are not __init__.py:
            py_match = py_regex.match(entry)
            if os.path.isfile(entry_path) and py_match and py_match.group(1) != "__init__":
                submodules.add(py_match.group(1))

    for entry in submodules:
        _recurse_import_package(name + "." + entry, False)



init_ctrl = None
"""
:class:`CtrlBase`:
The controller to be used as a master controller. Do not use directly, use :obj:`set_init_ctrl`.
Set from route targets that want to invoke a controller (and render a response using MVC).
"""


def set_init_ctrl(ctrl):
    """
    Invoke a controller after the route target has finished executing.

    Args:
        ctrl (:class:`CtrlBase`): The controller object to be used.
    """
    global init_ctrl
    init_ctrl = ctrl


ctrl_stack = []
"""
list of :class:`CtrlBase`: The stack of controllers, used with nested controllers. Semi-deprecated.
Do not use directly, use :obj:`push_ctrl` and :obj:`pop_ctrl`.
"""

curr_ctrl = None
""":class:`CtrlBase`: The current controller, used in controller stacking. Semi-deprecated.
Do not use directly, use :obj:`push_ctrl` and :obj:`pop_ctrl`.
"""


def push_ctrl(new_ctrl):
    """
    Save the current controller and set a new one.

    Args:
        new_ctrl (:class:`CtrlBase`): The new controller.

    Returns:
        :class:`CtrlBase`: The new controller.
    """

    global curr_ctrl
    if curr_ctrl is not None:
        ctrl_stack.append(curr_ctrl)

    curr_ctrl = new_ctrl
    return new_ctrl


def pop_ctrl():
    """
    Discard the current controller and set the one before it.

    Returns:
        :class:`CtrlBase`: The old, now current, controller.
    """

    global curr_ctrl
    if len(ctrl_stack) == 0:
        curr_ctrl = None
        return None
    else:
        curr_ctrl = ctrl_stack.pop()
        return curr_ctrl
