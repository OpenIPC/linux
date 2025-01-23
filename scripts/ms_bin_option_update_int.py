#
# Copyright (c) [2019~2020] SigmaStar Technology.
#
#
# This software is licensed under the terms of the GNU General Public
# License version 2, as published by the Free Software Foundation, and
# may be copied, distributed, and modified under those terms.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License version 2 for more details.
#

#!/usr/bin/python

import re, fnmatch, os, sys, mmap, struct

if __name__ == '__main__':

    name=sys.argv[2]
    if sys.argv[3].upper().startswith( '0X' ):
        value=long(sys.argv[3],16)
    else:
        value=long(sys.argv[3])

    fmap=mmap.mmap(os.open(sys.argv[1],os.O_RDWR),0)

    offset=fmap.find(name)
#    print ('%s:%d\n' % (name,offset))
    if offset < 0:
        print ('error finding ms_bin_option:%s in %s\n' % (name,sys.argv[1]))
    else:
        fmap.seek(offset + 8, os.SEEK_SET)
    fmap.write(struct.pack('<I', value))

    fmap.close()
