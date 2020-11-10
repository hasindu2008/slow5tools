#!/bin/sh

DATA_DIR="$1"

wc -c "$DATA_DIR"/bench.*low5*
wc -c "$DATA_DIR"/../fast5/* | tail -1
