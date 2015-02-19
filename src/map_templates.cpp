#include "map_templates.h"

#include "init.h"

#include <vector>
#include <assert.h>

using namespace std;

namespace Map_templ_handling
{

namespace
{

Map_templ templates_[int(Map_templ_id::END)];

struct Translation
{
    Translation(char c, const Map_templ_cell& map_templ_cell) :
        CH   (c),
        cell (map_templ_cell) {}

    const char          CH;
    const Map_templ_cell  cell;
};

Map_templ_cell ch_to_cell(const char CH, const vector<Translation>& translations)
{
    for (const Translation& translation : translations)
    {
        if (translation.CH == CH)
        {
            return translation.cell;
        }
    }
    TRACE << "Failed to translate char: " <<  CH << endl;
    assert(false);
    return Map_templ_cell();
}

void mk_templ(const string& str, const Map_templ_id id,
             const vector<Translation>& translations)
{
    Map_templ& templ = templates_[int(id)];

    vector<Map_templ_cell> inner;

    for (const auto ch : str)
    {
        switch (ch)
        {
        case ';':
            //Delimiting character (";") found, inner vector is pushed to outer
            templ.add_row(inner);
            inner.clear();
            break;

        case '#': inner.push_back({Feature_id::wall});           break;
        case '.': inner.push_back({Feature_id::floor});          break;
        case ' ': inner.push_back({});                          break;
        default:  inner.push_back(ch_to_cell(ch, translations));  break;
        }
    }
}

void init_templs()
{
    //Blank level with correct dimensions to copy/paste when making new templates
//  "################################################################################;"
//  "#..............................................................................#;"
//  "#..............................................................................#;"
//  "#..............................................................................#;"
//  "#..............................................................................#;"
//  "#..............................................................................#;"
//  "#..............................................................................#;"
//  "#..............................................................................#;"
//  "#..............................................................................#;"
//  "#..............................................................................#;"
//  "#..............................................................................#;"
//  "#..............................................................................#;"
//  "#..............................................................................#;"
//  "#..............................................................................#;"
//  "#..............................................................................#;"
//  "#..............................................................................#;"
//  "#..............................................................................#;"
//  "#..............................................................................#;"
//  "#..............................................................................#;"
//  "#..............................................................................#;"
//  "#..............................................................................#;"
//  "################################################################################;";

    //Filled version
//  "################################################################################;"
//  "################################################################################;"
//  "################################################################################;"
//  "################################################################################;"
//  "################################################################################;"
//  "################################################################################;"
//  "################################################################################;"
//  "################################################################################;"
//  "################################################################################;"
//  "################################################################################;"
//  "################################################################################;"
//  "################################################################################;"
//  "################################################################################;"
//  "################################################################################;"
//  "################################################################################;"
//  "################################################################################;"
//  "################################################################################;"
//  "################################################################################;"
//  "################################################################################;"
//  "################################################################################;"
//  "################################################################################;"
//  "################################################################################;";

    //----------------------------------------------------------------- CHURCH
    string str =
        "             ,,,,,,,,,,,     ;"
        "          ,,,,,######,,,,    ;"
        " ,,,,,,,,,,,,,,#v..v#,,,,,   ;"
        ",,,,,,,,,,,,,###....###,,,,  ;"
        ",,#####,#,,#,#.#....#.#,,,,, ;"
        ",,#v.v########.#....#.######,;"
        "..#...#v.................v##,;"
        ".,#.#.#..[.[.[.[...[.[....>#,;"
        "..+...+*****************-..#,;"
        ".,#.#.#..[.[.[.[...[.[.....#,;"
        ".,#...#v.................v##,;"
        ",,#v.v########.#....#.######,;"
        ",,#####,#,,#,#.#....#.#,,,,, ;"
        ",,,,,,,,,,,,,###....###,,,,  ;"
        " ,,,,,,,,,,,,,,#v..v#,,,,,   ;"
        "         ,,,,,,######,,,,    ;"
        "            ,,,,,,,,,,,,     ;";

    mk_templ(str, Map_templ_id::church, vector<Translation>
    {
        {',', {Feature_id::grass}},
        {'v', {Feature_id::brazier}},
        {'[', {Feature_id::church_bench}},
        {'-', {Feature_id::altar}},
        {'*', {Feature_id::carpet}},
        {'>', {Feature_id::stairs}},
        {'+', {Feature_id::floor,  Actor_id::END, Item_id::END, 1}} //Doors
    });

    //----------------------------------------------------------------- EGYPT
    str =
        "################################################################################;"
        "###...################################........................##################;"
        "###.1.###############################..######################..#################;"
        "###...##############################..#########################.################;"
        "####.##############################..####v....################|....|############;"
        "####.#############################..####..###v..##############......############;"
        "####.##########################.....####..######.v############|....|############;"
        "#####.####.#.#.#.#.###########..######v..#######..###############.##############;"
        "######.##|C........|#########.#######..##########..############..###############;"
        "#######.#...........##.....##.#####...############v.##########..################;"
        "########....M...C....#.#.#.#..####..###...#########..########..#################;"
        "#########..P.....C.#..........####.####.@...........v#######..##################;"
        "########....M...C....#.#.#.#..##|..|###...#################.v###################;"
        "#######.#...........##.....##.##....######################...###################;"
        "######.##|C........|#########.##|..|########......#######.v##.##################;"
        "#####.####.#.#.#.#.##########.####.########..###v..#####..####.#################;"
        "####.########################.####...#####..#####v..###..######.################;"
        "####.########################..#####..###..#######v.....########.###############;"
        "###...########################...####.....#############.#########.###|...|######;"
        "###.2.##########################...##################v..##########........######;"
        "###...############################v....................##############|...|######;"
        "################################################################################;";

    mk_templ(str, Map_templ_id::egypt, vector<Translation>
    {
        {'@', {Feature_id::floor, Actor_id::END, Item_id::END, 1}},  //Start
        {'v', {Feature_id::brazier}},
        {'|', {Feature_id::pillar}},
        {'S', {Feature_id::statue}},
        {'P', {Feature_id::floor, Actor_id::khephren}},
        {'M', {Feature_id::floor, Actor_id::mummy}},
        {'C', {Feature_id::floor, Actor_id::croc_head_mummy}},
        {'1', {Feature_id::floor, Actor_id::END, Item_id::END, 2}},  //Stair candidate #1
        {'2', {Feature_id::floor, Actor_id::END, Item_id::END, 3}}   //Stair candidate #2
    });

    //----------------------------------------------------------------- LENG
    str =
        "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx;"
        "%%%%%%%%%%-%%--%--%%--%%%--%%--%%-%-%%%--%-%%-x,,,,,x,,,,,x,,,,,x,,,,,,,,,,,,,,x;"
        "%@----%--------%---%---%--------%----%-----%--x,,,,,x,,,,,x,,,,,x,,,,,,,,,,,,,,x;"
        "%%--------------------------------------------xxx,xxxxx,xxxxx,xxx,,,,,,,,,,,,,,x;"
        "%-------------%--------------------------%----x,,,,,,,,,,,,,,,,,x,,,,,,,,,,,,,,x;"
        "%%------------%-------------------------------x,,,xxx,xxxxx,xxx,,,,,,,,,,,,,,,,x;"
        "%%---------------------%%%--------------%-----x,,,x,,,,,x,,,,,xxxxxxxxxxxxxxxxxx;"
        "%%%----%%---------------S%--------------------x,,,x,,,,,x,,,,,xxxxxxxxxxxxxxxxxx;"
        "%%%------------------%%-S%S------%------------x,,,xxxxxxxxxxxxxxxx,,,,,,,,,,,,xx;"
        "%%%%-----------------%--%%-%----%%------------x,,,,,,x,,,x,,,,,,xx,,,,,$,,,,,,xx;"
        "%%%--------------------%%S--------------------xx,xxx,x,,,x,,,,,,xxx,$,,,,,$,,,xx;"
        "%%------------------%-%%S---------------------+,,,,,,x,,,xxxxxx,,,4,,,,,,,,,E,xx;"
        "%%%-------------------S%%S-%------------------xx,xxx,x,,,x,,,,,,xxx,$,,,,,$,,,xx;"
        "%%-------------------%-S%%-%----%-------------x,,,x,,xxx,x,xxxxxxx,,,,,$,,,,,,xx;"
        "%%---------------------S%%S-------------------x,,,x,,,,,,x,,,,,,xx,,,,,,,,,,,,xx;"
        "%%%------------------%-%%---------------%-----x,xxxxxxxxxxxxxxx,xxxxxxxxxxxxx5xx;"
        "%%-----%---------------------------%----------x,x,,,,,,,,,,,,,,,xxxxxxxxxxxxx,xx;"
        "%%%---------%---------------------------------x,x,xxx,xxx,xxx,x,xxxx,,,,xxxxx,,x;"
        "%%--%-----------------------------------------x,x,xxx,xxx,xxx,x,xx,,,xx,,,xxxx,x;"
        "%%%--------------%----------------%---%-----%-x,x,xxx,xxx,xxx,x,x,,xxxxxx,,,,x,x;"
        "%%%%%%--%---%--%%%-%--%%%%%%%-%-%%%--%%--%-%%%x,,,,,,,,,,,,,,,x,,,xxxxxxxxxx,,,x;"
        "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx;";

    mk_templ(str, Map_templ_id::leng, vector<Translation>
    {
        {'@', {Feature_id::floor,  Actor_id::END,       Item_id::END, 1}}, //Start
        {'%', {Feature_id::wall,   Actor_id::END,       Item_id::END, 2}},
        {'x', {Feature_id::wall,   Actor_id::END,       Item_id::END, 3}},
        {',', {Feature_id::floor,  Actor_id::END,       Item_id::END, 3}},
        {'E', {Feature_id::floor,  Actor_id::leng_elder, Item_id::END, 3}},
        {'4', {Feature_id::floor,  Actor_id::END,       Item_id::END, 4}},
        {'5', {Feature_id::wall,   Actor_id::END,       Item_id::END, 5}},
        {'$', {Feature_id::altar}},
        {'-', {Feature_id::grass}},
        {'+', {Feature_id::floor,  Actor_id::END,       Item_id::END, 6}}, //Door
        {'S', {Feature_id::grass,  Actor_id::leng_spider}}
    });

    //----------------------------------------------------------------- RATS IN THE WALLS
    str =
        "################################################################################;"
        "##@#################,##,##xxxxxxxxx###xxxxxxxxxxx######rr#,##########,#,########;"
        "##.##############,,,,,,,,,x,,,,,,,xrrrxrr,rrr,rrxr,rrrr,,,,,,##,,#,,,,,,,#######;"
        "##...&##########,,,,xxxxxxx,,,,,,,xr,rxr,,,,,,,rxrrr,,,,,:,,,,,,,,,,,:,,,,######;"
        "###..:#########,,:,,x,,,,,,,,,,,,,,,,rrrrxx,xxrrxr,r,,,,,,,,,,,,,,,,,,,,,,,,,###;"
        "###:...#######,,,,,,xx,xxxx,,,,,,,xrrrxrrx,,,xrrxrrr,,,,,,,,:,,,,,,:,,,,,,######;"
        "##&..:..#####,,,,,,,,,,,,,xxxx,xxxx,r,xxxx,,,x,xx,,,,,,,,,,,,,,,,,,,,,,,,,######;"
        "####.&.:####,,,,,,,,,,,,,,,:,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,:,,,,,####;"
        "####..##&:3.,,,,,,,,,:,,,,,,,,,,:,,,,,,,,,,,1,,,:,,,xxx,xx,xxx,,,,,,,,,,,,,#####;"
        "#####..&:.#,,,,,,,,,,,,,,,x,x,x,x,x,x,,,,1,,,,,1,,,rx,,,,,,,,x,,,,:,,,,,,,######;"
        "###########,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,x,rrrrr,,x,,,,,,,,,,,#######;"
        "###########,,,,,,,,1,,,,,,,,,,,,,,,,:,,,1,,,,,,,1,,rr,,,,,r,,,,,,,#,,,,,########;"
        "###########,,,,,1,,,,,1,,,,,,,,,,,,,,,,,,,,,,,,,,,,,x,r,rr,,,x,,####,,,,,#######;"
        "###########,,,,,,,,,,,,,,,x,x,x,x,x,x,,,,1,,,,,1,,,rx,,rrr,,,x,,,,##############;"
        "############,,,1,,,,,,,1,,,,,,,,,,,,,,,,,,,,1,,,,,x,x,r,,r,,,x,,,###############;"
        "############,,,,,,,,,,,,,,,,,,,,:,,,,,,,,,,,,,,,,,rr,,,rr,,,,x,:##xxxxxx########;"
        "#############,,,1,,,,,1,,,xxx,xxxxx,xxx,,,:,,,,r,,,rx,rr,,,,,x,,,,x....x########;"
        "##############,,,,,1,,,,,,x,,,,,xrrrrrx,,,,,,,,,,,r,x,r,,,,,rx,,,,...>.x########;"
        "#################,,,,,,:,,x,,,,,xrr,,rxrr,rr,rr,r,,,xxxxxxxxxx,:,,x....x########;"
        "################:,,,,,,,,,x,,,,,xrrrrrxrrrr##rrr,r,,rr,rr,rr,r,##:xxxxxx########;"
        "###################:,,####xxxxxxxxxxxxx##r#####rr###r###r#rrr###################;"
        "################################################################################;";

    mk_templ(str, Map_templ_id::rats_in_the_walls, vector<Translation>
    {
        {'@', {Feature_id::floor,  Actor_id::END,     Item_id::END, 1}}, //Start
        {'x', {Feature_id::wall,   Actor_id::END,     Item_id::END, 2}}, //Constructed walls
        {'&', {Feature_id::bones,  Actor_id::END,     Item_id::END}},
        {'3', {Feature_id::floor,  Actor_id::END,     Item_id::END, 3}}, //Discovery event
        {',', {Feature_id::floor,  Actor_id::END,     Item_id::END, 4}}, //Random bones
        {'r', {Feature_id::floor,  Actor_id::rat,     Item_id::END, 4}}, //Random bones + rat
        {'>', {Feature_id::stairs}},
        {'1', {Feature_id::monolith}},
        {':', {Feature_id::stalagmite}}
    });

    //----------------------------------------------------------------- BOSS LEVEL
    str =
        "################################################################################;"
        "############################...................................................#;"
        "############################...................................................#;"
        "############################...#.....#...#.....#...#.....#...#.....#...........#;"
        "############################...###.###...###.###...###.###...###.###...........#;"
        "##############v..v#########......###.......###.......###.......###.............#;"
        "##############....#########....###.###...###.###...###.###...###.###...........#;"
        "############.#....#.#######....#.....#...#.....#...#.....#...#.....#...........#;"
        "#v.v########.#....#.#######.................................................####;"
        "#...#v.................v#v#.......|....|....|....|....|....|....|...|...|...####;"
        "#.#.#.............................................................v...v...v..###;"
        "#@..........................................................................M###;"
        "#.#.#.............................................................v...v...v..###;"
        "#...#v.................v#v#.......|....|....|....|....|....|....|...|...|...####;"
        "#v.v########.#....#.#######.................................................####;"
        "############.#....#.#######....#.....#...#.....#...#.....#...#.....#...........#;"
        "##############....#########....###.###...###.###...###.###...###.###...........#;"
        "##############v..v#########......###.......###.......###.......###.............#;"
        "############################...###.###...###.###...###.###...###.###...........#;"
        "############################...#.....#...#.....#...##...##...#.....#...........#;"
        "############################...................................................#;"
        "################################################################################;";

    mk_templ(str, Map_templ_id::boss_level, vector<Translation>
    {
        {'@', {Feature_id::floor,    Actor_id::END,           Item_id::END, 1}}, //Start
        {'M', {Feature_id::floor,    Actor_id::the_high_priest, Item_id::END}},    //Boss
        {'|', {Feature_id::pillar,   Actor_id::END,           Item_id::END}},
        {'v', {Feature_id::brazier,  Actor_id::END,           Item_id::END}},
        {'>', {Feature_id::stairs,   Actor_id::END,           Item_id::END}}
    });

    //----------------------------------------------------------------- BOSS LEVEL
    str =
        "################################################################################;"
        "#####################################...|...####################################;"
        "#####################################.|...|.####################################;"
        "#####################################...|...####################################;"
        "####################################..|...|..###################################;"
        "###################################.....|.....##################################;"
        "##################################....|...|....#################################;"
        "#################################...#.......#...################################;"
        "#######.......................##..#.#.##.##.#.#..##.......######################;"
        "#######.|.|.|.|.|.|.|.|.|.|.|.#v..|.v.|...|.v.|..v#.|.|.|.######################;"
        "#######..@..............................o.................######################;"
        "#######.|.|.|.|.|.|.|.|.|.|.|.#v..|.v.|...|.v.|..v#.|.|.|.######################;"
        "#######.......................##..#.#.##.##.#.#..##.......######################;"
        "#################################...#.......#...################################;"
        "##################################....|...|....#################################;"
        "###################################.....|.....##################################;"
        "####################################..|...|..###################################;"
        "#####################################...|...####################################;"
        "#####################################.|...|.####################################;"
        "#####################################...|...####################################;"
        "################################################################################;"
        "################################################################################;";

    mk_templ(str, Map_templ_id::trapezohedron_level, vector<Translation>
    {
        {'@', {Feature_id::floor,    Actor_id::END,         Item_id::END, 1}}, //Start
        {'|', {Feature_id::pillar,   Actor_id::END,         Item_id::END}},
        {'v', {Feature_id::brazier,  Actor_id::END,         Item_id::END}},
        {'o', {Feature_id::floor,    Actor_id::END,         Item_id::trapezohedron}}
    });
}

} //namespace

void init()
{
    init_templs();
}

const Map_templ& get_templ(const Map_templ_id id)
{
    assert(id != Map_templ_id::END);
    return templates_[int(id)];
}

} //Map_templ_handling
