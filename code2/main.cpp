#include "main.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <math.h>
#include <set>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

ifstream in_file;
ofstream out_file;
vector<cascade_lib> c_lib;
vector<cascade_inst> c_inst;
vector<node> nodes;
vector<net> nets;
vector<region_constraint> r_constraint;
site_SLICE SLICE_map;
site_DSP DSP_map;
site_BRAM BRAM_map;
site_URAM URAM_map;
site_IO IO_map;
int node_num;
////////////parsing functions///////////////////
void parse_nodes()
{
    in_file.open("./design.nodes");
    string input;
    while (input != "###############################################################k##############################")
    {
        in_file >> input;
    }
    while (in_file >> input)
    {
        string type;
        node tmp;
        tmp.name = input;
        in_file >> type;
        tmp.type = type;
        nodes.push_back(tmp);
    }
    node_num = nodes.size();
    in_file.close();
}
void parse_pl()
{
    in_file.open("./design.pl");
    string input;
    while (input != "###############################################################k##############################")
    {
        in_file >> input;
    }
    while (in_file >> input)
    {
        string name = input;
        string fixedornot;
        int column, site, bel, idx;
        in_file >> column >> site >> bel >> fixedornot;
        for (int i = 0; i < nodes.size(); i++)
        {
            if (nodes[i].name == name)
            {
                nodes[i].column_idx = column;
                nodes[i].site_idx = site;
                nodes[i].bel_idx = bel;
                if (fixedornot == "FIXED")
                    nodes[i].fixed = true;
                break;
            }
        }
    }
    in_file.close();
}
void parse_nets()
{
    in_file.open("./design.nets");
    string input;
    while (input != "###############################################################k##############################")
    {
        in_file >> input;
    }
    while (in_file >> input)
    {
        bool flag = false;
        string netname;
        int nodecnt;
        in_file >> netname >> nodecnt;
        vector<pair<string, string>> tmp;
        for (int i = 0; i < nodecnt; i++)
        {
            string node_name, pin_name;
            in_file >> node_name;
            if (node_name == "endnet")
            {
                flag = true;
                break;
            }
            in_file >> pin_name;
            tmp.push_back(make_pair(node_name, pin_name));
        }
        net N;
        N.name = netname;
        N.num = tmp.size();
        N.node = tmp;
        nets.push_back(N);
        if (!flag)
            in_file >> netname;
    }
    in_file.close();
}
void parse_lib()
{
}
void parse_scl()
{
    in_file.open("./design.scl");
    string input, SITE;
    int x_pos, y_pos;
    while (input != "####################################################################################"){
        in_file >> input;
    }
    while (input != "SITEMAP"){
        in_file >> input;
    }
    in_file >> input;
    in_file >> input;
    while(1){
        in_file >> x_pos >> y_pos >> SITE;
        if(SITE == "SLICE"){
            if(y_pos == 0){
                SLICE_map.x_pos.push_back(x_pos);
                vector<vector<string>> tmp1(300, vector<string>(16, "0"));
                vector<vector<string>> tmp2(300, vector<string>(16, "0"));
                vector<string> tmp3(300, "0");
                SLICE_map.LUT.push_back(tmp1);
                SLICE_map.FF.push_back(tmp2);
                SLICE_map.CARRY8.push_back(tmp3);
            }
        }
        else if(SITE == "DSP"){
            if(y_pos == 0){
                DSP_map.x_pos.push_back(x_pos);
                int size = DSP_map.y_pos.size();
                vector<string> tmp(size, "0");
                DSP_map.DSP48E2.push_back(tmp);
            }
        }
        else if(SITE == "BRAM"){
            if(y_pos == 0){
                BRAM_map.x_pos.push_back(x_pos);
                int size = BRAM_map.y_pos.size();
                vector<string> tmp(size, "0");
                BRAM_map.RAMB36E2.push_back(tmp);
            }
        }
        else if(SITE == "URAM"){
            if(y_pos == 0){
                URAM_map.x_pos.push_back(x_pos);
                int size = URAM_map.y_pos.size();
                vector<string> tmp(size, "0");
                URAM_map.RURAM288.push_back(tmp);
            }
        }
        else if(SITE == "IO"){
            if(y_pos == 0){
                IO_map.x_pos.push_back(x_pos);
                int size = IO_map.y_pos.size();
                vector<vector<string>> tmp(size, vector<string>(64, "0"));
                IO_map.IO.push_back(tmp);
            }
        }
        if(x_pos == 205 && y_pos == 299) break;
    }
    in_file >> input >> SITE;
    in_file.close();
}
void parse_region()
{
    in_file.open("./design.regions");
    string input, input2;
    int ID, num_b, x_lo, y_lo, x_hi, y_hi;
    while (input != "###############################################################k##############################")
    {
        in_file >> input;
    }
    for(int i = 0; i < 78; ++i){
        in_file >> input;
    }
    while(1){
        in_file >> input >> input2;
        if(input == "InstanceToRegionConstraintMapping")
            break;
        in_file >> ID >> num_b;
        region_constraint rgc;
        rgc.ID = ID; rgc.num_boxes = num_b;
        in_file >> input >> x_lo >> y_lo >> x_hi >> y_hi;
        vector<int> tmp; tmp.push_back(x_lo); tmp.push_back(y_lo); tmp.push_back(x_hi); tmp.push_back(y_hi);
        rgc.rect.push_back(tmp);
        in_file >> input >> input2;
        r_constraint.push_back(rgc);
    }
    while(1){
        in_file >> input;
        if(input == "InstanceToRegionConstraintMapping") break;
        in_file >> ID;
        for(int i = 0; i < nodes.size(); ++i){
            if(nodes[i].name == input){
                nodes[i].rg_constraint = ID;
                break;
            }
        }
    }
    in_file >> input;
    in_file.close();
}
void parse_cascade_shape(){
    in_file.open("./design.cascade_shape");
    string input, name, tmp;
    int column, row;
    while (input != "###############################################################k##############################")
    {
        in_file >> input;
    }
    while(in_file >> input){
        in_file >> name >> row >> column;
        in_file >> input;
        vector<string> buf;
        for(int i = 0; i<row*column; i++){
            in_file >> tmp;
            buf.push_back(tmp);
        }
        in_file >> input;
        cascade_lib new_lib;
        new_lib.name = name;
        new_lib.row = row;
        new_lib.column = column;
        new_lib.lib = buf;
        c_lib.push_back(new_lib);
    }
    in_file.close();
}
void parse_cascade_inst()
{
    in_file.open("./design.cascade_shape_instances");
    if(!in_file.is_open()) return;
    in_file.close();
    parse_cascade_shape();
    in_file.open("./design.cascade_shape_instances");
    string input, name, tmp,type;
    int column, row;
    while (input != "###############################################################k##############################")
    {
        in_file >> input;
    }
    while(in_file >> input){
        in_file >> name >> row >> column >> type;
        in_file >> input;
        vector<string> buf;
        for(int i = 0; i<row*column; i++){
            in_file >> tmp;
            buf.push_back(tmp);
        }
        in_file >> input;
        cascade_inst new_inst;
        new_inst.name = name;
        new_inst.type = type;
        new_inst.row = row;
        new_inst.column = column;
        new_inst.inst = buf;
        c_inst.push_back(new_inst);
    }
    in_file.close();
}

void parse_design(){
    parse_nodes();
    parse_scl();
    parse_pl();
    parse_nets();
    parse_cascade_inst();
    parse_region();
}
int main()
{
    parse_design();
    return 0;
}