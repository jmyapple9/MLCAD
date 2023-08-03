echo "Start running macro placement!"

cd ./code
g++ -o main main.cpp -O3 --std=c++17
# ./main

# set vivado=1 will try routing with vivado
vivado=0

for i in {2..2}
do
    if [ -d "../archive/Design_$i" ]; then
        echo "running design $i"
        ./main $i
    fi

    if [ $vivado -eq 1 ]; then
        cd ~
        source /tools/Xilinx/Vivado/2021.1/settings64.sh
        export MYVIVADO="/home/ray99915/vivado_patch_dir/vivado/"
        cd ./archive/Design_$i
        vivado -mode tcl -so place_route.tcl
    fi
done

echo "Finish parsing benchmark!"

cd ~