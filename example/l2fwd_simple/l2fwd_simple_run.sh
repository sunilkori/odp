#!/bin/bash
#
# Copyright (c) 2016-2018, Linaro Limited
# All rights reserved.
#
# SPDX-License-Identifier:     BSD-3-Clause
#

SRC_DIR=$(dirname $0)
TEST_EXAMPLE_DIR=platform/$ODP_PLATFORM/test/example
PLATFORM_TEST_EXAMPLE=${SRC_DIR}/../../${TEST_EXAMPLE_DIR}

if  [ -f ./pktio_env ]; then
  . ./pktio_env
elif [ -f ${PLATFORM_TEST_EXAMPLE}/l2fwd_simple/pktio_env ]; then
        . ${PLATFORM_TEST_EXAMPLE}/l2fwd_simple/pktio_env
else
  echo "BUG: unable to find pktio_env!"
  echo "pktio_env has to be in current or platform example directory"
  exit 1
fi

setup_interfaces

if [ "$(which stdbuf)" != "" ]; then
	STDBUF="stdbuf -o 0"
else
	STDBUF=
fi

$STDBUF ./odp_l2fwd_simple${EXEEXT} $IF0 $IF1 02:00:00:00:00:01 02:00:00:00:00:02 -t 2
STATUS=$?

if [ "$STATUS" -ne 0 ]; then
  echo "Error: status was: $STATUS, expected 0"
  exit 1
fi

validate_result

cleanup_interfaces

exit 0
