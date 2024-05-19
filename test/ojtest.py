#!/usr/bin/python3
# -*- coding: utf-8 -*-

import os
import sys
import time
import json
import subprocess
import re
import shutil
import random
import string
import hashlib
import threading

# first, get the directory of the script
script_dir = os.path.dirname(os.path.realpath(__file__))
print("script_dir: " + script_dir)
path_to_exec_file = os.path.join(script_dir, "../build/code")
print("path_to_exec_file: " + path_to_exec_file)