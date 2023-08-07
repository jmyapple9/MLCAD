#include "main.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <math.h>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <cstdlib>
#include <time.h>
#include <climits>

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
vector<pair<int, int>> design_pl;
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
int idx_of_macro(string name)
{
    for (int i = 0; i < macros.size(); i++)
    {
        if (macros[i].name == name)
            return i;
    }
    return -1;
}
int boundingbox(node macro,vector<node> &macro_arr)
{
    int total = 0;
    for (int i = 0; i < macro.net.size(); i++)
    {
        int box_lowx = 100000, box_lowy = 1000000, box_highx = -1, box_highy = -1;
        // count boxes with more than one macros.
        if (macro.net[i].size() >= 2)
        {
            for (int j = 0; j < macro.net[i].size(); j++)
            {
                int target = macro.net[i][j];
                int macro_lowx = macro_arr[target].column_idx;
                int macro_lowy = macro_arr[target].site_idx;
                int macro_highx = macro_arr[target].column_idx + macro_arr[target].cascadeSize.second - 1;
                int macro_highy = macro_arr[target].site_idx + macro_arr[target].cascadeSize.first - 1;
                if (macro_lowx < box_lowx)
                    box_lowx = macro_lowx;
                if (macro_lowy < box_lowy)
                    box_lowy = macro_lowy;
                if (macro_highx > box_highx)
                    box_highx = macro_highx;
                if (macro_highy > box_highy)
                    box_highy = macro_highy;
            }
            total = total + box_highx + box_highy - box_lowx - box_lowy;
        }
    }
    return total;
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
        for (int i = nodes.size() - 1; i >= 0; i--)
        {
            if (nodes[i].name == input)
            {
                macros.push_back(nodes[i]);
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
                design_pl.push_back({column, site});
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
        int idx;
        in_file >> netname >> nodecnt;
        // cout << "Nets name: " << netname << "; idx = " << ++idx << endl;
        vector<int> tmpnet;
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
            idx = idx_of_macro(node_name);
            if (idx != -1)
                tmpnet.push_back(idx);
            in_file >> pin_name;
            tmp.push_back(make_pair(node_name, pin_name));
        }
        for (int i = 0; i < tmpnet.size(); i++)
        {
            int macro_idx = tmpnet[i];
            macros[macro_idx].net.push_back(tmpnet);
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
        for (int i = 0; i < macros.size(); ++i)
        {
            // if (nodes[i].name == input)
            if (input.find(macros[i].name) != string::npos)
            {
                macros[i].rg_constraint = ID;
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
    parse_macros();
    parse_region();
    parse_pl();
    logCascade();
    parse_nets();
    parse_lib();
    parse_scl();
}

/*

Data structure
1. macro
2. node -> net (cost function)

Function
1. boundingbox(node macro) -> HPWL with all the macros within the corresponding nets

1. cost function
2. perturbation
3. thermoequilibrium


*/

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

pair<int, int> updatePos(vector<vector<string>> &site_map, int startX, int startY, int endX, int endY, int cascadeX, int cascadeY, string name)
{
    // cout << " pos: " << startX << " " << endX << " " << startY << " " << endY << endl;
    for (int k = 0; k < 90000; k++)
    {
        // get random position
        int x = rand() % (endX - startX - cascadeX + 1) + startX;
        int y = rand() % (endY - startY - cascadeY + 1) + startY;
        int flag = 0;
        for (int i = 0; i < (int)design_pl.size(); i++)
        {
            if (x == design_pl[i].first && y == design_pl[i].second)
            {
                flag = 1;
                break;
            }
        }
        if (flag)
            continue;
        bool available = true;
        for (int i = 0; i < cascadeX; i++)
        {
            // check boundary to prevent segmentation fault
            int size = (int)site_map.size();
            if (x + i >= size)
                break;
            for (int j = 0; j < cascadeY; j++)
            {
                size = (int)site_map[x + i].size();
                if (y + j >= size)
                    break;
                if (site_map[x + i][y + j] != "0")
                {
                    available = false;
                    break;
                }
            }
        }
        if (available)
        {
            for (int i = 0; i < cascadeX; i++)
            {
                for (int j = 0; j < cascadeY; j++)
                {
                    // cout << x + i << " " << y + j << endl;
                    site_map[x + i][y + j] = name;
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
/* 
rgstartX: region constrain of x's start coordinate
rgstartY: region constrain of y's start coordinate
rgendX: region constrain of x's end coordinate
rgendY: region constrain of y's end coordinate
erase: true if we want to erase the origional macro on that, used in Perturb()
 */
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
            validXY = updatePos(SLICE_map.CARRY8, startX, startY, endX, endY, n.cascadeSize.second, n.cascadeSize.first, n.name);
            validXY.first = (validXY.first == -1 || validXY.second == -1) ? -1 : SLICE_map.x_pos[validXY.first];
            validXY.second = (validXY.first == -1 || validXY.second == -1) ? -1 : SLICE_map.y_pos[validXY.second];
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
            validXY.first = (validXY.first == -1 || validXY.second == -1) ? -1 : SLICE_map.x_pos[validXY.first];
            validXY.second = (validXY.first == -1 || validXY.second == -1) ? -1 : SLICE_map.y_pos[validXY.second];
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
            validXY.first = (validXY.first == -1 || validXY.second == -1) ? -1 : SLICE_map.x_pos[validXY.first];
            validXY.second = (validXY.first == -1 || validXY.second == -1) ? -1 : SLICE_map.y_pos[validXY.second];
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
        validXY = updatePos(DSP_map.DSP48E2, startX, startY, endX, endY, n.cascadeSize.second, n.cascadeSize.first, n.name);
        validXY.first = (validXY.first == -1 || validXY.second == -1) ? -1 : DSP_map.x_pos[validXY.first];
        validXY.second = (validXY.first == -1 || validXY.second == -1) ? -1 : DSP_map.y_pos[validXY.second];
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
        validXY = updatePos(BRAM_map.RAMB36E2, startX, startY, endX, endY, n.cascadeSize.second, n.cascadeSize.first, n.name);
        validXY.first = (validXY.first == -1 || validXY.second == -1) ? -1 : BRAM_map.x_pos[validXY.first];
        validXY.second = (validXY.first == -1 || validXY.second == -1) ? -1 : BRAM_map.y_pos[validXY.second];
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
        validXY = updatePos(URAM_map.RURAM288, startX, startY, endX, endY, n.cascadeSize.second, n.cascadeSize.first, n.name);
        validXY.first = (validXY.first == -1 || validXY.second == -1) ? -1 : URAM_map.x_pos[validXY.first];
        validXY.second = (validXY.first == -1 || validXY.second == -1) ? -1 : URAM_map.y_pos[validXY.second];
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
        validXY.first = (validXY.first == -1 || validXY.second == -1) ? -1 : IO_map.x_pos[validXY.first];
        validXY.second = (validXY.first == -1 || validXY.second == -1) ? -1 : IO_map.y_pos[validXY.second];
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
void output()
{
    cout << "Output...." << endl;
    out_file.open(filePath + to_string(designId) + "/macroplacement.pl");

    for (auto n = macros.rbegin(); n != macros.rend(); ++n)
    {
        node node2output = *n;
        out_file << node2output.name << " " << node2output.column_idx << " " << node2output.site_idx << " " << 0 << endl;
    }
    out_file.close();
}
void placement()
{
    cout << "Placement..." << endl;
    out_file.open(filePath + to_string(designId) + "/macroplacement.pl");
    // for (auto node : nodes)
    int cursor = macros.size();
    // int macrosize = macros.size();
    for (auto n = macros.rbegin(); n != macros.rend(); ++n)
    {
        cursor--;
        // cout << "placing macro number " << cursor;
        auto node2place = *n;
        // SLICE
        if (node2place.type == "LUT1" ||
            node2place.type == "LUT2" ||
            node2place.type == "LUT3" ||
            node2place.type == "LUT4" ||
            node2place.type == "LUT5" ||
            node2place.type == "LUT6" ||
            node2place.type == "FDRE" ||
            node2place.type == "FDSE" ||
            node2place.type == "CARRY8")
        {
            node2place.site = SLICE;
        }
        // DSP
        else if (node2place.type == "DSP48E2")
        {
            node2place.site = DSP;
        }
        // BRAM
        else if (node2place.type == "RAMB36E2")
        {
            node2place.site = BRAM;
        }
        // URAM
        else if (node2place.type == "URAM288")
        {
            node2place.site = URAM;
        }
        // IO
        else if (node2place.type == "IBUF" ||
                 node2place.type == "OBUF" ||
                 node2place.type == "BUFGCE")
        {
            node2place.site = IO;
        }

        /* for (auto inst : c_inst) {
            if (node2place.name.find(node2place.name) != string::npos) {
                node2place.isCascade = true;
                node2place.cascadeSize.first = inst.row;
                node2place.cascadeSize.second = inst.column;
            }
        } */

        int startX = 0, startY = 0;
        int endX = 206;
        int endY = 300;
        // int validX, validY;
        // cout << node2place.rg_constraint << endl;
        if (node2place.rg_constraint != -1)
        {
            auto rgc = r_constraint[node2place.rg_constraint];
            for (int i = 0; i < rgc.num_boxes; ++i)
            {
                // cout<<" in ";
                startX = rgc.rect[i][0];
                startY = rgc.rect[i][1];
                endX = rgc.rect[i][2];
                endY = rgc.rect[i][3];
                auto [validX, validY] = getValidPos(node2place, startX, startY, endX, endY);
                // cout<<validX<<" "<<validY<<endl;
                // cout<<cursor<<" "<<macros[cursor].name<<endl;
                if (validX == -1 || validY == -1)
                    continue;
                else
                {
                    auto newnode = node2place;
                    newnode.column_idx = (int)validX;
                    newnode.site_idx = (int)validY;
                    newnode.bel_idx = 0;
                    macros[cursor] = newnode;
                    break;
                }
            }
        }
        else
        {
            auto [validX, validY] = getValidPos(node2place, startX, startY, endX, endY);
            // cout<<" in "<<validX<<" "<<validY<<endl;
            // if (validX == -1 || validY == -1) continue;
            auto newnode = node2place;
            newnode.column_idx = (int)validX;
            newnode.site_idx = (int)validY;
            newnode.bel_idx = 0;
            macros[cursor] = newnode;
        }
    }
    out_file.close();
}
bool if_macro_legal()
{
    int cnt = -1;
    for (auto n = macros.rbegin(); n != macros.rend(); ++n)
    {
        cnt++;
        node node2output = *n;
        int rg_constraint_id = node2output.rg_constraint;
        if (rg_constraint_id != -1)
        {
            auto rgc = r_constraint[rg_constraint_id];
            for (int i = 0; i < rgc.num_boxes; ++i)
            {
                int startX = rgc.rect[i][0];
                int startY = rgc.rect[i][1];
                int endX = rgc.rect[i][2];
                int endY = rgc.rect[i][3];
                int low_x = node2output.column_idx;
                int high_x = node2output.column_idx + node2output.cascadeSize.second - 1;
                int low_y = node2output.site_idx;
                int high_y = node2output.site_idx + node2output.cascadeSize.first - 1;
                if (low_x < startX || high_x >= endX)
                {
                    cout << "macros number " << cnt << " is not legal." << endl;
                    cout << "x: " << low_x << " " << high_x << " , " << startX << " " <<  endX;
                    cout << "y: " << low_y << " " << high_y << " , " << startY << " " <<  endY << endl;
                    return false;
                }
                if (low_y < startY || high_y >= endY)
                {
                    cout << "macros number " << cnt << " is not legal." << endl;
                    cout << "x: " << low_x << " " << high_x << " , " << startX << " " << endX;
                    cout << "y: " << low_y << " " << high_y << " , " << startY << " " << endY << endl;
                    return false;
                }
            }
        }
    }
    cout << "Legal!" << endl;
    return true;
}
bool accept(double T, double cost)
{
    // cout << cost << endl;
    if (T == 0)
        return false;
    double r = ((double)rand() / (INT_MAX));
    double power = -1 * cost / T;
    return exp(power) > r;
}


// Improvement option:
// 1. add Perturb movement: Move random macro up, down, left, or right with random distance
// 2. add Perturb movement: Randomly put a macro to another position
// 3. initialization without macro next to each other

vector<node> Perturb(vector<node> sNow)
{
    /* int perturbMode = rand() % 2; // 0 or 1
    switch (perturbMode)
    {
    case 0:

        break;
    case 1:

        break;
    default:
        cout << "Error! perturb mode " << perturbMode << " not exist." << endl;
        break;
    } */
    int skip = 0;
    int times = 0;
    while (1)
    {
        times++;
        bool flag1 = true, flag2 = true;
        int r1 = rand() % (sNow.size());
        int r2 = rand() % (sNow.size());
        auto &n1 = sNow[r1];
        auto &n2 = sNow[r2];
        auto n1rgc = n1.rg_constraint;
        auto n2rgc = n2.rg_constraint;
        if (n1.site != n2.site ||
            n1.cascadeSize.first != n2.cascadeSize.first ||
            n1.cascadeSize.second != n2.cascadeSize.second)
        {
            skip++;
            continue;
        }
        // n1rgc 的其中一組要可以放 n2coord
        if (n1rgc != -1)
        {
            flag1 = false;
            for (int i = 0; i < r_constraint[n1rgc].num_boxes; ++i)
            {
                auto rgc = r_constraint[n1rgc].rect[i];
                auto startX = rgc[0];
                auto startY = rgc[1];
                auto endX = rgc[2];
                auto endY = rgc[3];
                auto n2X = n2.column_idx;
                auto n2Y = n2.site_idx;
                if (startX <= n2X && endX >= n2X && startY <= n2Y && endY >= n2Y)
                {
                    flag1 = true;
                    break;
                };
            }
        }

        // n2rgc 的其中一組要可以放 n1coord
        if (n2rgc != -1)
        {
            flag2 = false;
            for (int i = 0; i < r_constraint[n2rgc].num_boxes; ++i)
            {
                auto rgc = r_constraint[n2rgc].rect[i];
                auto startX = rgc[0];
                auto startY = rgc[1];
                auto endX = rgc[2];
                auto endY = rgc[3];
                auto n1X = n1.column_idx;
                auto n1Y = n1.site_idx;
                if (startX <= n1X && endX >= n1X && startY <= n1Y && endY >= n1Y)
                {
                    flag2 = true;
                    break;
                };
            }
        }

        if (flag1 && flag2)
        {
            swap(n1.column_idx, n2.column_idx);
            swap(n1.site_idx, n2.site_idx);
            break;
        }
    }
    // cout << "during perturb: " << times << " try and " << skip << "skip"<<endl;
    return sNow;
}

int cost(vector<node> &tmp_ans)
{
    int total = 0;
    for (auto m : tmp_ans)
    {
        total += boundingbox(m,tmp_ans);
    }
    if (total < 0)
        cout << "overflow!!!!!!!" << endl;
    return total;
}

int INIT_T = 2000;
int END_T = 20;
int printCount = 0;
void decrease(double &T){
    // T *= 0.999;
    T -= 20;
}

int ThermalEquilibrium(double T)
{
    int a = INIT_T / 10;
    int b = a/2 + END_T;
    if (printCount++ == 0){
        cout << "In ThermalEquilibrium: " << (T + a) / b << endl;
        printCount = 0;
    }
    return (T + a) / b;
}

void SA()
{
    cout << "SA...." << endl;
    vector<node> sNow, sNext, sBest;
    double T, endT;
    sNow = macros;
    sBest = macros;
    T = INIT_T;
    endT = END_T;
    while (T > endT)
    {
        // cout << "T: " << T << endl;
        int cnt = ThermalEquilibrium(T);
        while (cnt--)
        {
            // cout << "sNext: " << cost(sNext) << endl;
            sNext = Perturb(sNow);
            // cout << "sNow: " << cost(sNow) << endl;
            if (cost(sNext) < cost(sNow))
            {
                sNow = sNext;
                if (cost(sNow) < cost(sBest))
                    sBest = sNow;
            }
            else if (accept(T, cost(sNext) - cost(sNow)))
                sNow = sNext;
        }
        decrease(T);
    }
    cout << "HPWL before SA " << setw(8) << cost(macros) << endl;
    cout << "HPWL after  SA " << setw(8) << cost(sBest)  << endl;
    cout << "Difference     " << setw(8) << cost(macros) - cost(sBest) << endl;
    macros = sBest;
}

/*
1. T = 2000
2. endT = 20
3. Perturb
4. ThermalEquilibrium = [1, 10]
5. Decrease = -20
*/

int main(int argc, char *argv[])
{
    srand(0);
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
    
    do{
        placement();
    }while(!if_macro_legal());
    
    SA();
    output();
    cout << "Check if placement violate the constraint..." << endl;
    if_macro_legal();
        
    end = chrono::steady_clock::now();
    cout << "Time difference (sec) = " << (chrono::duration_cast<chrono::microseconds>(end - begin).count()) / 1000000.0 << endl;
    return 0;
}