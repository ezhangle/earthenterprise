#!/usr/bin/python
#
# Copyright 2017 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


"""Builds and tests Portable Server file unpacker library.

Builds the library for the 3 support systems: Mac, Linux
and Windows. Does basic tests to make sure that it can
read files and 3d and 2d packet data from globes and
maps.
"""

import os
import shutil
import sys
import util


def BuildLibrary(os_dir, ignore_results):
  """Returns whether able to build file unpacker library."""
  try:
    os.mkdir("dist")
  except OSError:
    pass  # ok if it already exists

  os.chdir("dist")
  fp = open("../%s/build_lib" % os_dir)
  for line in fp:
    result = util.ExecuteCmd(line, use_shell=True)
    if result:
      if ignore_results:
        print result
      else:
        print "FAIL: %s" % result
        fp.close()
        return False
  fp.close()
  return True


def RunTests():
  """Runs tests using the generated library."""
  print "Running tests ..."
  shutil.copyfile("../util.py", "util.py")
  shutil.copyfile("../test.py", "test.py")

  old_path = sys.path
  sys.path = sys.path + [os.getcwd()]
  import test
  sys.path = old_path

  test.main()

def main():
  """Main for build and test."""
  if (len(sys.argv) != 2 or
      sys.argv[1].lower() not in ["mac", "windows", "linux"]):
    print "Usage: build_and_test.py <OS_target>"
    print
    print "<OS_target> can be Mac, Windows, or Linux"
    return

  os.chdir(sys.path[0])
  os_dir = "%s%s" % (sys.argv[1][0:1].upper(), sys.argv[1][1:].lower())
  print "Build and test file unpacker library for %s Portable Server." % os_dir

  if BuildLibrary(os_dir, sys.argv[1].lower()=="windows"):
    print "Library built."
    RunTests()


if __name__ == "__main__":
  main()
