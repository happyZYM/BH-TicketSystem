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
data_dir = script_dir + "/data/ojdata/"
print("directory of data:", data_dir)
config_file_path = data_dir + "config.json"
print("config_file_path: ", config_file_path)

with open(config_file_path, 'r') as f:
  test_config = json.load(f)

# print(test_config)  # print the loaded config to verify

argc = len(sys.argv)
argv = sys.argv

print("argc: ", argc)
print("argv: ", argv)
if argc==1:
  print("preparing to run all testgroups...")
elif argc>=2:
  print("preparing to run testgroup: ", argv[1])

skip_check=False
ignore_first_dependency=False
enable_tested_program_logging=False
for i in range(argc):
  if argv[i]=="--skip-check":
    skip_check=True
  if argv[i]=="--ignore-first-dependency":
    ignore_first_dependency=True
  if argv[i]=="--enable-tested-program-logging":
    enable_tested_program_logging=True

if not skip_check:
  command = 'cat ticket.sum | sha256sum -c'
  print("checking the sha256sum of testpoint...")
  process = subprocess.run(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=data_dir)
  # Check the exit status of the command
  if process.returncode == 0:
    print("The test data is correct!")
  else:
    print(f"Command failed with exit code {process.returncode}")
    print(process.stderr.decode('utf-8'))  # print the error message

def FetchTestPointInfo(id):
  for it in test_config["Details"]:
    if it["ID"]==id:
      return it

passed_test={0:True}
has_unpassed_test=False
def RunTestGroup(name):
  global passed_test
  global has_unpassed_test
  print("preparing to run testgroup: ", name)
  test_point_list=[]
  for it in test_config["Groups"]:
    if it["GroupName"]==name:
      test_point_list=it["TestPoints"]
  print("test_point_list: ", test_point_list)
  playground_dir="/tmp/"+name
  # remove directory /tmp/$name
  shutil.rmtree(playground_dir, ignore_errors=True)
  # create directory /tmp/$name
  os.makedirs(playground_dir)
  for test_point_id in test_point_list:
    test_info=FetchTestPointInfo(test_point_id)
    print("test info of test point ", test_point_id, ": ", test_info)
    disk_limit=test_info["DiskLimit"]
    dependency=test_info["Dependency"]
    if passed_test.get(dependency, False)==False and not (ignore_first_dependency and test_point_id==test_point_list[0]):
      print(f'dependency {dependency} not met, skip this test point')
      passed_test[test_point_id]=False
      has_unpassed_test=True
      continue
    if disk_limit<0:
      disk_limit = -disk_limit
      # remove directory /tmp/$name
      shutil.rmtree(playground_dir, ignore_errors=True)
      # create directory /tmp/$name
      os.makedirs(playground_dir)
    time_limit=test_info["TimeLimit"]
    file_number_limit=test_info["FileNumberLimit"]
    memory_limit=test_info["MemoryLimit"]
    input_file=data_dir+str(test_point_id)+".in"
    output_file=playground_dir+"/"+str(test_point_id)+".out"
    answer_file=data_dir+str(test_point_id)+".out"
    stderr_file=playground_dir+"/"+str(test_point_id)+".err"
    diff_file=playground_dir+"/"+str(test_point_id)+".diff"
    log_file=playground_dir+"/"+str(test_point_id)+".log"
    time_limit = int(time_limit / 1000) # convert to seconds
    memory_limit = int(memory_limit / 1024) # convert to KB
    disk_limit = int(disk_limit / 512) # convert to 512B
    print("input_file {}, output_file {}, answer_file {}".format(input_file, output_file, answer_file))
    print("time limit {}, disk limit {}, file number limit {}".format(time_limit, disk_limit, file_number_limit))
    # run the path_to_exec_file with input_file and output_file with cwd=playground_dir
    if not enable_tested_program_logging:
      command = f'ulimit -t {time_limit} && ulimit -m {memory_limit} && ulimit -f {disk_limit} && ulimit -n {file_number_limit} && {path_to_exec_file} < {input_file} > {output_file} 2> {stderr_file}'
    else:
      command = f'ulimit -t {time_limit} && ulimit -m {memory_limit} && ulimit -f {disk_limit} && ulimit -n {file_number_limit} && {path_to_exec_file} -l {log_file} --level debug < {input_file} > {output_file} 2> {stderr_file}'
    print("the test command is: ", command)
    process = subprocess.run(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=playground_dir)
    # Check the exit status of the command
    if process.returncode == 0:
      print("Test point ", test_point_id, " successfully run!")
      # run diff command to check the output
      command = f'diff {output_file} {answer_file} > {diff_file}'
      process = subprocess.run(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=playground_dir)
      if process.returncode == 0:
        print("Test point ", test_point_id, " passed!")
        passed_test[test_point_id]=True
      else:
        print("Test point ", test_point_id, " failed!")
        print(process.stderr.decode('utf-8'))  # print the error message
        has_unpassed_test=True
        passed_test[test_point_id]=False
    else:
      print("Test point ", test_point_id, " failed to run!")
      print(process.stderr.decode('utf-8'))  # print the error message
      has_unpassed_test=True
      passed_test[test_point_id]=False

if argc>=2:
  RunTestGroup(argv[1])
else:
  for it in test_config["Groups"]:
    RunTestGroup(it["GroupName"])

if has_unpassed_test:
  print("Some tests failed!")
  exit(1)