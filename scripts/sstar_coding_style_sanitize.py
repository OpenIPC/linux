#!/usr/bin/python3
#
# SigmaStar trade secret.
# Copyright (c) [2019~2020] SigmaStar Technology.
# All rights reserved.
#
# Unless otherwise stipulated in writing, any and all information contained
# herein regardless in any format shall remain the sole proprietary of
# SigmaStar and be kept in strict confidence
# (SigmaStar Confidential Information) by the recipient.
# Any unauthorized act including without limitation unauthorized disclosure,
# copying, use, reproduction, sale, distribution, modification, disassembling,
# reverse engineering and compiling of the contents of SigmaStar Confidential
# Information is unlawful and strictly prohibited. SigmaStar hereby reserves the
# rights to any and all damages, losses, costs and expenses resulting therefrom.
#

import sys, os, re
from argparse import ArgumentParser

parser = ArgumentParser()
parser.add_argument("-f", help="cmd file", dest="cmd_file", type=str)
parser.add_argument("-l", help="sanitizing level", dest="level", default=1, type=int)

# Using hardcoded paths here due to the clang tools
# aren't in the standard place on our build servers.
CLANG_FORMAT="/tools/bin/clang-13/clang-format"
HDR_WHITELIST = ["drivers/sstar/"]
CLANG_TIDY="/tools/bin/clang-13/clang-tidy"

def whitelist_check(filename):
    for prefix in HDR_WHITELIST:
        if filename.startswith(prefix):
            return True
    return False

def clang_format(filename):
    if not os.path.isfile(filename):
        return

    command = CLANG_FORMAT + " -i " + filename;
    os.system(command);

def clang_tidy(options, filename):
    if not os.path.isfile(filename):
        return

    command = CLANG_TIDY + " --format-style=file "
    command += filename + " -- " + options;
    os.system(command);

isystem_pattern = re.compile('\s-isystem\s[^\s]*\s')
i_include_pattern = re.compile('\s-I[^\s]*\s')
D_pattern = re.compile('\s-D[^\s]*\s')

def tidy_options_generator(compile_options):
    options = ""
    m = isystem_pattern.search(compile_options)
    if m:
        options += m.group(0).strip()
        options += " "
    m = i_include_pattern.search(compile_options)

    it = i_include_pattern.finditer(compile_options)
    for m in it:
        options += m.group(0).strip()
        options += " "

    it = D_pattern.finditer(compile_options)
    for m in it:
        options += m.group(0).strip()
        options += " "

    # print(options)
    return options

def main():
    args = parser.parse_args()
    cmd_file = args.cmd_file
    level = args.level
    first_line = True
    filename = ""
    header = ""
    src_pattern = re.compile('\s[^\s]*\.c$')
    hdr_pattern = re.compile('[^\s]*.h')
    module_level_pattern = re.compile('\s-DSSTAR_CODING_STYLE_SANITIZE_MODULE=[0-9]\s');
    deps = False
    tidy_options = ""

    if (args.level <= 0) or (args.level > 2):
        return

    if (not os.path.isfile(CLANG_FORMAT)) and (not os.path.isfile(CLANG_TIDY)):
        return

    if not cmd_file:
        return

    fd = open(cmd_file, mode='r')
    while True:
        line = fd.readline()
        module_level = 0

        if not line:
            break;
        line = line.strip()

        if first_line:
            first_line = False

            if (not line.startswith('cmd_')) or (not line.endswith('.c')):
                break
            elif "-DSSTAR_CODING_STYLE_SANITIZE_MODULE" not in line:
                break
            else:
                m = module_level_pattern.search(line)
                if m:
                    module_level = int(m.group(0).strip().split("=")[1])

            m = src_pattern.search(line)
            if m:
                filename = m.group(0).strip()

            if not filename:
                break

            # print(filename)

            if ((level >= 1) or (module_level >= 1)):
                clang_format(filename)

            if ((level >= 2) or (module_level >= 2)):
                tidy_options = tidy_options_generator(line)
                clang_tidy(tidy_options, filename)

            # current is disable deps file sanitize
            continue

        if not deps:
            if line.startswith('deps_'):
                deps = True
                continue
            else:
                continue

        if line.startswith('$(wildcard'):
            continue

        m = hdr_pattern.search(line)
        if not m:
            continue

        header = m.group(0).strip()
        # print(header)
        if whitelist_check(header):
            if ((level >= 1) or (module_level >= 1)):
                clang_format(header)
            if ((level >= 2) or (module_level >= 2)):
                clang_tidy(tidy_options, header)
    fd.close()

if __name__ == '__main__':
    main()
