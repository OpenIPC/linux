#!/usr/bin/python

import re, fnmatch, os, sys, mmap, struct

if __name__ == '__main__':
    
    name='#UNFDT_#'
 
    dtb_file=open(sys.argv[2])
    dtb_file.seek(0,os.SEEK_END)
    size=dtb_file.tell()
    dtb_file.seek(0,os.SEEK_SET)
    dtb=dtb_file.read()
    dtb_file.close()
    
    if sys.getsizeof(dtb) > (128*1024):
        print ('UNFDT size 0x%08X too big!!' % dtb.size())
        sys.exit() 
        
    fmap=mmap.mmap(os.open(sys.argv[1],os.O_RDWR),0)

    offset=fmap.find(name)    
    if offset >=0:
        print ('Refill unflatten device tree...')
        print ('offset:0x%08X' % offset)
        print ('  size:0x%08X' % size )
        fmap.seek(offset + 8, os.SEEK_SET)
        #fmap.write(struct.pack('<I', size))
        #fmap.seek(offset + 16, os.SEEK_SET)
        fmap.write(dtb)

    fmap.close()
    
    
