#!/bin/sh

# foreach f (`ls */design.regions`)
for i in $(seq 1 3)
do
    f="Design_${i}/design.regions"
    echo "editing $f"
    sed -i "s#/DSP_ALU_INST##g" "$f"
done