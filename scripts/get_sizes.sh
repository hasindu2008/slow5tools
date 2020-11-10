#!/bin/sh

DATA_DIR="$1"

wc -c "$DATA_DIR"/bench.*low5* | cut -f1
