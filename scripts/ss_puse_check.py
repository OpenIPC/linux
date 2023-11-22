from __future__ import print_function
from sys import argv
import sys
import os
from string import digits
def main():
    puse_name = argv[1]
    if os.path.exists(puse_name):
        mdrv_puse_file = open(puse_name, 'r')
    else:
        sys.exit(0)

    mdrv_puse_read = mdrv_puse_file.readline()
    while mdrv_puse_read:
        if mdrv_puse_read.find('// IP_Index') != -1:
            ip_number = 0
            mdrv_puse_read = mdrv_puse_file.readline()
            while len(mdrv_puse_read) != 1:
                ip_number += 1
                mdrv_puse_read = mdrv_puse_file.readline()
        if mdrv_puse_read.find('// Pad_Index') != -1:
            pad_number = 0
            mdrv_puse_read = mdrv_puse_file.readline()
            while len(mdrv_puse_read) != 1:
                pad_number += 1
                mdrv_puse_read = mdrv_puse_file.readline()
        mdrv_puse_read = mdrv_puse_file.readline()

    ip_list = [0 for i in range(ip_number)]
    puse_number_list = [0 for i in range(ip_number)]
    ip_name = [0 for i in range(ip_number)]
    ip_value = [0 for i in range(ip_number)]
    puse_number = [0 for i in range(ip_number)]
    pad_index_list = [0 for i in range(pad_number)]
    puse_number_line = [0 for i in range(pad_number)]

    mdrv_puse_file = open(puse_name, 'r')
    line = 0
    mdrv_puse_read = mdrv_puse_file.readline()
    line += 1
    while mdrv_puse_read:
        if mdrv_puse_read.find('// IP_Index') != -1:
            mdrv_puse_read = mdrv_puse_file.readline()
            line += 1
            ip_index = 0
            while len(mdrv_puse_read) != 1:
                ip_list[ip_index] = mdrv_puse_read.split(' ')
                ip_name[ip_index] = ip_list[ip_index][1].replace('PUSE_', '')
                ip_value[ip_index] = ip_list[ip_index][len(ip_list[ip_index])-1].replace('\n', '')
                ip_index += 1
                mdrv_puse_read = mdrv_puse_file.readline()
                line += 1

        if mdrv_puse_read.find('// Puse_Num_List') != -1:
            mdrv_puse_read = mdrv_puse_file.readline()
            line += 1
            ip_index = 0
            while len(mdrv_puse_read) != 1:
                puse_number_list[ip_index] = mdrv_puse_read.split(' ')
                puse_number[ip_index] = int(puse_number_list[ip_index][len(puse_number_list[ip_index]) - 1].replace('\n', ''), 16)
                puse_number_line[ip_index] = line
                ip_index += 1
                mdrv_puse_read = mdrv_puse_file.readline()
                line += 1
        if mdrv_puse_read.find('// Pad_Index') != -1:
            mdrv_puse_read = mdrv_puse_file.readline()
            line += 1
            pad_index = 0
            while len(mdrv_puse_read) != 1:
                if mdrv_puse_read.find('#define') != -1:
                    pad_index_list[pad_index] = mdrv_puse_read.split(' ')
                    pad_index += 1
                mdrv_puse_read = mdrv_puse_file.readline()
                line += 1

            pad_index_len = pad_index
            puse_ip_index = 0
        if mdrv_puse_read.find('MDRV_PUSE_NA') != -1:
            mdrv_puse_read = mdrv_puse_file.readline()
            line += 1
            mdrv_puse_read = mdrv_puse_file.readline()
            line += 1
            while mdrv_puse_read.find('#endif') == -1:
                ip = mdrv_puse_read[mdrv_puse_read.find(' ') + 1:mdrv_puse_read.find('\n')]
                for i in range(len(ip_name)):
                    if ip == ip_name[i]:
                        ip_count = int(ip_value[i], 16)
                        break
                mdrv_puse_read = mdrv_puse_file.readline()
                line += 1
                number = 0
                while len(mdrv_puse_read) != 0:
                    number += 1
                    if mdrv_puse_read.find('Channel') != -1:
                        channel = int(mdrv_puse_read.split(' ')[2].replace('\n', ''))
                        mdrv_puse_read = mdrv_puse_file.readline()
                        line += 1
                    if len(mdrv_puse_read) == 1:
                        break
                    for k in range(pad_index_len):
                        if (pad_index_list[k][1].replace(ip + '_', '') in mdrv_puse_read) or (pad_index_list[k][1].replace(ip + '_', '') == 'NA'):
                            index = int(pad_index_list[k][len(pad_index_list[k]) - 1].replace('\n', ''), 16)
                            puse_read = mdrv_puse_read.split(' ')
                            macro = int(puse_read[len(puse_read)-1].replace('\n', ''), 16)
                            if macro == index + channel * 256 + ip_count:
                                mdrv_puse_read = mdrv_puse_file.readline()
                                line += 1
                                break
                        if k == pad_index_len-1:
                            print('[ Line:' + str(line) + ' ]' + 'Please check this PUSE define: ' + mdrv_puse_read[mdrv_puse_read.find(' ') + 1:mdrv_puse_read.find('0x')])
                            sys.exit(-1)
                mdrv_puse_read = mdrv_puse_file.readline()
                line += 1
                if number-1 == puse_number[puse_ip_index]:
                    puse_ip_index += 1
                else:
                    print('[ Line:' + str(puse_number_line[puse_ip_index]) + ' ]' + 'Please check this macro definition is equal to the PUSE number of IP: ', ip)
                    sys.exit(-1)
        mdrv_puse_read = mdrv_puse_file.readline()
        line += 1
    print('PUSE check PASS!')
if __name__ == "__main__":
    main()