class Const:
    class ConstError(TypeError):
        pass

    def __setattr__(self, name, value):
        if name in self.__dict__:
            raise self.ConstError("Can't rebind const (%s)" % name)
        self.__dict__[name] = value

import sys
sys.modules[__name__] = Const()

import const
const.OFFLOADABLE = 0
const.PARTIAL_OFFLOADABLE = 1
const.UN_OFFLOADABLE = 2