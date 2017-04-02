class CtrlBase(object):
    def get_view(self):
        raise NotImplementedError("CtrlBase.get_view()")


class SimpleCtrl(CtrlBase):
    def __init__(self, view, members):
        super(SimpleCtrl, self).__init__()
        self.view = view
        for name, value in members.iteritems():
            self.__setattr__(name, value)

    def get_view(self):
        return self.view


ctrl_stack = []
curr_ctrl = None


def push_ctrl(new_ctrl):
    global curr_ctrl
    if curr_ctrl is not None:
        ctrl_stack.append(curr_ctrl)

    curr_ctrl = new_ctrl
    return new_ctrl


def pop_ctrl():
    global curr_ctrl
    if len(ctrl_stack) == 0:
        curr_ctrl = None
        return None
    else:
        curr_ctrl = ctrl_stack.pop()
        return curr_ctrl


init_ctrl = None


def set_init_ctrl(ctrl):
    global init_ctrl
    init_ctrl = ctrl
