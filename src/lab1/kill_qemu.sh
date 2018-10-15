#!/bin/bash

proc_id=`ps aux | grep qemu-sys | awk '{print $2}' | head -1`
kill -9 $proc_id
