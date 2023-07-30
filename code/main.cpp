#include "main.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <math.h>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <cstdlib>
#include <time.h>

using namespace std;

//////////// File path variables ///////////////////
string filePath = "../archive/Design_";
int designId = 2;

//////////// Global variables ///////////////////
ifstream in_file;
ofstream out_file;
vector<cascade_lib> c_lib;
vector<cascade_inst> c_inst;
vector<node> macros;
vector<node> nodes;
vector<net> nets;
vector<region_constraint> r_constraint;
site_SLICE SLICE_map;
site_DSP DSP_map;
site_BRAM BRAM_map;
site_URAM URAM_map;
vector<pair<int,int>> design_pl;
site_IO IO_map;
enum siteMap
{
    SLICE,
    DSP,
    BRAM,
    URAM,
    IO
};
int node_num;

bool EndWith(string str, string tar)
{
    //  std::string str = "example_inst";

    // Check if the string ends with "_inst"
    if (str.length() >= tar.length() && str.substr(str.length() - tar.length()) == tar)
    {
        return true;
    }
    else
    {
        return false;
    }
}
void logCascade()
{
    // for (auto n : nodes)
    // {
    //     cout << "Node Name: " << n.name << endl;
    // }
}
//////////// parsing functions ///////////////////
void parse_nodes()
{
    cout << "Parse node ..." << endl;
    in_file.open(filePath + to_string(designId) + "/design.nodes");
    string input;
    while (input != "###############################################################k##############################")
    {
        in_file >> input;
    }
    int idx = 0;
    while (in_file >> input)
    {
        string type;
        node tmp;
        tmp.name = input;
        in_file >> type;
        tmp.type = type;
        // if (tmp.name.find("CASCADE") != string::npos)
        // {
        //     for (auto c : c_inst)
        //     {
        //         if (tmp.name.find(c.name) != string::npos)
        //         {
        //             tmp.cascadeSize.first = c.row;
        //             tmp.cascadeSize.second = c.column;
        //             nodes.push_back(tmp);
        //             break;
        //         }
        //     }
        // } else nodes.push_back(tmp);

        if (tmp.name.find("CASCADE") != string::npos)
        {
            if (tmp.name.find("BRAM") != string::npos)
            {
                if (EndWith(tmp.name, "_inst"))
                {
                    for (auto c : c_inst)
                    {
                        if (tmp.name.find(c.name) != string::npos)
                        {
                            tmp.cascadeSize.first = c.row;
                            tmp.cascadeSize.second = c.column;
                            nodes.push_back(tmp);
                            break;
                        }
                    }
                }
                // else, skip those non-reference inst in cascade inst
            }
            else if (tmp.name.find("URAM") != string::npos)
            {
                if (EndWith(tmp.name, "_inst1"))
                {
                    for (auto c : c_inst)
                    {
                        if (tmp.name.find(c.name) != string::npos)
                        {
                            tmp.cascadeSize.first = c.row;
                            tmp.cascadeSize.second = c.column;
                            nodes.push_back(tmp);
                            break;
                        }
                    }
                }
                // else, skip those non-reference inst in cascade inst
            }
            else if (tmp.name.find("DSP") != string::npos)
            {
                if (EndWith(tmp.name, "i_primitive") && tmp.name.find("_instance_name1/") != string::npos)
                {
                    for (auto c : c_inst)
                    {
                        if (tmp.name.find(c.name) != string::npos)
                        {
                            tmp.cascadeSize.first = c.row;
                            tmp.cascadeSize.second = c.column;
                            nodes.push_back(tmp);
                            break;
                        }
                    }
                }
                // else, skip those non-reference inst in cascade inst
            }
            else
            {
                // cout << "Error! node name with CASCADE but isn't BRAM, URAM or DSP" <<endl;
                continue;
            }
        }
        else
        {
            nodes.push_back(tmp); // non-cascade inst
        }
    }
    node_num = nodes.size();
    in_file.close();
}
void parse_macros()
{
    cout << "Parse macros ..." << endl;
    in_file.open(filePath + to_string(designId) + "/design.macros");
    string input, name, tmp;
    int column, row;
    int cnt = 0;
    while (in_file >> input)
    {
        node target;
        for (int i = nodes.size() - 1; i >= 0; i--)
        {
            if (nodes[i].name == input)
            {
                target = nodes[i];
                macros.push_back(target);
                break;
            }
        }
    }
    in_file.close();
}
void parse_pl()
{
    cout << "Parse Design.pl ..." << endl;
    in_file.open(filePath + to_string(designId) + "/design.pl");
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
                design_pl.push_back({column,site});
                break;
            }
        }
    }
    in_file.close();
}
void parse_nets()
{
    cout << "Parse_nets ..." << endl;
    in_file.open(filePath + to_string(designId) + "/design.nets");
    string input;
    while (input != "###############################################################k##############################")
    {
        in_file >> input;
    }

    int idx = 0;
    while (in_file >> input) // eat "net"
    {
        bool flag = false;
        string netname;
        int nodecnt;
        in_file >> netname >> nodecnt;
        // cout << "Nets name: " << netname << "; idx = " << ++idx << endl;
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
    cout << "Parse_lib ..." << endl;
}
void parse_scl()
{
    cout << "Parse_scl ..." << endl;
    in_file.open(filePath + to_string(designId) + "/design.scl");
    string input, SITE;
    int x_pos, y_pos;
    while (input != "###############################################################k##############################")
    {
        in_file >> input;
    }
    while (input != "SITEMAP")
    {
        in_file >> input;
    }
    in_file >> input;
    in_file >> input;
    while (1)
    {
        in_file >> x_pos >> y_pos >> SITE;
        if (SITE == "SLICE")
        {
            if (y_pos == 0)
            {
                SLICE_map.x_pos.push_back(x_pos);
                vector<vector<string>> tmp1(300, vector<string>(16, "0"));
                vector<vector<string>> tmp2(300, vector<string>(16, "0"));
                vector<string> tmp3(300, "0");
                SLICE_map.LUT.push_back(tmp1);
                SLICE_map.FF.push_back(tmp2);
                SLICE_map.CARRY8.push_back(tmp3);
            }
        }
        else if (SITE == "DSP")
        {
            if (y_pos == 0)
            {
                DSP_map.x_pos.push_back(x_pos);
                int size = DSP_map.y_pos.size();
                vector<string> tmp(size, "0");
                DSP_map.DSP48E2.push_back(tmp);
            }
        }
        else if (SITE == "BRAM")
        {
            if (y_pos == 0)
            {
                BRAM_map.x_pos.push_back(x_pos);
                int size = BRAM_map.y_pos.size();
                vector<string> tmp(size, "0");
                BRAM_map.RAMB36E2.push_back(tmp);
            }
        }
        else if (SITE == "URAM")
        {
            if (y_pos == 0)
            {
                URAM_map.x_pos.push_back(x_pos);
                int size = URAM_map.y_pos.size();
                vector<string> tmp(size, "0");
                URAM_map.RURAM288.push_back(tmp);
            }
        }
        else if (SITE == "IO")
        {
            if (y_pos == 0)
            {
                IO_map.x_pos.push_back(x_pos);
                int size = IO_map.y_pos.size();
                vector<vector<string>> tmp(size, vector<string>(64, "0"));
                IO_map.IO.push_back(tmp);
            }
        }
        if (x_pos == 205 && y_pos == 299)
            break;
    }
    in_file >> input >> SITE;
    in_file.close();
}
void parse_region()
{
    cout << "Parse_region ..." << endl;
    in_file.open(filePath + to_string(designId) + "/design.regions");
    string input, input2;
    int ID, num_b, x_lo, y_lo, x_hi, y_hi;
    for (int i = 0; i < 78; ++i) // eat comments
    {
        in_file >> input;
    }
    while (1)
    {
        in_file >> input; // eat "RegionConstraint"
        if (in_file.eof())
        {
            in_file.close();
            return;
        }
        in_file >> input2; // "BEGIN"
        if (input == "InstanceToRegionConstraintMapping")
            break;
        in_file >> ID >> num_b;
        region_constraint rgc;
        rgc.ID = ID;
        rgc.num_boxes = num_b;
        in_file >> input >> x_lo >> y_lo >> x_hi >> y_hi;
        vector<int> tmp;
        tmp.push_back(x_lo);
        tmp.push_back(y_lo);
        tmp.push_back(x_hi);
        tmp.push_back(y_hi);
        rgc.rect.push_back(tmp);
        in_file >> input >> input2; // eat "RegionConstraint END"
        r_constraint.push_back(rgc);
    }
    while (1)
    {
        in_file >> input;
        if (input == "InstanceToRegionConstraintMapping")
            break;
        in_file >> ID;
        for (int i = 0; i < nodes.size(); ++i)
        {
            // if (nodes[i].name == input)
            if (input.find(nodes[i].name) != string::npos)
            {
                nodes[i].rg_constraint = ID;
                break;
            }
        }
    }
    in_file >> input;
    in_file.close();
}
void parse_cascade_shape()
{
    cout << "Parse_cascade_shape ..." << endl;
    in_file.open(filePath + to_string(designId) + "/design.cascade_shape");
    string input, name, tmp;
    int column, row;
    while (input != "###############################################################k##############################")
    {
        in_file >> input;
    }

    while (in_file >> input)
    {
        in_file >> name >> row >> column;
        in_file >> input;
        vector<string> buf;
        for (int i = 0; i < row * column; i++)
        {
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
    cout << "Parse_cascade_inst ..." << endl;
    in_file.open(filePath + to_string(designId) + "/design.cascade_shape_instances");
    if (in_file.fail())
    {
        in_file.close();
        return;
    }
    in_file.close();
    parse_cascade_shape();
    in_file.open(filePath + to_string(designId) + "/design.cascade_shape_instances");
    string input, name, tmp, type;
    int column, row;
    while (in_file >> type) // TYPE
    {
        if (type[0] == '#')
        {
            while (type != "###############################################################k##############################")
            {
                in_file >> type;
            }
            in_file >> type;
        }
        bool flag;
        flag = false;
        in_file >> row >> column >> name;

        in_file >> input; // eat "BEGIN"
        vector<string> buf;
        for (int i = 0; i < row * column; i++)
        {
            in_file >> tmp;
            if (tmp == "END")
            {
                flag = true;
                break;
            }
            buf.push_back(tmp);
        }
        if (flag)
        {
            string temp = buf[0];
            int leng = temp.length();
            for (int i = 7; i < 10; i++)
            {
                string ans = temp.substr(0, 40) + to_string(i) + temp.substr(41, leng - 41);
                buf.push_back(ans);
            }
        }
        else
        {
            in_file >> input;
        }
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
void parse_design()
{
    parse_cascade_inst();
    parse_nodes();
    parse_region();
    parse_macros();
    parse_pl();
    logCascade();
    parse_nets();
    parse_lib();
    parse_scl();
}
int randint(int n) {
  if ((n - 1) == RAND_MAX) {
    return rand();
  } else {
    long end = RAND_MAX / n;
    end *= n;
    int r;
    while ((r = rand()) >= end);
    return r % n;
  }
}
void showSiteMap(vector<vector<string>> &site_map, int startx, int starty, int cascadeX, int cascadeY)
{
    cout << "site map after placed. Cascade x,y = " << cascadeX << ", " << cascadeY << "\n";
    for (int i = 0; i < site_map.size(); i++)
    {
        for (int j = 0; j < site_map[0].size(); j++)
        {
            cout << site_map[i][j];
        }
        cout << endl;
    }
    cout << endl;
}

pair<int, int> updatePos(vector<vector<string>> &site_map, int startX, int startY, int endX, int endY, int cascadeX, int cascadeY)
{
    for(int k = 0; k<90000; k++)
    {
        // get random position
        
        int x = randint(endX - startX) + startX;
        int y = randint(endY - startY) + startY;
        int flag = 0;
        for(int i = 0; i<design_pl.size(); i++){
            if(x == design_pl[i].first && y == design_pl[i].second){flag = 1;break;}
        }
        if(flag)continue;
        bool available = true;
            for (int i = 0; i < cascadeX; i++)
            {
                // check boundary to prevent segmentation fault
                if(x+i>=site_map.size())break;
                for (int j = 0; j < cascadeY; j++)
                {
                    if(y+j>=site_map[0].size())break;
                    if (site_map[x + i][y + j] != "0")
                        available = false;
                    break;
                }
            }
            if (available)
            {
                for (int i = 0; i < cascadeX; i++)
                {
                    for (int j = 0; j < cascadeY; j++)
                    {
                        site_map[x + i][y + j] = "1";
                    }
                }
                // showSiteMap(site_map, x, y, cascadeX, cascadeY);
                
                return {x, y};
            }
    }
    
    return {-1, -1};
}
pair<int, int> updatePos3D(vector<vector<vector<string>>> &site_map, int startX, int startY, int endX, int endY)
{
    for (int x = startX; x < endX; x++)
    {
        for (int y = startY; y < endY; y++)
        {
            for (int z = 0; z < site_map[0][0].size(); z++)
            {
                if (site_map[x][y][z] == "0")
                {
                    site_map[x][y][z] = "1";
                    return {x, y};
                }
            }
        }
    }
    return {0, 0};
}
pair<int, int> getValidPos(node n, int rgstartX, int rgstartY, int rgendX, int rgendY)
{
    pair<int, int> validXY(-1, -1);
    int startX = -1, startY = -1, endX, endY;

    switch (n.site)
    {
    case SLICE:
        if (n.type == "CARRY8")
        {
            for (int x = 0; x < SLICE_map.x_pos.size(); ++x)
            {
                bool available = true;
                for (int len = 0; len < n.cascadeSize.second; len++)
                {
                    if (SLICE_map.x_pos[x] + len < rgstartX || SLICE_map.x_pos[x] + len > rgendX)
                        available = false;
                }
                if (available == true)
                {
                    startX = x;
                    for (int i = x; i < SLICE_map.x_pos.size(); ++i)
                    {
                        if (SLICE_map.x_pos[i] > rgendX)
                            break;
                        endX = i;
                    }
                    break;
                }
            }
            for (int y = 0; y < SLICE_map.y_pos.size(); ++y)
            {
                bool available = true;
                for (int len = 0; len < n.cascadeSize.first; len++)
                {
                    if (SLICE_map.y_pos[y] + len < rgstartY || SLICE_map.y_pos[y] + len > rgendY)
                        available = false;
                }
                if (available == true)
                {
                    startY = y;
                    for (int i = y; i < SLICE_map.y_pos.size(); ++i)
                    {
                        if (SLICE_map.y_pos[i] > rgendY)
                            break;
                        endY = i;
                    }
                    break;
                }
            }
            validXY = updatePos(SLICE_map.CARRY8, startX, startY, endX, endY, n.cascadeSize.second, n.cascadeSize.first);
            validXY.first = SLICE_map.x_pos[validXY.first];
            validXY.second = SLICE_map.y_pos[validXY.second];
        }
        else if (n.type == "LUT1" ||
                 n.type == "LUT2" ||
                 n.type == "LUT3" ||
                 n.type == "LUT4" ||
                 n.type == "LUT5" ||
                 n.type == "LUT6")
        {
            for (int x = 0; x < SLICE_map.x_pos.size(); ++x)
            {
                bool available = true;
                for (int len = 0; len < n.cascadeSize.second; len++)
                {
                    if (SLICE_map.x_pos[x] + len < rgstartX || SLICE_map.x_pos[x] + len > rgendX)
                        available = false;
                }
                if (available == true)
                {
                    startX = x;
                    for (int i = x; i < SLICE_map.x_pos.size(); ++i)
                    {
                        if (SLICE_map.x_pos[i] > rgendX)
                            break;
                        endX = i;
                    }
                    break;
                }
            }
            for (int y = 0; y < SLICE_map.y_pos.size(); ++y)
            {
                bool available = true;
                for (int len = 0; len < n.cascadeSize.first; len++)
                {
                    if (SLICE_map.y_pos[y] + len < rgstartY || SLICE_map.y_pos[y] + len > rgendY)
                        available = false;
                }
                if (available == true)
                {
                    startY = y;
                    for (int i = y; i < SLICE_map.y_pos.size(); ++i)
                    {
                        if (SLICE_map.y_pos[i] > rgendY)
                            break;
                        endY = i;
                    }
                    break;
                }
            }
            validXY = updatePos3D(SLICE_map.LUT, startX, startY, endX, endY);
            validXY.first = SLICE_map.x_pos[validXY.first];
            validXY.second = SLICE_map.y_pos[validXY.second];
        }
        else
        { // FDRE
            for (int x = 0; x < SLICE_map.x_pos.size(); ++x)
            {
                bool available = true;
                for (int len = 0; len < n.cascadeSize.second; len++)
                {
                    if (SLICE_map.x_pos[x] + len < rgstartX || SLICE_map.x_pos[x] + len > rgendX)
                        available = false;
                }
                if (available == true)
                {
                    startX = x;
                    for (int i = x; i < SLICE_map.x_pos.size(); ++i)
                    {
                        if (SLICE_map.x_pos[i] > rgendX)
                            break;
                        endX = i;
                    }
                    break;
                }
            }
            for (int y = 0; y < SLICE_map.y_pos.size(); ++y)
            {
                bool available = true;
                for (int len = 0; len < n.cascadeSize.first; len++)
                {
                    if (SLICE_map.y_pos[y] + len < rgstartY || SLICE_map.y_pos[y] + len > rgendY)
                        available = false;
                }
                if (available == true)
                {
                    startY = y;
                    for (int i = y; i < SLICE_map.y_pos.size(); ++i)
                    {
                        if (SLICE_map.y_pos[i] > rgendY)
                            break;
                        endY = i;
                    }
                    break;
                }
            }
            validXY = updatePos3D(SLICE_map.FF, startX, startY, endX, endY);
            validXY.first = SLICE_map.x_pos[validXY.first];
            validXY.second = SLICE_map.y_pos[validXY.second];
        }
        break;
    case DSP:
        for (int x = 0; x < DSP_map.x_pos.size(); ++x)
            {
                bool available = true;
                for (int len = 0; len < n.cascadeSize.second; len++)
                {
                    if (DSP_map.x_pos[x] + len < rgstartX || DSP_map.x_pos[x] + len > rgendX)
                        available = false;
                }
                if (available == true)
                {
                    startX = x;
                    for (int i = x; i < DSP_map.x_pos.size(); ++i)
                    {
                        if (DSP_map.x_pos[i] > rgendX)
                            break;
                        endX = i;
                    }
                    break;
                }
            }
            for (int y = 0; y < DSP_map.y_pos.size(); ++y)
            {
                bool available = true;
                for (int len = 0; len < n.cascadeSize.first; len++)
                {
                    if (DSP_map.y_pos[y] + len < rgstartY || DSP_map.y_pos[y] + len > rgendY)
                        available = false;
                }
                if (available == true)
                {
                    startY = y;
                    for (int i = y; i < DSP_map.y_pos.size(); ++i)
                    {
                        if (DSP_map.y_pos[i] > rgendY)
                            break;
                        endY = i;
                    }
                    // cout << rgstartY << " " << rgendY << " ";
                    // cout << DSP_map.y_pos[startY] << " " << DSP_map.y_pos[endY] << endl << endl;
                    break;
                }
            }
        validXY = updatePos(DSP_map.DSP48E2, startX, startY, endX, endY, n.cascadeSize.second, n.cascadeSize.first);
        validXY.first = DSP_map.x_pos[validXY.first];
        validXY.second = DSP_map.y_pos[validXY.second];
        break;
    case BRAM:
        for (int x = 0; x < BRAM_map.x_pos.size(); ++x)
            {
                bool available = true;
                for (int len = 0; len < n.cascadeSize.second; len++)
                {
                    if (BRAM_map.x_pos[x] + len < rgstartX || BRAM_map.x_pos[x] + len > rgendX)
                        available = false;
                }
                if (available == true)
                {
                    startX = x;
                    for (int i = x; i < BRAM_map.x_pos.size(); ++i)
                    {
                        if (BRAM_map.x_pos[i] > rgendX)
                            break;
                        endX = i;
                    }
                    break;
                }
            }
            for (int y = 0; y < BRAM_map.y_pos.size(); ++y)
            {
                bool available = true;
                for (int len = 0; len < n.cascadeSize.first; len++)
                {
                    if (BRAM_map.y_pos[y] + len < rgstartY || BRAM_map.y_pos[y] + len > rgendY)
                        available = false;
                }
                if (available == true)
                {
                    startY = y;
                    for (int i = y; i < BRAM_map.y_pos.size(); ++i)
                    {
                        if (BRAM_map.y_pos[i] > rgendY)
                            break;
                        endY = i;
                    }
                    break;
                }
            }
        validXY = updatePos(BRAM_map.RAMB36E2, startX, startY, endX, endY, n.cascadeSize.second, n.cascadeSize.first);
        validXY.first = BRAM_map.x_pos[validXY.first];
        validXY.second = BRAM_map.y_pos[validXY.second];
        break;
    case URAM:
        for (int x = 0; x < URAM_map.x_pos.size(); ++x)
            {
                bool available = true;
                for (int len = 0; len < n.cascadeSize.second; len++)
                {
                    if (URAM_map.x_pos[x] + len < rgstartX || URAM_map.x_pos[x] + len > rgendX)
                        available = false;
                }
                if (available == true)
                {
                    startX = x;
                    for (int i = x; i < URAM_map.x_pos.size(); ++i)
                    {
                        if (URAM_map.x_pos[i] > rgendX)
                            break;
                        endX = i;
                    }
                    break;
                }
            }
            for (int y = 0; y < URAM_map.y_pos.size(); ++y)
            {
                bool available = true;
                for (int len = 0; len < n.cascadeSize.first; len++)
                {
                    if (URAM_map.y_pos[y] + len < rgstartY || URAM_map.y_pos[y] + len > rgendY)
                        available = false;
                }
                if (available == true)
                {
                    startY = y;
                    for (int i = y; i < URAM_map.y_pos.size(); ++i)
                    {
                        if (URAM_map.y_pos[i] > rgendY)
                            break;
                        endY = i;
                    }
                    break;
                }
            }
        validXY = updatePos(URAM_map.RURAM288, startX, startY, endX, endY, n.cascadeSize.second, n.cascadeSize.first);
        validXY.first = URAM_map.x_pos[validXY.first];
        validXY.second = URAM_map.y_pos[validXY.second];
        break;
    case IO:
        for (int x = 0; x < IO_map.x_pos.size(); ++x)
            {
                bool available = true;
                for (int len = 0; len < n.cascadeSize.second; len++)
                {
                    if (IO_map.x_pos[x] + len < rgstartX || IO_map.x_pos[x] + len > rgendX)
                        available = false;
                }
                if (available == true)
                {
                    startX = x;
                    for (int i = x; i < IO_map.x_pos.size(); ++i)
                    {
                        if (IO_map.x_pos[i] > rgendX)
                            break;
                        endX = i;
                    }
                    break;
                }
            }
            for (int y = 0; y < IO_map.y_pos.size(); ++y)
            {
                bool available = true;
                for (int len = 0; len < n.cascadeSize.first; len++)
                {
                    if (IO_map.y_pos[y] + len < rgstartY || IO_map.y_pos[y] + len > rgendY)
                        available = false;
                }
                if (available == true)
                {
                    startY = y;
                    for (int i = y; i < IO_map.y_pos.size(); ++i)
                    {
                        if (IO_map.y_pos[i] > rgendY)
                            break;
                        endY = i;
                    }
                    break;
                }
            }
        validXY = updatePos3D(IO_map.IO, startX, startY, endX, endY);
        validXY.first = IO_map.x_pos[validXY.first];
        validXY.second = IO_map.y_pos[validXY.second];
        break;
    default:
        break;
    }

    if (startX == -1)
        cout << "Error, uninitialize startX\n";
    if (startY == -1)
        cout << "Error, uninitialize startY\n";

    /* if(validXY.first == -1 && validXY.second == -1){
        cout << n.name <<endl;
    } */

    return {validXY.first, validXY.second};
}
void placement()
{
    cout << "Placement..." << endl;
    out_file.open(filePath + to_string(designId) + "/macroplacement.pl");
    // for (auto node : nodes)
    for (auto n = macros.rbegin(); n != macros.rend(); ++n)
    {
        auto node = *n;
        // SLICE
        if (node.type == "LUT1" ||
            node.type == "LUT2" ||
            node.type == "LUT3" ||
            node.type == "LUT4" ||
            node.type == "LUT5" ||
            node.type == "LUT6" ||
            node.type == "FDRE" ||
            node.type == "FDSE" ||
            node.type == "CARRY8")
        {
            node.site = SLICE;
        }
        // DSP
        else if (node.type == "DSP48E2")
        {
            node.site = DSP;
        }
        // BRAM
        else if (node.type == "RAMB36E2")
        {
            node.site = BRAM;
        }
        // URAM
        else if (node.type == "URAM288")
        {
            node.site = URAM;
        }
        // IO
        else if (node.type == "IBUF" ||
                 node.type == "OBUF" ||
                 node.type == "BUFGCE")
        {
            node.site = IO;
        }

        /* for (auto inst : c_inst) {
            if (node.name.find(node.name) != string::npos) {
                node.isCascade = true;
                node.cascadeSize.first = inst.row;
                node.cascadeSize.second = inst.column;
            }
        } */

        int startX = 0, startY = 0;
        int endX = 206;
        int endY = 300;
        // int validX, validY;
        // cout << node.rg_constraint << endl;
        if (node.rg_constraint != -1)
        {
            auto rgc = r_constraint[node.rg_constraint];
            for (int i = 0; i < rgc.num_boxes; ++i)
            {
                startX = rgc.rect[i][0];
                startY = rgc.rect[i][1];
                endX = rgc.rect[i][2];
                endY = rgc.rect[i][3];
                auto [validX, validY] = getValidPos(node, startX, startY, endX, endY);

                if (validX == -1 || validY == -1)
                    continue;
                else
                {
                    out_file << node.name << " " << validX << " " << validY << " " << 0 << endl;
                    break;
                }
            }
        }
        else
        {
            auto [validX, validY] = getValidPos(node, startX, startY, endX, endY);
            // if (validX == -1 || validY == -1) continue;
            out_file << node.name << " " << validX << " " << validY << " " << 0 << endl;
        }
    }
    out_file.close();
}
void logInput()
{
    int logNum = 10;
    cout << endl
         << "Total node number are: " << node_num << endl
         << endl;

    // Nets
    cout << "The first " << logNum << " of nets are: " << endl
         << endl;
    cout << "###############################################################" << endl
         << endl;
    for (int i = 0; i < logNum; ++i)
    {
        cout << "NetsName: " << nets[i].name << "; ";
        cout << "Pin Number: " << nets[i].num << endl;
        for (int j = 0; j < logNum; ++j)
        {
            cout << "Nodes Name: " << nets[i].node[j].first << "; ";
            cout << "Pin Name: " << nets[i].node[j].second << endl;
        }
        cout << endl;
    }

    // Nodes
    cout << "The first " << logNum << " of nodes are : " << endl
         << endl;
    cout << "###############################################################" << endl
         << endl;
    for (int i = 0; i < logNum; ++i)
    {
        cout << "Node Name: " << nodes[i].name << "; ";
        cout << "Node Type: " << nodes[i].type << endl;
        cout << "Node column_idx: " << nodes[i].column_idx << "; ";
        cout << "Node site_idx: " << nodes[i].site_idx << "; ";
        cout << "Node bel_idx: " << nodes[i].bel_idx << "; ";
        cout << "Node fixed: " << nodes[i].fixed << endl
             << endl;
    }

    // Cascade Shape
    // for (int i=0; i<logNum; ++i) {
    //     cout << "Cascade Name: " << c_lib[i].name << " ;";
    //     cout << "Cascade row: " << c_lib[i].row << " ;";
    //     cout << "Cascade column: " << c_lib[i].column << endl;
    //     cout << "Cascade lib: " << c_lib[i].lib[0] << endl;
    // }
}
int main(int argc, char *argv[])
{
    srand( time(NULL) );
    designId = atoi(argv[1]);
    cout << "Current design Id: " << designId << endl;
    chrono::steady_clock::time_point begin = chrono::steady_clock::now();
    in_file.open(filePath + to_string(designId));
    if (in_file.fail())
    {
        in_file.close();
        cout << "This is a invalid design Id" << endl;
        return 0;
    }
    in_file.close();
    parse_design();
    chrono::steady_clock::time_point end = chrono::steady_clock::now();
    cout << "Time difference (sec) = " << (chrono::duration_cast<chrono::microseconds>(end - begin).count()) / 1000000.0 << endl;
    begin = chrono::steady_clock::now();
    placement();
    end = chrono::steady_clock::now();
    cout << "Time difference (sec) = " << (chrono::duration_cast<chrono::microseconds>(end - begin).count()) / 1000000.0 << endl;
    return 0;
}