#!/bin/bash

mkdir -p $2
./server 12345 $1 1000 $2/log
