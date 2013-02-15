#!/bin/sh

# start data-router
if (/bin/ps -e | /bin/grep data-router); then
  /bin/echo "Already DR activated. No need launch DR"
else
  /bin/echo "Launch DR"
  /usr/bin/data-router &
fi
