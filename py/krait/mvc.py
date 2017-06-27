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
4. Set a controller to handle a specific URL
    Create a Python script with the appropriate location and name to be reached by the URL.
    Import ``krait.mvc``, then call :obj:`set_init_ctrl`, passing as an argument an object
    of your controller type. Krait will then call its overridden :obj:`CtrlBase.get_view` method
    and render the template.

Reference
=========
"""


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
