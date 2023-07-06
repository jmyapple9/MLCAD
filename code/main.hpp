#include <fstream>
#include <iostream>
#include <math.h>
#include <set>
#include <sstream>
#include <string>
#include <vector>
using namespace std;
class node
{
public:
    string name;
    string type;
    int column_idx;
    int site_idx;
    int bel_idx;
    int rg_constraint;
    bool fixed;
    node();
};

class pin
{
public:
    string name;
    int IO;
    pin();
};

class net
{
public:
    net();
    string name;
    int num;
    vector<pair<string, string>> node; // pair <node name, pin name>
};

class cascade_lib
{
public:
    string name;
    int row;
    int column;
    vector<string> lib;
    cascade_lib();
};

class cascade_inst
{
public:
    string name;
    string type;
    int row;
    int column;
    vector<string> inst;
    cascade_inst();
};

// SLICE has 16 LUT, 16 FF, 1 CARRY8 resources.
// In each column, has 300 SLICE(0 -> 1 -> 2)
class site_SLICE
{
public:
    vector<int> x_pos;
    vector<int> y_pos;
    vector<vector<vector<string>>> LUT; // you can change string to bool if u want to represnt it as "placed" or "placeable"
    vector<vector<vector<string>>> FF;
    vector<vector<string>> CARRY8;
    site_SLICE();
};

// DSP has 1 DSP48E2 resource.
// In each column, the y position will be like 0 -> 2 -> 5 -> 7 -> 10.
class site_DSP
{
public:
    vector<int> x_pos; 
    vector<int> y_pos;
    vector<vector<string>> DSP48E2;
    site_DSP(); 
};

// BRAM has 1 RAMB36E2 resource
// In each column, the y position will be like 0 -> 5 -> 10 -> 15.
class site_BRAM
{
public:
    vector<int> x_pos; 
    vector<int> y_pos;
    vector<vector<string>> RAMB36E2;
    site_BRAM();
};

// URAM has 1 URAM288 resource
// In each column, the y position will be like 0 -> 15 -> 30 -> 45.
class site_URAM
{
public:
    vector<int> x_pos; 
    vector<int> y_pos;
    vector<vector<string>> RURAM288;
    site_URAM();
};

// IO has 64 IO resources
// In each column, the y position will be like 0 -> 30 -> 60 -> 90.
class site_IO
{
public:
    vector<int> x_pos; 
    vector<int> y_pos;
    vector<vector<vector<string>>> IO;
    site_IO();
};

class region_constraint // box/rect <xLo> <yLo> <xHi> <yHi>
{
public:
    int ID, num_boxes;
    vector<vector<int>> rect; // e.g. rect 0 0 211 60 => rect[0][0] = 0, rect[0][1] = 0, rect[0][2] = 21, rect[0][3] = 60
    region_constraint();
};

cascade_lib::cascade_lib() {}
cascade_inst::cascade_inst() {}
net::net() {}
pin::pin() {}
region_constraint::region_constraint() {}
node::node()
{
    bel_idx = -1;
    site_idx = -1;
    column_idx = -1;
    rg_constraint = -1;
    fixed = false;
}
site_SLICE::site_SLICE() {
    for(int i = 0; i < 300; ++i){
        y_pos.push_back(i);
    }
}
site_DSP::site_DSP() {
    bool flag = false;
    for(int i = 0; i < 300;){
        y_pos.push_back(i);
        if(!flag) i += 2;
        else i += 3;
        flag = !flag;
    }
}
site_BRAM::site_BRAM() {
    for(int i = 0; i < 300; i += 5){
        y_pos.push_back(i);
    }
}
site_URAM::site_URAM() {
    for(int i = 0; i < 300; i += 15){
        y_pos.push_back(i);
    }
}
site_IO::site_IO() {
    for(int i = 0; i < 300; i += 30){
        y_pos.push_back(i);
    }
}

vector<string> table{"FDRE", "FDSE", "LUT6", "LUT5", "LUT4", "LUT2", "LUT1", "CARRY8", "DSP48E2", "RAMB36E2", "BUFGCE", "IBUF", "OBUF", "URAM288"};