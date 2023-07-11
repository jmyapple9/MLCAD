echo "Start running macro placement!"

cd ./code
g++ -o main main.cpp -O3 --std=c++17
./main

echo "Finish parsing benchmark!"