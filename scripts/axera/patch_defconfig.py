#encoding=utf-8
import sys
import os
import re

def usage():
    '''usage:
    {} in_defconfig1 in_defconfig2 ... out_kernel_defconfig
    '''.format(sys.argv[0])

def get_config_maps(srcList):
    cfg_maps = {}
    for src in srcList:
        with open(src) as f:
            lines = [line.strip() for line in f if line.strip() and not line.startswith("#")]

        for line in lines:
            k,v = line.split("=")
            if k in cfg_maps.keys():
                print("[Warning] found duplicated config [{}] in {}, ignore!".format(line, src))
            cfg_maps.update({k.strip():v.strip()})

    return cfg_maps

def patch_defconfig(config_maps, src_defconfig):

    out_defconfig = src_defconfig
    out_dir = os.path.dirname(os.path.abspath(out_defconfig))

    if not os.path.exists(src_defconfig):
        print("[error] cannot find deconfig file: {}".format(src_defconfig))
        return False

    if not os.path.exists(out_dir):
        os.makedirs(out_dir)

    patched_configs = []
    with open(src_defconfig) as f:
        src_lines = f.readlines()

    ptn=r"(\w+)\s*=\s*(\w+)"
    ptn2=r"#\s*(\w+)\s*is not set"
    new_config_lines = []
    for line in src_lines:
        if line.strip().startswith("#"):
            matched = re.search(ptn2, line)
            if matched:
                config_name = matched.groups()[0]
                if config_name in config_maps.keys():
                    line ="{}={}\n".format(config_name,config_maps[config_name])
                    patched_configs.append(config_name)
            new_config_lines.append(line)
            continue
        matched = re.search(ptn, line)
        if not matched:
            new_config_lines.append(line)
            continue
        k,_ = matched.groups()
        if k in config_maps.keys():
            patched_configs.append(k)
            new_config="{}={}\n".format(k,config_maps[k])
            new_config_lines.append(new_config)
        else:
            new_config_lines.append(line)

    for k in config_maps.keys():
        if k not in patched_configs:
            new_config="{}={}\n".format(k,config_maps[k])
            new_config_lines.append(new_config)

    with open(out_defconfig, "wt") as f:
        f.writelines(new_config_lines)

    print("patch {} done!".format(src_defconfig))

if __name__ == "__main__":
    args_count = len(sys.argv)
    if args_count < 3:
        usage()
        sys.exit(0)
    src_defconfig_list = sys.argv[1:args_count-1]
    dst_defconfig = sys.argv[-1]
    config_maps = get_config_maps(src_defconfig_list)
    patch_defconfig(config_maps, dst_defconfig)
