import os
import subprocess
import shlex

def test_basic(utils):
  utils.compile_and_run(__file__)
