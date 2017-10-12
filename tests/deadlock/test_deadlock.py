import os
import subprocess
import shlex
import signal

def test_main(utils):
  utils.compile_and_run(__file__, signal=signal.SIGABRT)
