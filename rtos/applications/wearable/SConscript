Import('RTT_ROOT')
Import('rtconfig')
from building import *

cwd = GetCurrentDir()
src = Glob('*.c') \
        + Glob('resource/*.c') \
        + Glob('func/*.c') \
        + Glob('func/setting/*.c')

CPPPATH = [cwd] + [cwd + '/func']  + [cwd + '/func/setting']

group = DefineGroup('Applications', src, depend = ['RT_USING_APP_WEARABLE'], CPPPATH = CPPPATH)

Return('group')
