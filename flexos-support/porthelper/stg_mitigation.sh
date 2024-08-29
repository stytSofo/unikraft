#!/bin/bash
# Make sure that stg instruction does not appear in the object files of the compartments
# We cannot allow suhc instructions to be used inside the compartments.
# ERIM also checks the compartments for the PKRU instruction for Intel X86 MPK
for obj_file in *.o; do
    echo "Analyzing $obj_file..."
    objdump -d "$obj_file" | grep "stg" > /dev/null
    if [ $? -eq 0 ]; then
        echo "Found 'stg' instructions in $obj_file"
    else
        echo "No 'stg' instructions found in $obj_file"
    fi
done
