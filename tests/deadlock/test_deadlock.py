import os
import subprocess
import shlex
import signal

def test_main(utils):
  utils.compile(__file__)

  # Test deadlock which should abort
  utils.run(cmd="safe", signal=signal.SIGABRT);

  # Test deadlock with safety turned off which
  # should succeed since those locks will not allow
  # to be taken
  utils.run(cmd="unsafe")
