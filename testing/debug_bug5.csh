#!/bin/csh

foreach f (`ls */design.cascade_shape_instances`)

    echo "editing $f"

    sed -i "s#URAM_cascade_8x2#URAM_CASCADE_8x2#g" $f

    sed -i "s#BRAM_cascade#BRAM_CASCADE#g" $f

    sed -i "s#DSP_cascade#DSP_CASCADE#g" $f

end