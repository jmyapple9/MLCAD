echo "Start running macro placement!"

cd ./code
g++ -o main main.cpp -O3 --std=c++17
# ./main

for i in {2..2}
do
    if [ -d "../archive/Design_$i" ]; then
        echo "running design $i"
        ./main $i
    fi
done

echo "Finish parsing benchmark!"