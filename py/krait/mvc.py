class CtrlBase(object):
    """
    The base of a MVC controller.
    The only necessary method is get_view, this returns the filename (relative to the site_root)
    to build the response from.
    """
    def get_view(self):
        """
        :return: The filename (relative to the site_root) of the view
        """
        raise NotImplementedError("CtrlBase.get_view()")


class SimpleCtrl(CtrlBase):
    """
    A simple controller wrapper. Best not to use, controllers should set their views themselves.
    """
    def __init__(self, view, members):
        """
        :param view: the view to be requested by the controller
        :param members: the attributes of the controller
        """
        super(SimpleCtrl, self).__init__()
        self.view = view
        for name, value in members.iteritems():
            self.__setattr__(name, value)

    def get_view(self):
        return self.view


"""
The stack of controllers, used with nested controllers, semi-deprecated.
Use with push_ctrl and pop_ctrl
"""
ctrl_stack = []
"""
The current controller, used by push_ctrl and pop_ctrl
"""
curr_ctrl = None


def push_ctrl(new_ctrl):
    """
    Save the current controller and set a new one
    param new_ctrl: the new controller
    :return: the new controller, for chaining purposes
    """
    global curr_ctrl
    if curr_ctrl is not None:
        ctrl_stack.append(curr_ctrl)

    curr_ctrl = new_ctrl
    return new_ctrl


def pop_ctrl():
    """
    Discards the current controller and sets the one before it
    :return: The newly-set controller
    """
    global curr_ctrl
    if len(ctrl_stack) == 0:
        curr_ctrl = None
        return None
    else:
        curr_ctrl = ctrl_stack.pop()
        return curr_ctrl


"""
The controller to set be used as an initial controller. Do not use directly, se set_init_ctrl().
Set from routable pages that want to invoke a controller.
"""
init_ctrl = None


def set_init_ctrl(ctrl):
    """
    Invoke a controller after the script has finish executing.
    :param ctrl: The controller to be used.
    """
    global init_ctrl
    init_ctrl = ctrl
