
// Originally created by Antal1987, modified for use by TCC in C code injection system for C3X
// See: https://github.com/Antal1987/C3CPatchFramework/blob/master/Info/Civ3Conquests.h

typedef struct IntList IntList;
typedef struct City_Base_vtable City_Base_vtable;
typedef struct Citizen Citizen;
typedef struct RulerTitle RulerTitle;
typedef struct UnitType UnitType;
typedef struct UnitItem UnitItem;
typedef struct Tile_vtable Tile_vtable;
typedef struct RaceEraAnimations RaceEraAnimations;
typedef struct String32 String32;
typedef struct String24 String24;
typedef struct String16 String16;
typedef struct Race_vtable Race_vtable;
typedef struct Citizens Citizens;
typedef struct Citizen_Info Citizen_Info;
typedef struct Citizen_Body Citizen_Body;
typedef struct Advance Advance;
typedef struct JGL_Graphics_vtable JGL_Graphics_vtable;
typedef struct CityItem CityItem;
typedef struct Citizen_Base Citizen_Base;
typedef struct Tile_Base_vtable Tile_Base_vtable;
typedef struct Citizen_Base_vtable Citizen_Base_vtable;
typedef struct Base Base;
typedef struct Continent_Body Continent_Body;
typedef struct Buildings_Info Buildings_Info;
typedef struct Population Population;
typedef struct City_Improvements City_Improvements;
typedef struct Cities Cities;
typedef struct Units Units;
typedef struct Main_Window Main_Window;
typedef struct Map_Body Map_Body;
typedef struct Map_vtable Map_vtable;
typedef struct Base_List_Item Base_List_Item;
typedef struct Base_List Base_List;
typedef struct TileUnits TileUnits;
typedef struct Game Game;
typedef struct Unknown_3 Unknown_3;
typedef struct Resource_Type Resource_Type;
typedef struct String56 String56;
typedef struct String64 String64;
typedef struct Worker_Job Worker_Job;
typedef struct World_Size World_Size;
typedef struct Cultural_Levels Cultural_Levels;
typedef struct General General;
typedef struct Fighter Fighter;
typedef struct Unknown_6 Unknown_6;
typedef struct Tile_Type_Image Tile_Type_Image;
typedef struct Improvement Improvement;
typedef struct Citizen_Type Citizen_Type;
typedef struct Map_Renderer_vtable Map_Renderer_vtable;
typedef struct String260 String260;
typedef struct Tile_Image_Info Tile_Image_Info;
typedef struct Tile_Image_Info_vtable Tile_Image_Info_vtable;
typedef struct Tile_Image_Info_List Tile_Image_Info_List;
typedef struct Base_vtable Base_vtable;
typedef struct City_Screen_FileNames City_Screen_FileNames;
typedef struct _188_48 _188_48;
typedef struct FLC_Frame_Image FLC_Frame_Image;
typedef struct _188_4C _188_4C;
typedef struct Civilopedia_Article Civilopedia_Article;
typedef struct JGL_Renderer JGL_Renderer;
typedef struct Buildings_Info_Item Buildings_Info_Item;
typedef struct Date Date;
typedef struct Sound_Info Sound_Info;
typedef struct Sound_Info_Array Sound_Info_Array;
typedef struct Citizen_Mood_Array Citizen_Mood_Array;
typedef struct Map_Body_vtable Map_Body_vtable;
typedef struct Unit_vtable Unit_vtable;
typedef struct _188_vtable _188_vtable;
typedef struct Advisor_Renderer Advisor_Renderer;
typedef struct Trade_Net Trade_Net;
typedef struct Map_Worker_Job_Info Map_Worker_Job_Info;
typedef struct Airfield_Body Airfield_Body;
typedef struct Colony_Body Colony_Body;
typedef struct Radar_Tower_Body Radar_Tower_Body;
typedef struct Outpost_Body Outpost_Body;
typedef struct Victory_Point_Body Victory_Point_Body;
typedef struct Victory_Point Victory_Point;
typedef struct Tile_Building_Body Tile_Building_Body;
typedef struct Tile_Building Tile_Building;
typedef struct PCX_Color_Table PCX_Color_Table;
typedef struct Tile_Type_Flags Tile_Type_Flags;
typedef struct String264 String264;
typedef struct String256 String256;
typedef struct City_Icon_Images City_Icon_Images;
typedef struct Map_Cursor_Images Map_Cursor_Images;
typedef struct Old_Interface_Images Old_Interface_Images;
typedef struct Main_Screen_Form_vtable Main_Screen_Form_vtable;
typedef struct GUI_Data_30 GUI_Data_30;
typedef struct Main_Screen_Data_170 Main_Screen_Data_170;
typedef struct Form_Data_30 Form_Data_30;
typedef struct GUI_Form_1_vtable GUI_Form_1_vtable;
typedef struct Base_Form_vtable Base_Form_vtable;
typedef struct Control_Data_Offsets Control_Data_Offsets;
typedef struct Context_Menu_Item Context_Menu_Item;
typedef struct Civilopedia_Article_vtable Civilopedia_Article_vtable;
typedef struct Palace_View_Info Palace_View_Info;
typedef struct Text_Reader Text_Reader;
typedef struct Button_Images_3 Button_Images_3;
typedef struct Point Point;
typedef struct Game_Version Game_Version;
typedef struct History History;
typedef struct History_Item History_Item;
typedef struct Combat_Experience Combat_Experience;
typedef struct _12C _12C;
typedef struct Leader_vtable Leader_vtable;
typedef struct Flic_Anim_Info Flic_Anim_Info;
typedef struct Game_Limits Game_Limits;
typedef struct Default_Game_Limits Default_Game_Limits;
typedef struct Campain_Records_Labels Campain_Records_Labels;
typedef struct City_Form_Labels City_Form_Labels;
typedef struct Unit_Move_Target Unit_Move_Target;
typedef struct Hash_Table Hash_Table;
typedef struct Hash_Table_vtable Hash_Table_vtable;
typedef struct Hash_Table_Item Hash_Table_Item;
typedef struct Leader_Data_10 Leader_Data_10;
typedef struct Item_List2 Item_List2;
typedef struct Culture Culture;
typedef struct Espionage Espionage;
typedef struct CTPG CTPG;
typedef struct Default_Game_Settings Default_Game_Settings;
typedef struct ComboBox_Text_Control ComboBox_Text_Control;
typedef struct Replay_Turn Replay_Turn;
typedef struct Replay_Turn_Node Replay_Turn_Node;
typedef struct Replay_Entry Replay_Entry;
typedef struct Replays_Body Replays_Body;
typedef struct Replay_Entry_Node Replay_Entry_Node;
typedef struct World_Features World_Features;
typedef struct Starting_Location Starting_Location;
typedef struct Scenario_Player Scenario_Player;
typedef struct Navigator_Cell Navigator_Cell;
typedef struct Navigator_Point Navigator_Point;
typedef struct JGL_Image JGL_Image;
typedef struct JGL_Image_vtable JGL_Image_vtable;
typedef struct JGL_Graphics JGL_Graphics;
typedef struct Window_Controller Window_Controller;
typedef struct Window_Controller_vtable Window_Controller_vtable;
typedef struct JGL_Image_Info JGL_Image_Info;
typedef struct JGL_Color_Table_vtable JGL_Color_Table_vtable;
typedef struct JGL_Color_Table JGL_Color_Table;
typedef struct JGL_Image_Info_vtable JGL_Image_Info_vtable;
typedef struct Units_Image_Data Units_Image_Data;
typedef struct Animation_Info Animation_Info;
typedef struct Animation_Info_vtable Animation_Info_vtable;
typedef struct Animation_Data_60 Animation_Data_60;
typedef struct Animation_Data_60_vtable Animation_Data_60_vtable;
typedef struct IDLS IDLS;
typedef struct FLIC_HEADER FLIC_HEADER;
typedef struct FLIC_CHUNK_HEADER FLIC_CHUNK_HEADER;
typedef struct FLIC_FRAME_HEADER FLIC_FRAME_HEADER;
typedef struct FLIC_COLOR_256_HEADER FLIC_COLOR_256_HEADER;
typedef union FLIC_CHUNK FLIC_CHUNK;
typedef struct FLC_Direction_Start_Frames FLC_Direction_Start_Frames;
typedef struct Animation_Node Animation_Node;
typedef struct Era_Type Era_Type;
typedef struct Civ_Treaties Civ_Treaties;
typedef struct Civ_Treaty Civ_Treaty;
typedef struct Difficulty_Level Difficulty_Level;
typedef struct Control_Tooltips Control_Tooltips;
typedef struct Control_Tooltip Control_Tooltip;
typedef struct City_Order City_Order;
typedef struct Trade_Net_Distance_Info Trade_Net_Distance_Info;
typedef struct City_Form_vtable City_Form_vtable;
typedef struct Reputation Reputation;
typedef struct Leader Leader;
typedef struct Government Government;
typedef struct Tile_Body Tile_Body;
typedef struct Race Race;
typedef struct City_Body City_Body;
typedef struct Continent Continent;
typedef struct Map_Renderer Map_Renderer;
typedef struct Tile_Type Tile_Type;
typedef struct PCX_Image PCX_Image;
typedef struct FLC_Animation FLC_Animation;
typedef struct Airfield Airfield;
typedef struct Colony Colony;
typedef struct Radar_Tower Radar_Tower;
typedef struct Outpost Outpost;
typedef struct Advisor_Hints Advisor_Hints;
typedef struct Advisor_Form_vtable Advisor_Form_vtable;
typedef struct Base_Form_Data Base_Form_Data;
typedef struct Button Button;
typedef struct Advisor_Science_Form Advisor_Science_Form;
typedef struct Advisor_Base_Form Advisor_Base_Form;
typedef struct Main_Screen_Data_1AD4 Main_Screen_Data_1AD4;
typedef struct Command_Button Command_Button;
typedef struct GUI_Form_1 GUI_Form_1;
typedef struct ComboBox ComboBox;
typedef struct Navigator_Data Navigator_Data;
typedef struct PCX_Animation PCX_Animation;
typedef struct ComboBox_2 ComboBox_2;
typedef struct ComboBox1 ComboBox1;
typedef struct Replays Replays;
typedef struct City City;
typedef struct Unit_Body Unit_Body;
typedef struct Tile Tile;
typedef struct Map Map;
typedef struct BIC BIC;
typedef struct _1A4 _1A4;
typedef struct Scroll_Bar Scroll_Bar;
typedef struct Base_Form Base_Form;
typedef struct Advisor_Military_Form Advisor_Military_Form;
typedef struct Advisor_Trade_Form Advisor_Trade_Form;
typedef struct Advisor_Internal_Form Advisor_Internal_Form;
typedef struct Advisor_GUI Advisor_GUI;
typedef struct City_View_Form City_View_Form;
typedef struct Context_Menu Context_Menu;
typedef struct Base_List_Control Base_List_Control;
typedef struct Base_List_Box Base_List_Box;
typedef struct CheckBox_Control CheckBox_Control;
typedef struct CheckBox CheckBox;
typedef struct Parameters_Form Parameters_Form;
typedef struct Common_Dialog_Form Common_Dialog_Form;
typedef struct Civilopedia_Form Civilopedia_Form;
typedef struct TextBox TextBox;
typedef struct File_Dialog_Body File_Dialog_Body;
typedef struct Main_Menu_Form Main_Menu_Form;
typedef struct Espionage_Form Espionage_Form;
typedef struct Palace_View_Form Palace_View_Form;
typedef struct Spaceship_Form Spaceship_Form;
typedef struct Demographics_Form Demographics_Form;
typedef struct Victory_Conditions_Form Victory_Conditions_Form;
typedef struct Wonders_Form Wonders_Form;
typedef struct MP_LAN_Game_Parameters_Form MP_LAN_Game_Parameters_Form;
typedef struct MP_Internet_Game_Parameters_Form MP_Internet_Game_Parameters_Form;
typedef struct MP_Manager_Form MP_Manager_Form;
typedef struct Wonder_Form Wonder_Form;
typedef struct New_Game_Map_Form New_Game_Map_Form;
typedef struct New_Game_Player_Form New_Game_Player_Form;
typedef struct Campain_Records_Form Campain_Records_Form;
typedef struct Load_Scenario_Form Load_Scenario_Form;
typedef struct Authors_Form Authors_Form;
typedef struct MP_Filters_Form MP_Filters_Form;
typedef struct Radio_Button_List Radio_Button_List;
typedef struct Game_Results_Form Game_Results_Form;
typedef struct Game_Spy_Connection_Form Game_Spy_Connection_Form;
typedef struct City_Form City_Form;
typedef struct Unit Unit;
typedef struct Tile_Info_Form Tile_Info_Form;
typedef struct Main_GUI Main_GUI;
typedef struct Advisor_Culture_Form Advisor_Culture_Form;
typedef struct Advisor_Foreign_Form Advisor_Foreign_Form;
typedef struct Main_Screen_Form Main_Screen_Form;
typedef struct Governor_Form Governor_Form;
typedef struct File_Dialog_Form File_Dialog_Form;
typedef struct New_Era_Form New_Era_Form;

// Used to simulated thiscall convention with fastcall by inserting a dummy unused second parameter to occupy edx.
typedef int __;

enum SquareTypes
{
  SQ_Desert = 0x0,
  SQ_Plains = 0x1,
  SQ_Grassland = 0x2,
  SQ_Tundra = 0x3,
  SQ_FloodPlain = 0x4,
  SQ_Hills = 0x5,
  SQ_Mountains = 0x6,
  SQ_Forest = 0x7,
  SQ_Jungle = 0x8,
  SQ_Swamp = 0x9,
  SQ_Volcano = 0xA,
  SQ_Coast = 0xB,
  SQ_Sea = 0xC,
  SQ_Ocean = 0xD,
};

enum BICDataCodes
{
  BIC_Header = 0x23524556,
  BIC_StartingLocations = 0x434F4C53,
  BIC_Leaders = 0x4441454C,
  BIC_Resources = 0x444F4F47,
  BIC_Races = 0x45434152,
  BIC_Tiles = 0x454C4954,
  BIC_GeneralSettings = 0x454C5552,
  BIC_Game = 0x454D4147,
  BIC_DifficultyLevels = 0x46464944,
  BIC_Improvements = 0x47444C42,
  BIC_Advances = 0x48434554,
  BIC_WorkerJobs = 0x4D524654,
  BIC_Diplomats = 0x4E505345,
  BIC_Citizens = 0x4E5A5443,
  BIC_UnitTypes = 0x4F545250,
  BIC_WorldMap = 0x50414D57,
  BIC_WorldCharacteristics = 0x52484357,
  BIC_CombatExperience = 0x52505845,
  BIC_TileTypes = 0x52524554,
  BIC_Eras = 0x53415245,
  BIC_Units = 0x54494E55,
  BIC_Culture = 0x544C5543,
  BIC_Continents = 0x544E4F43,
  BIC_Governments = 0x54564F47,
  BIC_Flavours = 0x56414C46,
  BIC_Colonies = 0x594E4C43,
  BIC_Cities = 0x59544943,
  BIC_WorldSizes = 0x5A495357,
};

enum Resource_Type_Classes
{
  RC_Bonus = 0x0,
  RC_Luxury = 0x1,
  RC_Strategic = 0x2,
};

enum City_Icons_Small
{
  CIS_Icon00 = 0x0,
  CIS_Science_Large = 0x1,
  CIS_Money_Income = 0x2,
  CIS_Money_Outcome = 0x3,
  CIS_Production_Income = 0x4,
  CIS_Production_Outcome = 0x5,
  CIS_Food_Income = 0x6,
  CIS_Food_Outcome = 0x7,
  CIS_Production_Cell = 0x8,
  CIS_Food_Cell = 0x9,
  CIS_Food_Minus = 0xA,
  CIS_Pollution = 0xB,
  CIS_Luxury_Small = 0xC,
  CIS_Production_Small = 0xD,
  CIS_Money_Small = 0xE,
  CIS_Food_Small = 0xF,
  CIS_Science_Small = 0x10,
  CIS_Corruption = 0x11,
  CIS_Culture = 0x12,
  CIS_Happy_Face = 0x13,
  CIS_Production_Small_2 = 0x14,
  CIS_Money_Small_2 = 0x15,
  CIS_Food_Small_2 = 0x16,
  CIS_Treasury_1 = 0x17,
  CIS_Treasury_2 = 0x18,
};

enum AnimatedEffect
{
  AE_Disorder = 0x1,
  AE_Fireworks = 0x2,
  AE_Hit = 0x3,
  AE_Hit2 = 0x4,
  AE_Hit3 = 0x5,
  AE_Hit5 = 0x6,
  AE_Miss = 0x7,
  AE_WaterMiss = 0x8,
  AE_Smolder = 0x9,
  AE_Eruption = 0xA,
  AE_Plague = 0xB,
};

enum ImprovementTypeFlags
{
  ITF_Center_of_Empire			    = 0x1,
  ITF_Veteran_Ground_Units		    = 0x2,
  ITF_50_Research_Output		    = 0x4,
  ITF_50_Luxury_Output			    = 0x8,
  ITF_50_Tax_Output			    = 0x10,
  ITF_Removes_Population_Pollution	    = 0x20,
  ITF_Reduces_Buildings_Pollution	    = 0x40,
  ITF_Resistant_To_Bribery		    = 0x80,
  ITF_Reduces_Corruption		    = 0x100,
  ITF_Doubles_City_Growth_Rate		    = 0x200,
  ITF_Increases_Luxury_Trade		    = 0x400,
  ITF_Allows_City_Level_2		    = 0x800,
  ITF_Allows_City_Level_3		    = 0x1000,
  ITF_Replaces_Other_Buildings		    = 0x2000,
  ITF_Must_Be_Near_Water		    = 0x4000,
  ITF_Must_Be_Near_River		    = 0x8000,
  ITF_Can_Explode_Or_Meltdown		    = 0x10000,
  ITF_Veteran_Sea_Units			    = 0x20000,
  ITF_Veteran_Air_Units			    = 0x40000,
  ITF_Capitalization			    = 0x80000,
  ITF_Allows_Water_Trade		    = 0x100000,
  ITF_Allows_Air_Trade			    = 0x200000,
  ITF_Reduces_War_Weariness		    = 0x400000,
  ITF_Increases_Shields_In_Water	    = 0x800000,
  ITF_Increases_Food_In_Water		    = 0x1000000,
  ITF_Increases_Trade_In_Water		    = 0x2000000,
  ITF_Vulnerable_To_Charm_Bombard	    = 0x4000000,
  ITF_8000000				    = 0x8000000,
  ITF_10000000				    = 0x10000000,
  ITF_20000000				    = 0x20000000,
  ITF_Produces_Unit			    = 0x40000000,
  ITF_Required_Goods_Must_Be_In_City_Radius = 0x80000000,
};

enum Worker_Jobs
{
  WJ_Build_Mines = 0x0,
  WJ_Irrigate = 0x1,
  WJ_Build_Fortress = 0x2,
  WJ_Build_Road = 0x3,
  WJ_Build_Railroad = 0x4,
  WJ_Plant_Forest = 0x5,
  WJ_Clean_Forest = 0x6,
  WJ_Clear_Swamp = 0x7,
  WJ_Clean_Pollution = 0x8,
  WJ_Build_Airfield = 0x9,
  WJ_Build_Radar = 0xA,
  WJ_Build_Outpost = 0xB,
  WJ_Build_Barricade = 0xC,
};

enum ImprovementTypeCharacteristics
{
  ITC_Coastal_Installation = 0x1,
  ITC_Militaristic = 0x2,
  ITC_Wonder = 0x4,
  ITC_Small_Wonder = 0x8,
  ITC_Continental_Mood_Effects = 0x10,
  ITC_Scientific = 0x20,
  ITC_Commercial = 0x40,
  ITC_Expansionistic = 0x80,
  ITC_Religious = 0x100,
  ITC_Industrious = 0x200,
  ITC_Agricultural = 0x400,
  ITC_Seafaring = 0x800,
};

enum ImprovementTypeFlags_Byte1
{
  ITF_B1_Reduces_Corruption = 0x1,
  ITF_B1_Doubles_City_Growth_Rate = 0x2,
  ITF_B1_Increases_Luxury_Trade = 0x4,
  ITF_B1_Allows_City_Level_2 = 0x8,
  ITF_B1_Allows_City_Level_3 = 0x10,
  ITF_B1_Replaces_Other_Buildings = 0x20,
  ITF_B1_Must_Be_Near_Water = 0x40,
  ITF_B1_Must_Be_Near_River = 0x80,
};

enum ImprovementTypeWonderFeatures
{
  ITW_Safe_Sea_Travel			     = 0x1,
  ITW_Gain_Any_Advance_Owned_by_2_Civ	     = 0x2,
  ITW_Double_Combat_Strength_vs_Barbarians   = 0x4,
  ITW_Ship_Movement_Inc_1		     = 0x8,
  ITW_Doubles_Research_Output		     = 0x10,
  ITW_Trade_In_Each_Tile_inc_1		     = 0x20,
  ITW_Halves_Unit_Upgrade_Cost		     = 0x40,
  ITW_Pays_Maintenance_For_Trade_Inst	     = 0x80,
  ITW_Allows_Construction_Of_Nuclear_Devices = 0x100,
  ITW_City_Growth_Inc_2_Citizens	     = 0x200,
  ITW_Free_Advance_Inc_2		     = 0x400,
  ITW_Reduces_War_Weariness		     = 0x800,
  ITW_Unk1				     = 0x1000,
  ITW_Allows_Diplomatic_Victory		     = 0x2000,
  ITW_Unk2				     = 0x4000,
  ITW_Unk3				     = 0x8000,
  ITW_Increases_Army_Value		     = 0x10000,
  ITW_Tourist_Attraction		     = 0x20000,
};

enum ImprovementTypeSmallWonderFeatures
{
  ITSW_Increases_Chance_Of_Leader_Appearance = 0x1,
  ITSW_Build_Army_Without_Leader	     = 0x2,
  ITSW_Larger_Armies			     = 0x4,
  ITSW_Treasury_Earns_5_Percent		     = 0x8,
  ITSW_Build_Spaceship_Parts		     = 0x10,
  ITSW_Reduces_Corruption		     = 0x20,
  ITSW_Decreases_Success_Of_Missile_Attacks  = 0x40,
  ITSW_Allows_Spy_Missions		     = 0x80,
  ITSW_Allows_Healing_In_Enemy_Territory     = 0x100,
  ITSW_200				     = 0x200,
  ITSW_Requires_Victorous_Army		     = 0x400,
};

enum CorruptionAndWasteTypes
{
  CWT_Minimal = 0x0,
  CWT_Nuisance = 0x1,
  CWT_Problematic = 0x2,
  CWT_Rampant = 0x3,
  CWT_Catastrophic = 0x4,
  CWT_Communal = 0x5,
  CWT_Off = 0x6,
};

enum AdvanceTypeFlags
{
  ATF_Enables_Diplomats = 0x1,
  ATF_Enables_Irrigation_Without_Fresh_Water = 0x2,
  ATF_Enables_Bridges = 0x4,
  ATF_Disabled_Deseases_From_Flood_Plains = 0x8,
  ATF_Enables_Conscription_Of_Units = 0x10,
  ATF_Enables_Mobilizations_Levels = 0x20,
  ATF_Enables_Recycling = 0x40,
  ATF_Enables_Precision_Bombing = 0x80,
  ATF_Enables_Mutual_Protection_Pacts = 0x100,
  ATF_Enables_Right_Of_Passage_Treaties = 0x200,
  ATF_Enables_Military_Alliances = 0x400,
  ATF_Enables_Trade_Embargos = 0x800,
  ATF_Doubles_Effect_Of_Wealth = 0x1000,
  ATF_Enables_Trade_Over_Sea_Tiles = 0x2000,
  ATF_Enables_Trade_Over_Ocean_Tiles = 0x4000,
  ATF_Enables_Map_Trading = 0x8000,
  ATF_Enables_Communication_Trading = 0x10000,
  ATF_Not_Required_For_Era_Advancement = 0x20000,
  ATF_Doubles_Work_Rate_Of_Workers = 0x40000,
  ATF_Cannot_Be_Traded = 0x80000,
  ATF_Permits_Sacrifice = 0x100000,
  ATF_Bonus_Tech = 0x200000,
  ATF_Reveal_Map = 0x400000,
};

enum Tile_Resource_Types
{
  TRT_Food = 0x0,
  TRT_Shields = 0x1,
  TRT_Trade = 0x2,
};

enum CityStatusFlags
{
  CSF_Civil_Disorder = 0x1,
  CSF_Airlift_Used = 0x4,
  CSF_Capitalization = 0x20,
};

enum UnitTypeAbilitiesFlags
{
  UTAF_Wheeled = 0x1,
  UTAF_Foot_Unit = 0x2,
};

enum UnitTypeAIFlags
{
  UTAI_Offence = 0x1,
  UTAI_Defence = 0x2,
  UTAI_Artillery = 0x4,
  UTAI_Explore = 0x8,
  UTAI_Army = 0x10,
  UTAI_Cruise_Missile = 0x20,
  UTAI_Air_Bombard = 0x40,
  UTAI_Air_Defence = 0x80,
  UTAI_Naval_Power = 0x100,
  UTAI_Air_Transport = 0x200,
  UTAI_Naval_Transport = 0x400,
  UTAI_Naval_Carrier = 0x800,
  UTAI_Terraform = 0x1000,
  UTAI_Settle = 0x2000,
  UTAI_Leader = 0x4000,
  UTAI_Tactical_Nuke = 0x8000,
  UTAI_ICBM = 0x10000,
  UTAI_Naval_Missile_Transport = 0x20000,
  UTAI_Flag_Unit = 0x40000,
  UTAI_King = 0x80000,
};

enum UnitTypeAbilities
{
  UTA_Wheeled = 0x0,
  UTA_Foot_Unit = 0x1,
  UTA_Blitz = 0x2,
  UTA_Cruise_Missile = 0x3,
  UTA_All_Terrains_As_Roads = 0x4,
  UTA_Radar = 0x5,
  UTA_Amphibious = 0x6,
  UTA_Invisible = 0x7,
  UTA_Transports_Only_Aircraft = 0x8,
  UTA_Draft = 0x9,
  UTA_Immobile = 0xA,
  UTA_Sinks_In_Sea = 0xB,
  UTA_Sinks_In_Ocean = 0xC,
  UTA_Flag_unit = 0xD,
  UTA_Transports_Only_Foot_Unit = 0xE,
  UTA_Starts_Golden_Age = 0xF,
  UTA_Nuclear_Weapon = 0x10,
  UTA_Hidden_Nationality = 0x11,
  UTA_Army = 0x12,
  UTA_Leader = 0x13,
  UTA_Infinite_Bombard_Range = 0x14,
  UTA_Stealth = 0x15,
  UTA_Detect_Invisible = 0x16,
  UTA_Tacticle_Missile = 0x17,
  UTA_Transports_Only_Tacticle_Missiles = 0x18,
  UTA_Ranged_Attack_Animation = 0x19,
  UTA_Rotate_Before_Attack = 0x1A,
  UTA_Lethal_Land_Bombardment = 0x1B,
  UTA_Lethal_Sea_Bombardment = 0x1C,
  UTA_King = 0x1D,
  UTA_Requires_Escort = 0x1E,
};

enum RaceBonusFlags
{
  RBF_Militaristic = 0x1,
  RBF_Commercial = 0x2,
  RBF_Expansionist = 0x4,
  RBF_Scientific = 0x8,
  RBF_Religious = 0x10,
  RBF_Industrious = 0x20,
  RBF_Agricultural = 0x40,
  RBF_Seafaring = 0x80,
};

enum RaceBonuses
{
  RB_Militaristic = 0x0,
  RB_Commercial = 0x1,
  RB_Expansionist = 0x2,
  RB_Scientific = 0x3,
  RB_Religious = 0x4,
  RB_Industrious = 0x5,
  RB_Agricultural = 0x6,
  RB_Seafaring = 0x7,
};

enum UnitTypeClasses
{
  UTC_Land = 0x0,
  UTC_Sea = 0x1,
  UTC_Air = 0x2,
};

enum City_Form_Buttons
{
  CFB_Orders = 542,
  CFB_CityView = 543,
  CFB_Exit = 545,
  CFB_Next = 546,
  CFB_Prev = 547,
  CFB_Hurry = 548,
  CFB_Draft = 549,
  CFB_Governor = 550,
  CFB_Resources_View = 551,
};

enum Civilopedia_Form_Buttons
{
  CPFB_Close = 0,
  CPFB_Right = 1,
  CPFB_Left = 2,
  CPFB_Up = 3,
  CPFB_Next = 4,
  CPFB_Prev = 5,
  CPFB_Description = 6,
};

enum Main_GUI_Buttons
{
  MGB_Unit_Command_0 = 0,
  MGB_Unit_Command_1 = 1,
  MGB_Unit_Command_2 = 2,
  // ...
  MGB_Unit_Command_40 = 40,
  MGB_Unit_Command_41 = 41,

  MGB_Geography = 42,
  MGB_Diplomacy = 43,
  MGB_Espionage = 44,
  MGB_Palace = 45,
  MGB_Victory = 46,
  MGB_Spaceship = 47,
  MGB_Task = 48,
  MGB_Menu = 49,
  MGB_Civilopedia = 50,
  MGB_Advisors = 51,
  MGB_Prev_City = 52,
  MGB_Navigate_City_Mode = 53,
  MGB_Next_City = 54,
  MGB_Prev_Unit = 55,
  MGB_Navigate_Unit_Mode = 56,
  MGB_Next_Unit = 57,
  MGB_Move_Unit_Group = 58,
  MGB_Move_Identical_Unit_Group = 59,
};

enum Culture_Groups
{
  CG_American = 0,
  CG_European = 1,
  CG_Roman = 2,
  CG_Mid_East = 3,
  CG_Asian = 4,
};

enum Citizen_Mood_Types
{
  CMT_Happy = 0,
  CMT_Content = 1,
  CMT_Unhappy = 2,
  CMT_Rebel = 3,
};

enum Unit_Command_Values
{
  // Standard Orders
  UCV_Skip_Turn = 0x1,
  UCV_Wait	= 0x2,
  UCV_Fortify	= 0x4,
  UCV_Disband	= 0x8,
  UCV_Go_To	= 0x10,
  UCV_Explore	= 0x20,
  UCV_Sentry	= 0x40,

  // Special Actions
  UCV_Load		  = 0x10000001,
  UCV_Unload		  = 0x10000002,
  UCV_Airlift		  = 0x10000004,
  UCV_Pillage		  = 0x10000008,
  UCV_Bombard		  = 0x10000010,
  UCV_Airdrop		  = 0x10000020,
  UCV_Build_Army	  = 0x10000040,
  UCV_Finish_Improvements = 0x10000080,
  UCV_Upgrade_Unit	  = 0x10000100,
  UCV_Rescue_Princess	  = 0x10000200,
  UCV_Stealth_Attack	  = 0x10001000,
  UCV_Enslave		  = 0x10004000,
  UCV_Unknown		  = 0x10008000,
  UCV_Charm_Bombard       = 0x10020000,
  UCV_Sacrifice		  = 0x10100000,
  UCV_Science_Age	  = 0x10200000,

  // Worker/Engineer Actions
  UCV_Build_Colony	= 0x20000001,
  UCV_Build_City	= 0x20000002,
  UCV_Build_Road	= 0x20000004,
  UCV_Build_Railroad	= 0x20000008,
  UCV_Build_Fortress	= 0x20000010,
  UCV_Build_Mine	= 0x20000020,
  UCV_Irrigate		= 0x20000040,
  UCV_Clear_Forest	= 0x20000080,
  UCV_Clear_Jungle	= 0x20000100,
  UCV_Plant_Forest	= 0x20000200,
  UCV_Clear_Pollution	= 0x20000400,
  UCV_Automate		= 0x20000800,
  UCV_Join_City		= 0x20001000,
  UCV_Build_Airfield	= 0x20002000,
  UCV_Build_Radar_Tower = 0x20004000,
  UCV_Build_Outpost	= 0x20008000,
  UCV_Build_Barricade	= 0x20010000,

  // Air Missions
  UCV_Bombing		= 0x30000001,
  UCV_Recon		= 0x30000002,
  UCV_Intercept		= 0x30000004,
  UCV_Rebase		= 0x30000008,
  UCV_Precision_Bombing = 0x30000010,

  UCV_Sentry_Wait	      = 0x40000001,
  UCV_Auto_Bombard	      = 0x40000002,
  UCV_Build_Remote_Colony     = 0x40000004,
  UCV_Build_Road_To_Tile      = 0x40000008,
  UCV_Build_Railroad_To_Tile  = 0x40000010,
  UCV_Auto_Build_Trade_Routes = 0x40000020,
  UCV_Auto_Irrigate	      = 0x40000040,
  UCV_Auto_Clear_Forest	      = 0x40000080,
  UCV_Auto_Clear_Swamp	      = 0x40000100,
  UCV_Auto_Clear_Pollution    = 0x40000200,
  UCV_Auto_City_Actions	      = 0x40000400,
  UCV_Auto_Save_Tiles	      = 0x40000800,
  UCV_Auto_Air_Bombard	      = 0x40001000,
  UCV_Unknown_40002000	      = 0x40002000,
  UCV_Auto_Save_City_Tiles    = 0x40004000,
  UCV_Go_To_City	      = 0x40008000,
  UCV_Rename		      = 0x40010000,

  UCV_Stack_Bombard = 0x80000001,
};

enum Unit_Mode_Actions
{
  UMA_Go_To = 0x1,
  UMA_Road_To_Tile = 0x2,
  UMA_Raiload_To_Tile = 0x3,
  UMA_Airdrop = 0x4,
  UMA_5 = 0x5,
  UMA_Bombard = 0x6,
  UMA_Air_Bombard = 0x7,
  UMA_Airlift = 0x8,
  UMA_Recon = 0x9,
  UMA_Rebase = 0xA,
  UMA_11 = 0xB,
  UMA_12 = 0xC,
  UMA_13 = 0xD,
  UMA_Build_Colony = 0xE,
  UMA_Auto_Bombard = 0xF,
  UMA_Auto_Air_Bombard = 0x10,
  UMA_17 = 0x11,
  UMA_18 = 0x12,
  UMA_19 = 0x13,
  UMA_20 = 0x14,
  UMA_21 = 0x15,
  UMA_22 = 0x16,
};

enum City_Order_Types
{
  COT_Improvement = 0x1,
  COT_Unit = 0x2,
};

enum Hurry_Production_Type
{
  HPT_Cannot_Hurry = 0x0,
  HPT_Forced_Labor = 0x1,
  HPT_Paid_Labor = 0x2,
};

enum ImprovementTypeWonderFeatures_Byte1
{
  ITW_B1_Allows_Construction_Of_Nuclear_Devices = 0x1,
  ITW_B1_City_Growth_Inc_2_Citizens = 0x2,
  ITW_B1_Free_Advance_Inc_2 = 0x4,
  ITW_B1_Reduces_War_Weariness = 0x8,
  ITW_B1_Unk1 = 0x10,
  ITW_B1_Allows_Diplomatic_Victory = 0x20,
  ITW_B1_Unk2 = 0x40,
  ITW_B1_Unk3 = 0x80,
  ITW_B1_Increases_Army_Value = 0x100,
  ITW_B1_Turist_Attraction = 0x200,
};

enum Tile_Owner_Types
{
  TOT_None = 0x0,
  TOT_Barbarians = 0x1,
  TOT_Civ = 0x2,
  TOT_Player = 0x3,
};

enum SquareType_Groups
{
  SQG_xtgc = 0x0,
  SQG_xpgc = 0x1,
  SQG_xdgc = 0x2,
  SQG_xdpc = 0x3,
  SQG_xdgp = 0x4,
  SQG_xggc = 0x5,
  SQG_wCSO = 0x6,
  SQG_wSSS = 0x7,
  SQG_wOOO = 0x8,
};

enum FLIC_Chuck_Types
{
  FLCT_CEL_DATA = 0x3,
  FLCT_COLOR_256 = 0x4,
  FLCT_DELTA_FLC = 0x7,
  FLCT_COLOR_64 = 0xB,
  FLCT_DELTA_FLI = 0xC,
  FLCT_BLACK = 0xD,
  FLCT_BYTE_RUN = 0xF,
  FLCT_FLI_COPY = 0x10,
  FLCT_PSTAMP = 0x12,
  FLCT_DTA_BRUN = 0x19,
  FLCT_DTA_COPY = 0x1A,
  FLCT_DTA_LC = 0x1B,
  FLCT_LABEL = 0x1F,
  FLCT_BMP_MASK = 0x20,
  FLCT_MLEV_MASK = 0x21,
  FLCT_SEGMENT = 0x22,
  FLCT_KEY_IMAGE = 0x23,
  FLCT_KEY_PAL = 0x24,
  FLCT_REGION = 0x25,
  FLCT_WAVE = 0x26,
  FLCT_USERSTRING = 0x27,
  FLCT_RGN_MASK = 0x28,
  FLCT_LABELEX = 0x29,
  FLCT_SHIFT = 0x2A,
  FLCT_PATHMAP = 0x2B,
  FLCT_PREFIX_TYPE = 0xF100,
  FLCT_SCRIPT_CHUNK = 0xF1E0,
  FLCT_FRAME_TYPE_Low_10 = 0xF1EA,
  FLCT_FRAME_TYPE = 0xF1FA,
  FLCT_SEGMENT_TABLE = 0xF1FB,
  FLCT_HUFFMAN_TABLE = 0xF1FC,
};

enum Civ_Contact_Type
{
  CCT_Have_Military_Map = 0x40,
};

enum Game_Render_Flags
{
  GRF_Show_Enemy_Units = 0x8,
};

enum Goody_Hut_Message_Type
{
  GHMT_Money = 0x0,
  GHMT_Maps = 0x1,
  GHMT_City = 0x2,
  GHMT_Nothing = 0x3,
  GHMT_Settlers = 0x4,
  GHMT_Mercenaries = 0x5,
  GHMT_Tech = 0x6,
  GHMT_Barbarians = 0x7,
};

enum UnitStateType
{
  UnitState_Fortifying = 0x1,
  UnitState_Build_Mines = 0x2,
  UnitState_Irrigate = 0x3,
  UnitState_Build_Forest = 0x4,
  UnitState_Build_Road = 0x5,
  UnitState_Build_Railroad = 0x6,
  UnitState_Plant_Forest = 0x7,
  UnitState_Clean_Forest = 0x8,
  UnitState_Clear_Pollution = 0xA,
  UnitState_Build_Airfield = 0xB,
  UnitState_Build_Radar_Tower = 0xC,
  UnitState_Build_Outpost = 0xD,
  UnitState_Build_Barricade = 0xE,
  UnitState_Intercept = 0xF,
  UnitState_Go_To = 0x10,
  UnitState_Road_To_Tile = 0x11,
  UnitState_Railroad_To_Tile = 0x12,
  UnitState_Build_Colony = 0x13,
  UnitState_Auto_Irrigate = 0x14,
  UnitState_Build_Trade_Routes = 0x15,
  UnitState_Auto_Clear_Forest = 0x16,
  UnitState_Auto_Clear_Swamp = 0x17,
  UnitState_Auto_Clear_Pollution = 0x18,
  UnitState_Auto_Save_City_Tiles = 0x19,
  UnitState_Explore = 0x1A,
  UnitState_1B = 0x1B,
  UnitState_Fleeing = 0x1C,
  UnitState_1D = 0x1D,
  UnitState_1E = 0x1E,
  UnitState_Auto_Bombard = 0x1F,
  UnitState_Auto_Air_Bombard = 0x20,
};

enum ImprovementTypeSmallWonderFeatures_Byte1
{
  ITSW_B1_Allows_Healing_In_Enemy_Territory = 0x1,
  ITSW_B1_Required_Good_Must_be_in_City_Radius = 0x2,
  ITSW_B1_Requires_Victorous_Army = 0x4,
};

enum VirtualKey
{
  VK_LBUTTON = 0x1,
  VK_RBUTTON = 0x2,
  VK_CANCEL = 0x3,
  VK_MBUTTON = 0x4,
  VK_XBUTTON1 = 0x5,
  VK_XBUTTON2 = 0x6,
  VK_BACK = 0x8,
  VK_TAB = 0x9,
  VK_CLEAR = 0xC,
  VK_RETURN = 0xD,
  VK_SHIFT = 0x10,
  VK_CONTROL = 0x11,
  VK_MENU = 0x12,
  VK_PAUSE = 0x13,
  VK_CAPITAL = 0x14,
  VK_KANA = 0x15,
  VK_KANJI = 0x19,
  VK_ESCAPE = 0x1B,
  VK_CONVERT = 0x1C,
  VK_NONCONVERT = 0x1D,
  VK_ACCEPT = 0x1E,
  VK_MODECHANGE = 0x1F,
  VK_SPACE = 0x20,
  VK_PRIOR = 0x21,
  VK_NEXT = 0x22,
  VK_END = 0x23,
  VK_HOME = 0x24,
  VK_LEFT = 0x25,
  VK_UP = 0x26,
  VK_RIGHT = 0x27,
  VK_DOWN = 0x28,
  VK_SELECT = 0x29,
  VK_PRINT = 0x2A,
  VK_EXECUTE = 0x2B,
  VK_SNAPSHOT = 0x2C,
  VK_INSERT = 0x2D,
  VK_DELETE = 0x2E,
  VK_HELP = 0x2F,
  VK_0 = 0x30,
  VK_1 = 0x31,
  VK_2 = 0x32,
  VK_3 = 0x33,
  VK_4 = 0x34,
  VK_5 = 0x35,
  VK_6 = 0x36,
  VK_7 = 0x37,
  VK_8 = 0x38,
  VK_9 = 0x39,
  VK_A = 0x41,
  VK_B = 0x42,
  VK_C = 0x43,
  VK_D = 0x44,
  VK_E = 0x45,
  VK_F = 0x46,
  VK_G = 0x47,
  VK_H = 0x48,
  VK_I = 0x49,
  VK_J = 0x4A,
  VK_K = 0x4B,
  VK_L = 0x4C,
  VK_M = 0x4D,
  VK_N = 0x4E,
  VK_O = 0x4F,
  VK_P = 0x50,
  VK_Q = 0x51,
  VK_R = 0x52,
  VK_S = 0x53,
  VK_T = 0x54,
  VK_U = 0x55,
  VK_V = 0x56,
  VK_W = 0x57,
  VK_X = 0x58,
  VK_Y = 0x59,
  VK_Z = 0x5A,
  VK_LWIN = 0x5B,
  VK_RWIN = 0x5C,
  VK_APPS = 0x5D,
  VK_SLEEP = 0x5F,
  VK_NUMPAD0 = 0x60,
  VK_NUMPAD1 = 0x61,
  VK_NUMPAD2 = 0x62,
  VK_NUMPAD3 = 0x63,
  VK_NUMPAD4 = 0x64,
  VK_NUMPAD5 = 0x65,
  VK_NUMPAD6 = 0x66,
  VK_NUMPAD7 = 0x67,
  VK_NUMPAD8 = 0x68,
  VK_NUMPAD9 = 0x69,
  VK_MULTIPLY = 0x6A,
  VK_ADD = 0x6B,
  VK_SEPARATOR = 0x6C,
  VK_SUBTRACT = 0x6D,
  VK_DECIMAL = 0x6E,
  VK_DIVIDE = 0x6F,
  VK_F1 = 0x70,
  VK_F2 = 0x71,
  VK_F3 = 0x72,
  VK_F4 = 0x73,
  VK_F5 = 0x74,
  VK_F6 = 0x75,
  VK_F7 = 0x76,
  VK_F8 = 0x77,
  VK_F9 = 0x78,
  VK_F10 = 0x79,
  VK_F11 = 0x7A,
  VK_F12 = 0x7B,
  VK_F13 = 0x7C,
  VK_F14 = 0x7D,
  VK_F15 = 0x7E,
  VK_F16 = 0x7F,
  VK_F17 = 0x80,
  VK_F18 = 0x81,
  VK_F19 = 0x82,
  VK_F20 = 0x83,
  VK_F21 = 0x84,
  VK_F22 = 0x85,
  VK_F23 = 0x86,
  VK_F24 = 0x87,
  VK_NUMLOCK = 0x90,
  VK_SCROLL = 0x91,
  VK_OEM_NEC_EQUAL = 0x92,
  VK_OEM_FJ_JISHO = 0x92,
  VK_OEM_FJ_MASSHOU = 0x93,
  VK_OEM_FJ_TOUROKU = 0x94,
  VK_OEM_FJ_LOYA = 0x95,
  VK_OEM_FJ_ROYA = 0x96,
  VK_LSHIFT = 0xA0,
  VK_RSHIFT = 0xA1,
  VK_LCONTROL = 0xA2,
  VK_RCONTROL = 0xA3,
  VK_LMENU = 0xA4,
  VK_RMENU = 0xA5,
  VK_BROWSER_BACK = 0xA6,
  VK_BROWSER_FORWARD = 0xA7,
  VK_BROWSER_REFRESH = 0xA8,
  VK_BROWSER_STOP = 0xA9,
  VK_BROWSER_SEARCH = 0xAA,
  VK_BROWSER_FAVORITES = 0xAB,
  VK_BROWSER_HOME = 0xAC,
  VK_VOLUME_MUTE = 0xAD,
  VK_VOLUME_DOWN = 0xAE,
  VK_VOLUME_UP = 0xAF,
  VK_MEDIA_NEXT_TRACK = 0xB0,
  VK_MEDIA_PREV_TRACK = 0xB1,
  VK_MEDIA_STOP = 0xB2,
  VK_MEDIA_PLAY_PAUSE = 0xB3,
  VK_LAUNCH_MAIL = 0xB4,
  VK_LAUNCH_MEDIA_SELECT = 0xB5,
  VK_LAUNCH_APP1 = 0xB6,
  VK_LAUNCH_APP2 = 0xB7,
  VK_OEM_1 = 0xBA,
  VK_OEM_PLUS = 0xBB,
  VK_OEM_COMMA = 0xBC,
  VK_OEM_MINUS = 0xBD,
  VK_OEM_PERIOD = 0xBE,
  VK_OEM_2 = 0xBF,
  VK_OEM_3 = 0xC0,
  VK_OEM_4 = 0xDB,
  VK_OEM_5 = 0xDC,
  VK_OEM_6 = 0xDD,
  VK_OEM_7 = 0xDE,
  VK_OEM_8 = 0xDF,
  VK_OEM_AX = 0x0,
  VK_OEM_102 = 0x0,
  VK_ICO_HELP = 0x0,
  VK_ICO_00 = 0x0,
  VK_PROCESSKEY = 0x0,
  VK_ICO_CLEAR = 0x0,
  VK_PACKET = 0x0,
  VK_OEM_RESET = 0x0,
  VK_OEM_JUMP = 0xEA,
  VK_OEM_PA1 = 0xEB,
  VK_OEM_PA2 = 0xEC,
  VK_OEM_PA3 = 0xED,
  VK_OEM_WSCTRL = 0xEE,
  VK_OEM_CUSEL = 0xEF,
  VK_OEM_ATTN = 0xF0,
  VK_OEM_FINISH = 0xF1,
  VK_OEM_COPY = 0xF2,
  VK_OEM_AUTO = 0xF3,
  VK_OEM_ENLW = 0xF4,
  VK_OEM_BACKTAB = 0xF5,
  VK_ATTN = 0xF6,
  VK_CRSEL = 0xF7,
  VK_EXSEL = 0xF8,
  VK_EREOF = 0xF9,
  VK_PLAY = 0xFA,
  VK_ZOOM = 0xFB,
  VK_NONAME = 0xFC,
  VK_PA1 = 0xFD,
  VK_OEM_CLEAR = 0xFE,
};

// Typedef the following enums for consistency with decompiled code

typedef enum font_style_flags
{
  FSF_NONE = 0,
  FSF_BOLD = 1,
  FSF_ITALIC = 2,
  FSF_UNDERLINE = 4,
} FontStyleFlags;

typedef enum unit_filter
{
	UF_0 = 0,
	UF_1,
	UF_AI_STRAT_A_VIS_TO_B,
	UF_NO_ESCORTEE_OR_CONTAINER_AI_STRAT_A_VIS_TO_B,
	UF_4,
	UF_ATTACKER_VIS_TO_A_OF_CLASS_B,
	UF_DEFENDER_VIS_TO_A_OF_CLASS_B,
	UF_BOMBARDER_VIS_TO_A_OF_CLASS_B,
	UF_8,
	UF_9,
	UF_10,
	UF_11,
	UF_12,
	UF_13,
	UF_IN_STATE_A_VIS_TO_B,
	UF_15,
	UF_16,
	UF_17,
	UF_KING_VIS_TO_A,
	UF_FLAG_VIS_TO_A,
	UF_20,
	UF_21,
	UF_22
} UnitFilter;

typedef enum leader_kind
{
	LK_Military = 1,
	LK_Scientific = 2
} LeaderKind;

typedef enum city_loc_validity
{
	CLV_OK = 0,
	CLV_1,
	CLV_BLOCKED,
	CLV_WATER,
	CLV_INVALID_TERRAIN,
	CLV_CITY_TOO_CLOSE
} CityLocValidity;

typedef enum pass_between_validity
{
	PBV_OK = 0,
	PBV_REQUIRES_ROAD,
	PBV_INVALID_SEA_MOVE,
	PBV_CANNOT_EMBARK,
	PBV_GENERIC_INVALID_MOVE,
} PassBetweenValidity;

typedef enum adjacent_move_validity
{
	AMV_OK = 0,
	AMV_1,
	AMV_NO_MOVES,
	AMV_CANNOT_ATTACK_CITY_WITH_NONCOMBATANT,
	AMV_CANNOT_ATTACK_CITY_WITH_SEA_UNIT,
	AMV_CANNOT_AMPHIB_ATTACK_CITY,
	AMV_CANNOT_AMPHIB_ATTACK,
	AMV_CANNOT_ATTACK_WITH_NONCOMBATANT,
	AMV_REQUIRES_BLITZ,
	AMV_TRIGGERS_WAR,
	AMV_10,
	AMV_11,
	AMV_REQUIRES_ROAD,
	AMV_INVALID_SEA_MOVE,
	AMV_14,
	AMV_CANNOT_EMBARK,
	AMV_16,
	AMV_CANNOT_PASS_BETWEEN,
} AdjacentMoveValidity;

typedef enum diplo_message
{
	DM_AI_FIRST_CONTACT		 = 0x00,
	DM_AI_FIRST_DEAL		 = 0x01,
	DM_AI_DEMAND_TRIBUTE		 = 0x02,
	DM_AI_ACCEPT_TRIBUTE		 = 0x03,
	DM_AI_BLUFF_IS_CALLED		 = 0x04,
	DM_AI_RENEGOTIATE		 = 0x05,
	DM_AI_RENEGOTIATE_PEACE		 = 0x06,
	DM_AI_BORDER_WARNING		 = 0x07,
	DM_AI_BORDER_WAR		 = 0x08,
	DM_AI_DONT_TEST			 = 0x09,
	DM_AI_OVERLOOK			 = 0x0A,
	DM_AI_WILL_ACCEPT		 = 0x0B,
	DM_AI_COUNTER			 = 0x0C,
	DM_AI_NO_COUNTER		 = 0x0D,
	DM_AI_TECH_TRADE		 = 0x0E,
	DM_AI_LUXURY_TRADE		 = 0x0F,
	DM_AI_LOAN_OFFER		 = 0x10,
	DM_AI_NO_LOAN_OFFER		 = 0x11,
	DM_AI_MAP_TRADE			 = 0x12,
	DM_AI_NO_MAP_TRADE		 = 0x13,
	DM_AI_COMMUNICATIONS		 = 0x14,
	DM_AI_GIVE_PEACE_TREATY		 = 0x15,
	DM_AI_PEACE_TREATY_THREAT	 = 0x16,
	DM_AI_NO_PEACE_TREATY_THREAT	 = 0x17,
	DM_AI_PEACE_TREATY		 = 0x18,
	DM_AI_NO_PEACE_TREATY		 = 0x19,
	DM_AI_MUTUAL_PROTECTION		 = 0x1A,
	DM_AI_RIGHT_OF_PASSAGE		 = 0x1B,
	DM_AI_NO_RIGHT_OF_PASSAGE	 = 0x1C,
	DM_AI_MILITARY_ALLIANCE		 = 0x1D,
	DM_AI_TRADE_EMBARGO		 = 0x1E,
	DM_AI_ALLIANCE_GREETINGS	 = 0x1F,
	DM_AI_PEACE_GREETINGS		 = 0x20,
	DM_AI_WAR_GREETINGS		 = 0x21,
	DM_AI_GIVE_TRIBUTE		 = 0x22,
	DM_AI_RECEIVE_GIFT		 = 0x23,
	DM_AI_ACCEPT			 = 0x24,
	DM_AI_WEAK_REJECT		 = 0x25,
	DM_AI_NEUTRAL_REJECT		 = 0x26,
	DM_AI_STRONG_REJECT		 = 0x27,
	DM_AI_TOTAL_REJECT		 = 0x28,
	DM_AI_REJECT_THREAT		 = 0x29,
	DM_AI_REJECT_BROKEN_DEAL	 = 0x2A,
	DM_AI_REJECT_PASSAGE_VIOLATION	 = 0x2B,
	DM_AI_REJECT_MILITARY_RETRACT	 = 0x2C,
	DM_AI_THANKS			 = 0x2D,
	DM_AI_WHATEVER			 = 0x2E,
	DM_AI_PROPOSAL_RESPONSE		 = 0x2F,
	DM_AI_COUNTER_RESPONSE		 = 0x30,
	DM_AI_REJECT_RESPONSE		 = 0x31,
	DM_AI_UNDERSTAND_RESPONSE	 = 0x32,
	DM_AI_WILL_RETREAT		 = 0x33,
	DM_AI_WILL_RETURN_HOME		 = 0x34,
	DM_USER_ACCEPT			 = 0x35,
	DM_USER_REJECT			 = 0x36,
	DM_USER_COUNTER			 = 0x37,
	DM_USER_THREAT			 = 0x38,
	DM_USER_ASK_FOR_COUNTER		 = 0x39,
	DM_USER_OFFER			 = 0x3A,
	DM_USER_ASK_FOR_TREAT		 = 0x3B,
	DM_USER_GIFT			 = 0x3C,
	DM_USER_ASK_FOR_EXCHANGE	 = 0x3D,
	DM_USER_NEVERMIND		 = 0x3E,
	DM_USER_PROPOSAL		 = 0x3F,
	DM_USER_UNDERSTAND		 = 0x40,
	DM_USER_ASK_FOR_LOAN		 = 0x41,
	DM_USER_MAP_TRADE		 = 0x42,
	DM_USER_PEACE_TREATY_THREAT	 = 0x43,
	DM_USER_PEACE_TREATY		 = 0x44,
	DM_USER_AUTO_RIGHT_OF_PASSAGE	 = 0x45,
	DM_USER_BORDER_WARNING		 = 0x46,
	DM_USER_BORDER_WAR		 = 0x47,
	DM_USER_WILL_GIVE_TRIBUTE	 = 0x48,
	DM_USER_WILL_NOT_GIVE_TRIBUTE	 = 0x49,
	DM_USER_WILL_RETREAT		 = 0x4A,
	DM_USER_WILL_RETURN_HOME	 = 0x4B,
	DM_USER_WAR			 = 0x4C,
	DM_USER_EXIT			 = 0x4D
} DiploMessage;

typedef enum toggleable_rules {
	TR_ALLOW_DOMINATION_VICTORY   = 0x1,
	TR_ALLOW_SPACE_RACE_VICTORY   = 0x2,
	TR_ALLOW_DIPLOMATIC_VICTORY   = 0x4,
	TR_ALLOW_CONQUEST_VICTORY     = 0x8,
	TR_ALLOW_CULTURAL_VICTORY     = 0x10,
	TR_CULTURALLY_LINKED_STARTS   = 0x40,
	TR_RESPAWN_AI_PLAYERS	      = 0x80,
	TR_PRESERVE_RANDOM_SEED	      = 0x100,
	TR_ACCELERATED_PRODUCTION     = 0x200,
	TR_ELIMINATION		      = 0x400,
	TR_REGICIDE		      = 0x800,
	TR_MASS_REGICIDE	      = 0x1000,
	TR_VICTORY_POINT_SCORING      = 0x2000,
	TR_CAPTURE_THE_PRINCESS	      = 0x4000,
	TR_ALLOW_CULTURAL_CONVERSIONS = 0x8000,
	TR_ALLOW_WONDER_VICTORY	      = 0x10000,
	TR_REVERSE_CAPTURE_THE_FLAG   = 0x20000,
	TR_ALLOW_SCIENTIFIC_LEADERS   = 0x40000,
} ToggleableRules;

typedef enum preferences {
	P_ALWAYS_WAIT_AT_END_OF_TURN        = 0x1,
	P_AUTO_SAVE                         = 0x2,
	P_SOUND_FX_ON                       = 0x4,
	P_MUSIC_ON                          = 0x8,
	P_ANIMATE_MANUAL_MOVES              = 0x10,
	P_ANIMATE_AUTO_MOVES                = 0x20,
	P_ANIMATE_FRIEND_MOVES              = 0x40,
	P_ANIMATE_ENEMY_MOVES               = 0x80,
	P_SHOW_MANUAL_MOVES                 = 0x100,
	P_SHOW_AUTO_MOVES                   = 0x200,
	P_ANIMATE_BATTLES                   = 0x400,
	P_SHOW_TEAM_COLOR_DISC              = 0x800,
	P_SHOW_YIELDS_ON_MAP                = 0x1000,
	P_SHOW_UNITS_OVER_CITIES            = 0x2000,
	P_CANCEL_ORDERS_FOR_FRIENDLY_UNIT   = 0x4000,
	P_CANCEL_ORDERS_FOR_ENEMY_UNIT      = 0x8000,
	P_ASK_AFTER_UNIT_CONSTRUCTION       = 0x10000,
	P_ALWAYS_BUILD_PREVIOUS_UNIT        = 0x20000,
	P_SHOW_CIVIL_DISORDER_POPUP         = 0x40000,
	P_SHOW_PEDIA_BOOK_CURSOR            = 0x80000,
	P_CAPITAL_GOVERNOR_IS_DEFAULT       = 0x100000,
	P_COLOR_BLIND_HELP                  = 0x200000,
	P_DISABLE_POP_LIMIT_WARNINGS        = 0x400000,
	P_SHOW_FRIEND_MOVES                 = 0x800000,
	P_SHOW_ENEMY_MOVES                  = 0x1000000,
	P_SHOW_CITY_POP_DROPSHADOW          = 0x2000000,
	P_SHOW_WONDER_INITIATION_POPUP      = 0x4000000,
	P_ALWAYS_RENEGOTIATE_DEALS          = 0x8000000,
	P_SHOW_ADVANCED_UNIT_ACTION_BUTTONS = 0x10000000,
	P_SHOW_FEWER_MP_POPUPS              = 0x20000000,
	P_DO_NOT_AUTO_SELECT_UNITS_IN_MP    = 0x40000000
} Preferences;

struct IntList
{
  int field_0;
  int length;
  int field_8;
  int * contents;
  int * contents_end;
  int * also_contents_end;
};

struct City_Base_vtable
{
  int m00;
  int m01;
  int m02;
  int m03;
  int m04;
  int m05;
  int m06;
  int m07;
  int m08;
  int m09;
  int m10;
  int m11;
  char (__fastcall * has_pollution_or_craters_in_range) (City *);
//  int (__thiscall *m13)(City *);
  void *m13;
//  int (__thiscall *m14)(int, int);
  void *m14;
  enum UnitStateType (__fastcall * instruct_worker) (City * this, int edx, int tile_x, int tile_y, byte param_3, Unit * worker);
  int m16;
  int m17;
  int m18;
  int m19;
  int m20;
  int m21;
  int m22;
  int m23;
  int m24;
  int m25;
//  char (__thiscall *m26)(City *, int *, char *, char *);
  void *m26;
  byte (__fastcall * m27) (City *, int, Unit *, int);
};

struct Citizen
{
  int vtable;
  int field_4;
  char str_CTZN[4];
  int field_C;
  int field_10;
  int field_14;
  int field_18;
  int Body;
  char Status;
  char field_21;
  short Tile_Index2;
  int field_24[65];
  int Mood;
  int Gender;
  int Turn_Of_Create;
  int CityID;
  int ID;
  int WorkerType;
  int RaceID;
  int field_144;
  int Turn_1;
};

struct RulerTitle
{
  char Male[32];
  char Female[32];
};

struct UnitType
{
  int field_0;
  int field_4;
  char Name[32];
  char Civilipedia_Entry[32];
  int Bombard_Strength;
  int Bombard_Range;
  int Transport_Capacity;
  int Cost;
  int Defence;
  int ID;
  int Attack;
  int OperationalRange;
  int PopulationCost;
  int FireRate;
  int Movement;
  int AdvReq;
  int UpgradeToID;
  int ResReq[3];
  int UnitAbilities;
  int AI_Strategy;
  int Available_To;
  int field_94;
  int field_98;
  enum UnitTypeClasses Unit_Class;
  int IconID_2;
  int Hit_Point_Bonus;
  int Standard_Actions;
  int Special_Actions;
  int Worker_Actions;
  int Air_Missions;
  int field_B8;
  int Bombard_Fix;
  int field_C0;
  int field_C4;
  int field_C8;
  int field_CC;
  int b_Not_King;
  int field_D4;
  int field_D8;
  int field_DC;
  int field_E0;
  int field_E4;
  int field_E8;
  int field_EC;
  int field_F0;
  int field_F4;
  IntList stealth_attack_targets;
  int field_110;
  int field_114;
  int field_118;
  int field_11C;
  int field_120;
  int field_124;
  int Create_Craters;
  float WorkerStrength;
  int Extra_Abilities;
  int Air_Defence;
};

struct UnitItem
{
  int field_0;
  Unit_Body *Unit;
};

struct Tile_vtable
{
  int m0_Get_SquareType_Container;
  char (__fastcall *m1_Set_Square_Type)(Tile *, __, int, signed int);
  void (__fastcall *m2_Set_Square_Real_Type)(Tile *, __, int, signed int);
  int (__fastcall *m3_Set_Square_Base_Type)(Tile *, __, int);
  void (__fastcall *m4_Dispose)(Tile *this, __, int);
  int m5;
  char (__fastcall *m06_Check_Airfield)(Tile *, __, int);
  int (__fastcall *m7_Check_Barbarian_Camp)(Tile *, __, int);
  int m08_Check_field_30_bit_23;
  int m9;
  int m10;
  void (__fastcall *m11_set_field_20_lobyte)(Tile *, __, signed int);
  int (__fastcall *m12_Check_Forest_Pines)(Tile *);
  int (__fastcall *m13_Check_Fortress)(Tile *, __, int);
  char (__fastcall *m14_Check_Barricade)(Tile *, __, int);
  char (__fastcall *m15_Check_Goody_Hut)(Tile *, __, int);
  int m16;
  char (__fastcall *m17_Check_Irrigation)(Tile *, __, int);
  int (__fastcall *m18_Check_Mines)(Tile *, __, int);
  char (__fastcall *m19_Check_Outpost)(Tile *, __, int);
  unsigned char (__fastcall *m20_Check_Pollution)(Tile *, __, int);
  int (__fastcall *m21_Check_Crates)(Tile *, __, int);
  int (__fastcall * m22_has_pollution_or_craters) (Tile *, __, int);
  int (__fastcall *m23_Check_Railroads)(Tile *, __, int);
  unsigned char (__fastcall *m24_Check_River)(Tile *);
  int (__fastcall *m25_Check_Roads)(Tile *this, __, int);
  char (__fastcall *m26_Check_Tile_Building)(Tile *);
  int (__fastcall *m27_Check_Special_Resource)(Tile *);
  char (__fastcall *m28_Check_field_30_bit_15)(Tile *);
  int (__fastcall *m29_Check_Mountain_Snowcap)(Tile *);
  int (__fastcall *m30_Check_is_LM)(Tile *);
  void (__fastcall *m31_set_field_30_bit_29)(Tile *, __, char);
  char (__fastcall *m32_Check_field_30_bit_19)(Tile *);
  char (__fastcall *m33_Check_Radar)(Tile *, __, int);
  int (__fastcall *m34_Check_field_20_hiword)(Tile *);
  int (__fastcall *m35_Check_Is_Water)(Tile *);
  int (__fastcall *m36_Get_Ruins)(Tile *);
  char (__fastcall *m37_Get_River_Code)(Tile *);
  char (__fastcall *m38_Get_Territory_OwnerID)(Tile *);
  int (__fastcall *m39_Get_Resource_Type)(Tile *);
  int (__fastcall *m40_get_TileUnit_ID)(Tile *);
  int (__fastcall *m41_Get_Square_Parts)(Tile *);
  int (__fastcall *m42_Get_Overlays)(Tile *, __, int);
  int (__fastcall *m43_Get_field_30)(Tile *);
  short (__fastcall *m44_Get_Barbarian_TribeID)(Tile *);
  short (__fastcall *m45_Get_City_ID)(Tile *);
  short (__fastcall *m46_Get_ContinentID)(Tile *);
  short (__fastcall *m47_Get_Tile_BuildingID)(Tile *);
  short (__fastcall *m48_Get_field_20_hiword)(Tile *);
  int (__fastcall *m49_Get_Square_RealType)(int);
  int (__fastcall *m50_Get_Square_BaseType)(Tile *);
  void (__fastcall *m51_Unset_Tile_Flags)(Tile *, __, int, int, int, int);
  void (__fastcall *m52_Unset_River_Code_call_m53)(Tile *, __, char);
  int (__fastcall *m53_set_River_Code_field_30)(Tile *);
  void (__fastcall *m54_Set_Square_Parts)(Tile *, __, int);
  void (__fastcall *m55_Set_Barbarian_TribeID)(Tile *, __, short);
  int (__fastcall *m56_Set_Tile_Flags)(Tile *, __, int, int, int, int);
  void (__fastcall *m57_Set_CityID)(Tile *, __, int);
  void (__fastcall *m58_Set_ContinentID)(Tile *, __, int);
  int (__fastcall *m59_Process_Cities)(Tile *, __, int);
  void (__fastcall *m60_Set_Ruins)(Tile *, __, int);
  int (__fastcall *m61_Set_River_Code_Flood_Plains)(Tile *, __, int);
  void (__fastcall *m62_Set_Tile_BuildingID)(Tile *, __, short);
  int (__fastcall *m63_get_field_34)(Tile *);
  int (__fastcall *m64_set_field_34)(Tile *, __, int);
  int m65;
  void (__fastcall *m66_Set_Territory_OwnerID)(Tile *, __, char);
  int m67;
  int m68;
  int (__fastcall *m69_get_Tile_City_CivID)(Tile *);
  int (__fastcall *m70_Get_Tile_Building_OwnerID)(Tile *);
  int (__fastcall *m71_Check_Worker_Job)(Tile *);
  int (__fastcall *m72_Get_Pollution_Effect)(Tile *);
  int m73;
  void (__fastcall *m74_Set_Square_Type)(Tile *, __, int, int, int);
  void (__fastcall *m75_Clear)(Tile *, __, int);
  int m76;
};

struct RaceEraAnimations
{
  char ForwardFilename[260];
  char ReverseFilename[260];
};

struct String32
{
  char S[32];
};

struct String24
{
  char S[24];
};

struct String16
{
  char field_0[16];
};

struct Race_vtable
{
  byte (__fastcall * CheckBonus) (Race *, int, enum RaceBonuses);
  int (__fastcall * GetBonuses) (Race *);
  char * (__fastcall * GetAdjectiveName) (Race *);
  char * (__fastcall * GetCountryName) (Race *);
  char * (__fastcall * GetLeaderName) (Race *);
  char * (__fastcall * GetTitle) (Race *);
  char * (__fastcall * GetSingularName) (Race *);
  int (__fastcall * GetStartupAdvance) (Race *, int, int);
  int (__fastcall * f1) (Race *);
};

struct Citizens
{
  int vtable;
  Citizen_Info *Items;
  int field_8;
  int field_C;
  int LastIndex;
  int Capacity;
};

struct Citizen_Info
{
  int field_0;
  Citizen_Body *Body;
};

struct Citizen_Body
{
  int vtable;
  int field_20[66];
  int Mood;
  int Gender;
  int field_130;
  int field_134;
  int field_138;
  int WorkerType;
  int RaceID;
  int field_144;
  int field_148;
};

struct Advance
{
  int field_0;
  char Name[32];
  char Civilopedia_Entry[32];
  int Cost;
  int Era;
  int Civ_Entry_Index;
  int X;
  int Y;
  int Reqs[4];
  enum AdvanceTypeFlags Flags;
  int Flavours;
  int field_70;
};

struct JGL_Graphics_vtable
{
  int m00;
  int m01_Create_Window;
  int m02;
  int m03;
  int m04;
  int m05;
  int m06;
  int m07;
  int m08;
  int m09;
  int m10;
  int m11;
  int m12;
  int m13;
  int m14;
  int m15;
  int m16;
  int m17;
  int m18;
  int m19;
  int m20;
  int m21;
  int m22;
  int m23;
  int m24;
  int m25;
  int m26;
  int m27;
  int m28;
  int m29;
  int m30;
  int m31_Create_Image;
  int m32;
  int m33;
  int m34;
  int m35;
  int m36;
  int m37;
  int m38;
  int m39_Set_Triple_Values;
  int m40_Get_Triple_Values;
  int m41;
  int m42;
  int m43;
  int m44;
  int m45_Null;
  int m46;
  int m47;
  int m48;
  int m49;
  int m50;
  int m51;
  int m52;
  int m53;
  int m54;
  int m55;
  int m56;
  int m57;
  int m58;
  int m59;
  int m60;
  int m61;
  int m62;
  int m63;
  int m64;
  int m65;
  int m66;
  int m67;
  int m68;
  int m69;
  int m70;
  int m71;
  int m72;
  int m73;
  int m74;
  int m75;
  int m76;
  int m77;
  int m78;
  int m79;
  int m80;
  int m81;
  int m82;
  int m83;
  int m84;
  int m85;
};

struct CityItem
{
  int field_0;
  City_Body *City;
};

struct Citizen_Base
{
  Citizen_Base_vtable *vtable;
  int field_4;
  char str_CTZN[4];
  int field_C;
  int field_10;
  int field_14;
  int field_18;
  Citizen_Body Body;
};

struct Tile_Base_vtable
{
  int ctor;
  int m1;
  int m2;
  int m3;
  int m4;
};

struct Citizen_Base_vtable
{
  int ctor;
  int m1;
  int m2;
  int m3;
  int m4;
};

struct Base
{
  Base_vtable *vtable;
  int field_4;
  int ClassName;
  int DataLength;
  int field_10;
  void *pStart;
  void *pEnd;
};

struct Continent_Body
{
  int vtable;
  int V1;
  int TileCount;
};

struct Buildings_Info
{
  Base Base;
  Buildings_Info_Item *Items;
  int Count;
  int field_24;
  int field_28;
  int field_2C;
  int field_30;
};

struct Population
{
  Base Base;
  int Jobs;
  int Size;
};

struct City_Improvements
{
  Base Base;
  char Improvements[32];
  int Count;
  int Aligned_Capacity;
};

struct Cities
{
  int vtable;
  CityItem *Cities;
  int V1;
  int V2;
  int LastIndex;
  int Capacity;
};

struct Units
{
  int vtable;
  UnitItem *Units;
  int V1;
  int V2;
  int LastIndex;
  int Capacity;
};

struct Main_Window
{
  int vtable;
  int field_4[5029];
  int ScreenX;
  int ScreenY;
  int field_4EA0[10145];
  int field_ED24;
  int field_ED28;
};

struct Map_Body
{
  Base Base;
};

struct Map_vtable
{
//  char (__thiscall *m00_Create_Tiles)(Map *, int);
  void *m00_Create_Tiles;
//  int (__thiscall *m01_Create_WH_Tiles)(Map *);
  void *m01_Create_WH_Tiles;
  int m02;
//  void (__thiscall *m03)(Map *);
  void *m03;
//  void (__thiscall *m04_Clear_Tiles)(Map *);
  void *m04_Clear_Tiles;
  int m05;
  int m06;
  int m07_Init_Continents2;
  int m08;
  int m09_Init;
  byte (__fastcall * m10_Get_Map_Zoom) (Map * this);
  int m11_Get_Tile_by_XY2;
//  Tile *(__thiscall *m12_Get_Tile_by_XY)(Map *, int, int);
  void *m12_Get_Tile_by_XY;
  Tile *(__fastcall * m13_Get_Tile_by_Index) (Map *, int, int);
  int m14;
  int m15_null;
  int m16;
  int m17;
//  char (__thiscall *m18)(Map *, int, int);
  void *m18;
//  int (__thiscall *m19_Create_Tiles)(Map *, Tile **);
  void *m19_Create_Tiles;
  int m20;
  byte (__fastcall * is_near_lake) (Map * this, int edx, int x, int y, int num_tiles);
  byte (__fastcall * is_near_river) (Map * this, int edx, int x, int y, int num_tiles);
  int m23;
  int (__fastcall * has_fresh_water) (Map * this, int edx, int x, int y);
  int m25;
  int m26;
  int m27;
  int m28_Find_Center_Neighbour_Point;
  int m29;
//  void (__thiscall *m30_Init_Tiles)(Map *this, int SquareType, int);
  void *m30_Init_Tiles;
//  void (__thiscall *m31)(Map *this);
  void *m31;
//  void (__thiscall *m32)(Map *);
  void *m32;
  Continent * (__fastcall * m33_Get_Continent) (Map *, int, int);
//  int (__thiscall *m34_Get_Continent_Count)(Map *);
  void *m34_Get_Continent_Count;
//  int (__thiscall *m35_Get_BIC_Sub_Data)(Map *this, int Object_Type, int Object_Index, void *Object);
  void *m35_Get_BIC_Sub_Data;
};

struct Base_List_Item
{
  int V;
  void *Object;
};

struct Base_List
{
  int vtable;
  Base_List_Item *Items;
  int V1;
  int V2;
  int LastIndex;
  int Capacity;
};

struct TileUnits
{
  Base_List Base;
  int DefaultValue;
};

struct Game
{
  Base Base;
};

struct Unknown_3
{
  int vtable;
  int V1;
  void *Units[1024];
  int Unit_Count;
  int field_100C[572];
  int field_18FC;
  int field_1900;
  int field_1904;
};

struct Resource_Type
{
  int ID;
  char Name[24];
  char Civilopedia_Entry[32];
  enum Resource_Type_Classes Class;
  int AppearanceRatio;
  int DisappearanceProbability;
  int IconID;
  int RequireID;
  int Food;
  int Shield;
  int Commerce;
};

struct String56
{
  char S[56];
};

struct String64
{
  char S[64];
};

struct Worker_Job
{
  int V;
  String32 Name;
  String32 Civilopedia_Entry;
  int TurnToComplete;
  int RequireID;
  int Resource1ID;
  int Resource2ID;
  String32 Order;
};

struct World_Size
{
  int V;
  int OptimalCityCount;
  int TechRate;
  int field_C;
  int field_10;
  int field_14;
  int field_18;
  int field_1C;
  int field_20;
  String32 Name;
  int Height;
  int CivDistance;
  int CivCount;
  int Width;
};

struct Cultural_Levels
{
  String64 *Levels;
  int Multiplier;
  int Count;
};

struct General
{
  int vtable;
  int V1;
  String32 CityTypeNames[3];
  int *SpaceshipPartsNeeded;
  int BarbarianAdvUnitID;
  int BarbarianBasicUnitID;
  int BarbarianSeaUnitID;
  int ArmySupportCities;
  int TurnPenalty_Hurry;
  int TurnPenalty_Draft;
  int ShieldCostPerGold;
  int DefenceBonus_Fortress;
  int HappyCitizensAffect;
  int V2;
  int V3;
  int ForestValueInShields;
  int ShieldValueInGold;
  int CitizenValueInShields;
  int Default_Difficulty;
  int BattleCreatedUnitID;
  int BuildArmyUnitID;
  int DefenceBonus_Building;
  int DefenceBonus_Citizen;
  int DefaultMoneyResource;
  int InterceptChance_Air;
  int InterceptChance_Stealth;
  int SpaceshipParts_Count;
  int StartingTreasury;
  int V4;
  int FoodPerCitizen;
  int DefenceBonus_River;
  int ChanceOfRioting;
  int DefaultUnit_Scout;
  int DefaultUnit_Captured;
  int RoadsMovementRate;
  int DefaultUnit_Start1;
  int DefaultUnit_Start2;
  int LoveKing_Min_Poppulation;
  int DefenceBonus_Cities[3];
  int MaximumSize_Town;
  int MaximumSize_City;
  int V5;
  int DefenceBonus_Fortification;
  Cultural_Levels CulturalLevels;
  int BorderFactor;
  int FutureTechCost;
  int GoldenAgeTurns;
  int ResearchTime_Max;
  int ResearchTime_Min;
  int FlagUnitID;
  int FoodUpdateCost;
};

struct Fighter
{
  Unit * attacker;
  Unit * defender;
  byte defender_eligible_to_retreat;
  byte attacker_eligible_to_retreat;
  byte field_A;
  byte field_B;
  int attack_direction;
  int field_10;
  int attacker_location_x;
  int attacker_location_y;
  int defender_location_x;
  int defender_location_y;
};

struct Unknown_6
{
  int vtable;
};

struct Tile_Type_Image
{
  int vtable;
  String64 Name;
  String64 CivilopediaEntry;
  String64 SmallImageFile;
  String64 LargeImageFile;
  int field_104;
  int field_108;
  int field_10C;
  int field_110;
  int field_114;
  int field_118;
  int field_11C;
  int field_120;
  int field_124;
  int field_128;
  int field_12C;
  Tile_Type *TileType;
};

struct Improvement
{
  int field_0[17];
  String32 Name;
  String32 CivilopediaEntry;
  int Double_Happiness_Building;
  int Gain_Building_Global;
  int Gain_Building_Continent;
  int ImprovementID;
  int Cost;
  int Culture;
  int Combat_Bombard;
  int Naval_Bombard_Defence;
  int Combat_Defence;
  int NavalDefenceBonus;
  int Maintenance;
  int Happy_Faces_All;
  int Happy_Faces;
  int Unhappy_Faces_All;
  int Unhappy_Faces;
  int RequiredBuildingsCount;
  int AirPower;
  int NavalPower;
  int Pollution;
  int Production;
  int GovernmentID;
  int SpaceshipPart;
  int RequiredID;
  int ObsoleteID;
  int Resource1ID;
  int Resource2ID;
  enum ImprovementTypeFlags ImprovementFlags;
  enum ImprovementTypeCharacteristics Characteristics;
  int SmallWonderFlags;
  int WonderFlags;
  int ArmyRequiredCount;
  int Flavours;
  int field_104;
  int Unit_Produced;
  int Unit_Frequency;
};

struct Citizen_Type
{
  int V1;
  int V2;
  String32 Name;
  String32 CivilopediaEntry;
  String32 PluralName;
  int RequireID;
  int Luxury;
  int Research;
  int Tax;
  int Corruption;
  int Construction;
};

struct Map_Renderer_vtable
{
  int m00;
  int m01;
  int m02;
  int m03;
  int m04;
//  int (__thiscall *m05_Check_dword_9AFD34)(int);
  void *m05_Check_dword_9AFD34;
//  void (__thiscall *m06_Draw_Tile_Terrain)(Map_Renderer *this, int, int, int, int, int, int);
  void *m06_Draw_Tile_Terrain;
//  void (__thiscall *m07_Draw_Tile_City_and_Victory_Point)(int, int, int, int, int, int, int);
  void *m07_Draw_Tile_City_and_Victory_Point;
//  void (__thiscall *m08_Draw_Tile_Forests_Jungles_Swamp)(int, int, int, int, int, int);
  void *m08_Draw_Tile_Forests_Jungles_Swamp;
//  void (__thiscall *m09_Draw_Tile_Resources)(int, int, int, int, int, int, int);
  void *m09_Draw_Tile_Resources;
//  void (__thiscall *m10_Draw_Tile_Mountains_Hills_Volcano)(int, int, int, int, int, int, int);
  void *m10_Draw_Tile_Mountains_Hills_Volcano;
//  void (__thiscall *m11_Draw_Tile_Irrigation)(int, int, int, int, int, int, int);
  void *m11_Draw_Tile_Irrigation;
//  void (__thiscall *m12_Draw_Tile_Buildings)(int, int, int, int, int, int, int);
  void *m12_Draw_Tile_Buildings;
//  void (__thiscall *m13_Draw_Tile_Pollution)(int, int, int, int, int, int, int);
  void *m13_Draw_Tile_Pollution;
//  void (__thiscall *m14_Draw_Tile_Railroads)(int, int, int, int, int, int, int);
  void *m14_Draw_Tile_Railroads;
//  void (__thiscall *m15_Draw_Tile_Rivers_and_Flood_Plains)(int, int, int, int, int, int, int);
  void *m15_Draw_Tile_Rivers_and_Flood_Plains;
//  void (__thiscall *m16_Draw_Tile_Roads)(int, int, int, int, int, int, int);
  void *m16_Draw_Tile_Roads;
//  void (__thiscall *m17_Draw_Tile_Territory)(int, int, int, int, int, int);
  void *m17_Draw_Tile_Territory;
//  void (__thiscall *m18_Draw_Tile_Tnt)(int, int, int, int, int, int);
  void *m18_Draw_Tile_Tnt;
//  void (__thiscall *m19_Draw_Tile_by_XY_and_Flags)(Map_Renderer *, int, int, int, int, signed int, int, int, int);
  void *m19_Draw_Tile_by_XY_and_Flags;
  int m20_Draw_Tiles;
//  void (__thiscall *m21_Draw_Tiles_by_Flags)(Map_Renderer *, int, int, int, int, int, int X, int Y, int Flags);
  void *m21_Draw_Tiles_by_Flags;
//  void (__thiscall *m22_Draw_Airfield)(Map_Renderer *, int, int, int, int, int, int);
  void *m22_Draw_Airfield;
//  void (__thiscall *m23_Draw_Barbarian_Camp)(Map_Renderer *, int);
  void *m23_Draw_Barbarian_Camp;
//  void (__thiscall *m24_Draw_Terrain_Images)(Map_Renderer *, signed int, int, int, int, int, int, int);
  void *m24_Draw_Terrain_Images;
//  void (__thiscall *m25_Draw_City_Image)(Map_Renderer *, int, int, int, int, int, int);
  void *m25_Draw_City_Image;
//  void (__thiscall *m26_Draw_Colony)(Map_Renderer *, int, int, int, int, int, int);
  void *m26_Draw_Colony;
//  void (__thiscall *m27_Draw_Flood_Plains)(Map_Renderer *, int, int, int, int);
  void *m27_Draw_Flood_Plains;
//  int (__thiscall *m28_Draw_Forests_Large)(Map_Renderer *, signed int, int, int, int, int, int);
  void *m28_Draw_Forests_Large;
//  int (__thiscall *m29_Draw_Forests_Pines)(Map_Renderer *, signed int, int, int, int, int, int);
  void *m29_Draw_Forests_Pines;
//  int (__thiscall *m30_Draw_Forests_Small)(Map_Renderer *, signed int, int, int, int, int, int);
  void *m30_Draw_Forests_Small;
//  void (__thiscall *m31_Draw_Fortress)(Map_Renderer *, int, int, int, int, int, int);
  void *m31_Draw_Fortress;
//  void (__thiscall *m32_Draw_Barricade)(Map_Renderer *, int, int, int, int, int, int);
  void *m32_Draw_Barricade;
//  int (__thiscall *m33_Draw_Resource)(Map_Renderer *, int, int, int, int, int, int, int);
  void *m33_Draw_Resource;
//  void (__thiscall *m34_Draw_Goody_Huts)(Map_Renderer *, int, int, int, int);
  void *m34_Draw_Goody_Huts;
//  void (__thiscall *m35_Check_Map_Zoom)(int, int, int, int, int, int);
  void *m35_Check_Map_Zoom;
//  void (__thiscall *m36_Check_Map_Zoom)(int, int, int, int, int, int);
  void *m36_Check_Map_Zoom;
//  int (__thiscall *m37_Draw_Hills)(Map_Renderer *, int, int, int, int, int, int);
  void *m37_Draw_Hills;
//  signed int (__thiscall *m38_Draw_Irrigation)(Map_Renderer *, signed int, int, int, int, int);
  void *m38_Draw_Irrigation;
//  int (__thiscall *m39_Draw_Grassland_Jungles_Large)(Map_Renderer *, signed int, int, int, int, int);
  void *m39_Draw_Grassland_Jungles_Large;
//  int (__thiscall *m40_Draw_Grassland_Jungles_Small)(Map_Renderer *, signed int, int, int, int, int);
  void *m40_Draw_Grassland_Jungles_Small;
//  void (__thiscall *m41_Draw_Mines)(Map_Renderer *, int, int, int);
  void *m41_Draw_Mines;
  int m42_Draw_Mountains;
//  int (__thiscall *m43_Draw_Marsh_Large)(Map_Renderer *, signed int, int, int, int, int);
  void *m43_Draw_Marsh_Large;
//  int (__thiscall *m44_Draw_Marsg_Small)(Map_Renderer *, signed int, int, int, int, int);
  void *m44_Draw_Marsg_Small;
//  int (__thiscall *m45_Draw_Volcano)(Map_Renderer *, int, int, int, int, int);
  void *m45_Draw_Volcano;
//  void (__thiscall *m46_Draw_Outpost)(Map_Renderer *, int, int, int, int, int, int);
  void *m46_Draw_Outpost;
//  int (__thiscall *m47_Draw_Polar_Icecaps)(Map_Renderer *, int, int, int, int);
  void *m47_Draw_Polar_Icecaps;
//  int (__thiscall *m48_Draw_Pollution)(Map_Renderer *, int, int, int, int);
  void *m48_Draw_Pollution;
//  void (__thiscall *m49_Draw_Craters)(Map_Renderer *, int, int, int, int);
  void *m49_Draw_Craters;
//  int (__thiscall *m50_Draw_Railroads)(Map_Renderer *this, int Image_Index, int, int, int);
  void *m50_Draw_Railroads;
//  int (__thiscall *m51_Draw_Rivers)(Map_Renderer *, signed int, int, int, int, int);
  void *m51_Draw_Rivers;
//  int (__thiscall *m52_Draw_Roads)(Map_Renderer *this, int ImageIndex, int, int, int);
  void *m52_Draw_Roads;
//  void (__thiscall *m53_Draw_City_Ruins)(Map_Renderer *, int, int, int, int);
  void *m53_Draw_City_Ruins;
//  void (__thiscall *m54_null)(int, int, int, int, int, int, int);
  void *m54_null;
//  void (__thiscall *m55_Draw_Territory_Border)(Map_Renderer *, int, int, char, int, int, int, int, int);
  void *m55_Draw_Territory_Border;
//  int (__thiscall *m56_null)(int, int, int, int, int, int, int);
  void *m56_null;
//  void (__thiscall *m57_Draw_Radar)(Map_Renderer *, int, int, int, int, int, int);
  void *m57_Draw_Radar;
//  void (__thiscall *m58_Draw_LM_Terrain)(int, int);
  void *m58_Draw_LM_Terrain;
//  int (__thiscall *m59_Draw_Tnt_Row0)(Map_Renderer *, int, int, int, int, int);
  void *m59_Draw_Tnt_Row0;
//  int (__thiscall *m60_Draw_Row1)(Map_Renderer *, int, int, int, int);
  void *m60_Draw_Row1;
//  int (__thiscall *m61_Draw_Tnt_Row2)(Map_Renderer *, int, int, int, int, int);
  void *m61_Draw_Tnt_Row2;
//  int (__thiscall *m62_Draw_Tnt_Row3)(Map_Renderer *, int, int, int, int, int);
  void *m62_Draw_Tnt_Row3;
//  int (__thiscall *m63_Draw_Tnt_Row4)(Map_Renderer *, int, int, int, int, int);
  void *m63_Draw_Tnt_Row4;
//  int (__thiscall *m64_Draw_Tnt_Row5)(Map_Renderer *, int, int, int, int);
  void *m64_Draw_Tnt_Row5;
//  void (__thiscall *m65_null)(int, int, int, int, int, int, int);
  void *m65_null;
//  int (__thiscall *m66_Draw_Victory_Point)(Map_Renderer *, int, int, int, int, int, int);
  void *m66_Draw_Victory_Point;
  int m67;
//  void (__thiscall *m68_Toggle_Grid)(Map_Renderer *);
  void *m68_Toggle_Grid;
  int m69;
//  void (__thiscall *m70_Draw_Info_Dialog_Tile)(Map_Renderer *, int, int, int, int, int, int);
  void *m70_Draw_Info_Dialog_Tile;
//  void (__thiscall *m71_Draw_Tiles)(Map_Renderer *this, int CivID, PCX_Image *PCX_Image, int Arg5);
  void *m71_Draw_Tiles;
  int m72;
};

struct String260
{
  char S[260];
};

struct Tile_Image_Info
{
  Tile_Image_Info_vtable *vtable;
  JGL_Image_Info *JGL_Image_Info;
  int field_8;
  PCX_Color_Table *Color_Table;
  int Width3;
  int Height3;
  int Width;
  int Height;
  int Width2;
  int Height2;
  int field_28;
};

struct Tile_Image_Info_vtable
{
  int m00;
};

struct Tile_Image_Info_List
{
  Tile_Image_Info field_0[81];
};

struct Base_vtable
{
  int m00;
  int m01;
//  int (__thiscall *m02_Save_Data)(Base *, void *);
  void *m02_Save_Data;
//  int (__thiscall *m03_Read_Data)(Base *, void *);
  void *m03_Read_Data;
};

struct City_Screen_FileNames
{
  String260 CityIcons42;
  String260 PCX_Background;
  String260 DraftButton;
  String260 BottomFadeBar;
  String260 BottomFadeBarAlpha;
  String260 TopFadeBar;
  String260 TopFadeBarAlpha;
  String260 Govern;
  String260 HurryButton;
  String260 CityIcons;
  String260 LuxuryIconsSmall;
  String260 PopheadHilite;
  String260 MgmtButtons;
  String260 ProdButton;
  String260 ProductionQueueBar;
  String260 QueueBase;
  String260 ProductionQueueBox;
  String260 Labels;
  String260 XAndView;
  String260 XAndView2;
};

struct _188_48
{
  int *vtable;
  int Direction;
  int field_8;
  int field_C[4];
  int Direction2;
  int Animation_Type;
  int field_24;
  int field_28;
  int Last;
};

struct FLC_Frame_Image
{
  int vtable;
  Flic_Anim_Info *Flic_Info;
  Tile_Image_Info Frame_Image;
  int field_34;
  FLIC_CHUNK *Current_Frame_Chunk;
  int Current_Frame_ID;
  int Direction;
};

struct _188_4C
{
  int field_0[19];
};

struct Civilopedia_Article
{
  Civilopedia_Article_vtable *vtable;
  String64 Name;
  String64 Civilopedia_Entry;
  String64 PCX_Small;
  String64 PCX_Large;
  int b_Show_Description;
  int field_108;
  int Article_Type;
  int IconID;
  int field_114;
  int field_118;
  int field_11C;
  int field_120;
  int field_124;
  int field_128;
  int field_12C;
  int field_130;
  int field_134;
};

struct JGL_Renderer
{
  JGL_Image *Image;
  int field_8[171];
  int Last;
};

struct Buildings_Info_Item
{
  int Year;
  int CivID;
  int Culture;
};

struct Date
{
  Base Base;
  String16 Text;
  int field_2D[12];
  int BaseTimeUnit;
  int Month;
  int Week;
  int Year;
  int field_6C;
};

struct Sound_Info
{
  int vtable;
  int field_4[26];
};

struct Sound_Info_Array
{
  Sound_Info Items[63];
};

struct Citizen_Mood_Array
{
  int vtable;
  String260 Mood_Text[14];
  String260 PCX_Heads;
  String260 Labels_File;
  Tile_Image_Info Images[520];
};

struct Map_Body_vtable
{
  int m00;
  int m01;
  int m02;
  int m03;
  int m04;
};

struct Unit_vtable
{
  int m00;
  int m01;
  int m02;
//  int (__thiscall *m03_Read_Unit)(Unit *, int);
  void *m03_Read_Unit;
  int m04;
  int (__fastcall * eval_escort_requirement) (Unit *);
  int (__fastcall * eval_cost_per_defense) (Unit *);
  int (__fastcall * eval_total_defense) (Unit *);
  byte (__fastcall * has_enough_escorters_present) (Unit *);
  void (__fastcall * check_escorter_health) (Unit *, int, byte *, byte *);
  int m10;
  int m11;
  int m12;
  int (__fastcall * get_sea_id) (Unit *);
  byte (__fastcall * ai_is_good_army_addition) (Unit *, int, Unit *);
  byte (__fastcall * is_enemy_of_civ) (Unit *, int, int, byte);
  byte (__fastcall * is_enemy_of_unit) (Unit *, int, Unit *, int);
  int m17;
  int m18;
  int (__fastcall * Move) (Unit *, int, int, char);
  int m20;
  void (__fastcall * update_while_active) (Unit *);
  int m22;
  byte (__fastcall * work) (Unit *);
  int m24;
  int m25;
  int m26;
  int m27;
};

struct _188_vtable
{
  int m00;
//  void (__thiscall *m01)(FLC_Animation *this, float Timing);
  void *m01;
  int m02;
};

struct Advisor_Renderer
{
  int vtable;
  int field_4[2075];
  int field_2070;
};

struct Trade_Net
{
  int vtable;
  int Map_Width;
  int Current_Unit_X;
  int Current_Unit_Y;
  Unit *Current_Unit;
  int Current_Unit_CivID;
  int Flags;
  int *Data1;
  Trade_Net_Distance_Info *Data2;
  Trade_Net_Distance_Info *Data3;
  int V1;
  Trade_Net_Distance_Info *Data4;
  Trade_Net_Distance_Info *Data5;
  int V2;
  int Matrix[262144];
};

struct Map_Worker_Job_Info
{
  int Set_Overlays_Flags;
  int Unset_Overlays_Flags;
  int field_8;
  int ID;
  int field_10;
};

struct Airfield_Body
{
  int vtable;
  int field_4;
  int field_8;
  int field_C;
  int field_10;
  int field_14;
};

struct Colony_Body
{
  int vtable;
  int field_4;
  int field_8;
  int field_C;
  int field_10;
};

struct Radar_Tower_Body
{
  int vtable;
  int field_4;
  int field_8;
  int field_C;
  int field_10;
};

struct Outpost_Body
{
  int vtable;
  int field_4;
  int field_8;
  int field_C;
  int field_10;
};

struct Victory_Point_Body
{
  int vtable;
  int field_4;
  int tile_x;
  int tile_y;
  int field_10;
};

struct Victory_Point
{
  Base Base;
  Victory_Point_Body Body;
};

struct Tile_Building_Body
{
  int vtable;
  int ID;
  int X;
  int Y;
  int OwnerID;
};

struct Tile_Building
{
  Base Base;
  Tile_Building_Body Body;
};

struct PCX_Color_Table
{
  int vtable;
  JGL_Color_Table *JGL_Color_Table;
  int field_8[5];
  int field_1C[14];
  int Last;
};

struct Tile_Type_Flags
{
  char Allow_Cities;
  char Allow_Colonies;
  char Impassable;
  char Impassable_By_Wheeled_Units;
  char Allow_Airfields;
  char Allow_Forts;
  char Allow_Outposts;
  char Allow_Radar_Towers;
};

struct String264
{
  char S[264];
};

struct String256
{
  char S[256];
};

struct City_Icon_Images
{
  Tile_Image_Info Icon_00;
  Tile_Image_Info Icon_01;
  Tile_Image_Info Icon_02_Science;
  Tile_Image_Info Icon_03;
  Tile_Image_Info Icon_04;
  Tile_Image_Info Icon_05_Shield_Outcome;
  Tile_Image_Info Icon_06;
  Tile_Image_Info Icon_07;
  Tile_Image_Info Icon_08;
  Tile_Image_Info Icon_09;
  Tile_Image_Info Icon_10;
  Tile_Image_Info Icon_11_Pollution;
  Tile_Image_Info Icon_12_Happy_Faces;
  Tile_Image_Info Icon_13_Shield;
  Tile_Image_Info Icon_14_Gold;
  Tile_Image_Info Icon_15_Food;
  Tile_Image_Info Icon_16_Science;
  Tile_Image_Info Icon_17_Gold_Outcome;
  Tile_Image_Info Icon_18_Culture;
  Tile_Image_Info Icon_19_Happy_Faces;
  Tile_Image_Info Icon_20_Shield_Highlighted;
  Tile_Image_Info Icon_21_Gold_Highlighted;
  Tile_Image_Info Icon_22_Food_Highlighted;
  Tile_Image_Info Icon_23;
  Tile_Image_Info Icon_24_Treasury;
};

struct Map_Cursor_Images
{
  Tile_Image_Info C00;
  Tile_Image_Info C01;
  Tile_Image_Info C02;
  Tile_Image_Info C03;
  Tile_Image_Info C04;
  Tile_Image_Info C05;
  Tile_Image_Info C06;
  Tile_Image_Info Bombing;
  Tile_Image_Info C08;
  Tile_Image_Info Recon;
  Tile_Image_Info C10;
  Tile_Image_Info Precision_Bombing;
  Tile_Image_Info C12;
  Tile_Image_Info C13;
  Tile_Image_Info C14;
  Tile_Image_Info C15;
  Tile_Image_Info Auto_Air_Bombard;
  Tile_Image_Info C17;
  Tile_Image_Info C18;
  Tile_Image_Info C19;
  Tile_Image_Info C20;
  Tile_Image_Info C21;
  Tile_Image_Info C22;
};

struct Old_Interface_Images
{
  Tile_Image_Info Images[3];
  int Last;
};

struct Main_Screen_Form_vtable
{
  int m00;
};

struct GUI_Data_30
{
  int vtable;
  int field_4[11];
};

struct Main_Screen_Data_170
{
  int vtable;
  int field_4[90];
  int Last;
};

struct Form_Data_30
{
  int vtable;
  int field_4[11];
};

struct GUI_Form_1_vtable
{
  int m00;
  int m01;
  int m02;
  int m03;
  int m04;
  int m05;
  int m06;
  int m07;
  int m08;
  int m09;
  int m10;
  int m11;
  int m12;
  int m13;
  int m14;
  int m15;
  int m16;
  int m17;
  int m18;
  int m19;
  int m20;
  int m21;
  int m22;
  int m23;
  int m24;
  int m25;
  int m26;
  int m27;
  int m28;
  int m29;
  int m30;
  int m31;
  int m32;
  int m33;
  int m34;
  int m35;
  int m36;
  int m37;
  int m38;
  int m39;
  int m40;
  int m41;
  int m42;
  int m43;
  int m44;
  int m45;
  int m46;
  int m47;
  int m48;
  int m49;
  int m50;
  int m51;
  int m52;
  int m53;
  int m54;
  int m55;
  int m56;
  int m57;
  int m58;
  int m59;
  int m60;
  int m61;
  int m62;
  int m63;
  int m64;
  int m65;
  int m66;
  int m67;
  int m68;
  int m69;
  int m70;
  int m71;
  int m72;
  int m73;
  int m74;
  int m75;
  int m76;
  int m77;
  int m78;
  int m79;
  int m80;
  int m81;
  int m82;
  int m83;
  int m84;
  int m85;
  int m86;
  int m87;
  int m88;
  int m89;
  int m90;
  int m91;
//  void (__thiscall *m92_Init_Dialog)(void *, char *, char *, int, int, int, int);
  void *m92_Init_Dialog;
  int m93;
  int m94;
  int m95;
  int m96;
  int m97;
  int m98;
};

struct Base_Form_vtable
{
  Base_Form * (__fastcall * destruct) (Base_Form *, __, byte);
  void (__fastcall * m01_Show_Enabled) (Base_Form *, __, byte);
  void (__fastcall * m02_Show_Disabled) (Base_Form *);
  int m03;
  int m04;
  int m05;
  int m06;
  int m07;
  int m08;
  int m09;
  int m10;
  int m11;
  int m12;
  int m13;
  int m14;
  int m15;
  int m16;
  int m17;
  int m18;
  int m19;
  int m20;
  int m21;
  void (__fastcall * m22_Draw) (Base_Form *);
  int m23;
  int m24;
  int m25_Process_Mouse_Wheel;
  void (__fastcall * m26_on_mouse_hover) (Base_Form * this, __, int local_x, int local_y, int param_3);
  int m27;
  int m28;
//  void (__thiscall *m29_On_Left_Click)(Base_Form *, int, int);
  void *m29_On_Left_Click;
  void (__fastcall * m30_Process_Left_Click) (Base_Form *, __, int, int);
  int m31;
  int m32_On_Right_Click;
  int m33_Process_Right_Click;
  int m34;
  int (__fastcall * m35_handle_key_down) (Base_Form *, __, int, int);
  int m36;
  int m37;
  int m38_On_Double_Click;
  int m39;
  int m40;
  int m41;
  int m42;
  int m43;
  int m44;
  int m45;
  int m46;
  int m47;
  int m48;
  int m49;
  int m50;
  int m51;
  int m52;
  int m53_On_Control_Click;
  int m54;
  int m55;
  int m56;
  int m57;
  int m58;
  int m59;
  int m60;
  int m61;
  int m62;
  int m63;
  int m64;
  int m65;
  int m66;
  int m67;
  int (__fastcall * m68_Show_Dialog) (Base_Form *, __, int, void *, void *);
  void (__fastcall * m69_Close_Dialog) (Base_Form *);
  int m70;
  int m71;
  int m72;
  void (__fastcall * m73_call_m22_Draw) (Base_Form *);
  int m74;
  void (__fastcall * m75) (Base_Form *);
  int m76;
  int m77;
  int m78;
  int m79;
  int m80;
  int m81;
  void (__fastcall * m82_handle_key_event) (Base_Form *, __, int, int);
  int m83_On_Char;
  void (__fastcall * m84_On_Mouse_Left_Down) (Base_Form *, __, int, int, int, int);
  void (__fastcall * m85_On_Mouse_Left_Up) (Base_Form *, __, int, int, int);
//  void (__thiscall *m86_On_Mouse_Right_Down)(Base_Form *, int, int, int, int);
  void *m86_On_Mouse_Right_Down;
//  void (__thiscall *m87_On_Mouse_Right_Up)(Base_Form *this, int, int, int);
  void *m87_On_Mouse_Right_Up;
  int m88;
  int m89;
  int m90;
  int m91;
};

struct Control_Data_Offsets
{
  int V1;
  int Control;
  int List;
};

struct Context_Menu_Item
{
  char *Text;
  int field_4;
  int Menu_Item_ID;
  int Status;
  int field_10;
  Tile_Image_Info *Image;
  int field_18;
};

struct Civilopedia_Article_vtable
{
  int m00_ctor;
//  void (__thiscall *m01_Draw_Article)(Civilopedia_Article *);
  void *m01_Draw_Article;
  int m02;
  int m03_Draw_Large_Icon;
  int m04_Draw_Small_Icon;
  int m05;
};

struct Palace_View_Info
{
  Base Base;
  int field_1C;
  int Items_Cultures[32];
  int field_A0;
  int Builds_Done;
  int Items_Flags;
  int Builds_Remained;
};

struct Text_Reader
{
  int vtable;
  String260 Str1;
  String260 Str2;
  char *Buffer0;
  FILE *File;
  char *Buffer1;
  char *Buffer2;
};

struct Button_Images_3
{
  Tile_Image_Info Images[3];
};

struct Point
{
  int X;
  int Y;
};

struct Game_Version
{
  int vtable;
  int field_4[7];
  int Last;
};

struct History
{
  Base Base;
  int Item_Count;
  int field_20;
  History_Item *First_Item;
  History_Item *Last_Item;
  int field_2C;
  int field_30;
  int field_34;
  int field_38;
  int field_3C;
};

struct History_Item
{
  int vtable;
  History_Item *Next;
  int field_8;
  int Date;
  int field_10;
  int field_14;
  int field_18;
  int field_1C;
  int field_20;
  int field_24;
};

struct Combat_Experience
{
  int field_0;
  String32 Name;
  int Base_Hit_Points;
  int Retreat_Bonus;
};

struct _12C
{
  int field_0[75];
};

struct Leader_vtable
{
  int m00;
  int m01;
  int m02;
  int m03;
  int m04;
  int m05;
  byte (__fastcall * would_raze_city) (Leader * this, int edx, City * city);
  int (__fastcall * ai_find_best_government) (Leader * this);
  int m08;
  int m09;
  int m10;
  void (__fastcall * ai_adjust_sliders) (Leader * this);
  int m12;
  Unit * (__fastcall * find_unsupported_unit) (Leader * this);
  int m14;
  int m15;
  int m16;
  int m17;
  int m18;
  int m19;
  int m20;
  int m21;
  int m22;
  int m23;
  int m24;
  int m25;
  int m26;
  int m27;
//  void (__thiscall *m28)(Leader *);
  void *m28;
  int m29;
  int m30;
  int m31;
  int m32;
  int m33;
  int m34;
  int m35;
  int m36;
  int m37;
  int m38;
  int m39;
  int m40;
  int m41;
  int m42;
};

struct Flic_Anim_Info
{
  FLC_Frame_Image Frame;
  FLIC_HEADER *Flic_Anim_Data;
  FLC_Direction_Start_Frames *Direction_Frames;
  FLIC_CHUNK **Delta_Frames;
  int field_50;
  int p_1;
  FLIC_HEADER *FLC_Data_2;
  int field_5C;
  int field_60;
  int field_64;
  int field_68;
  int field_6C;
  int field_70;
  PCX_Color_Table *ColorTable1;
  PCX_Color_Table *Color_Table;
  int FLC_File_Size;
  int Last;
};

struct Game_Limits
{
  int Turns_Limit;
  int Time_Limit;
  int Points_Limit;
  int Destroyed_Cities_Limit;
  int City_Culture_Limit;
  int Civ_Culture_Limit;
  int Population_Limit;
  int Territory_Limit;
  int Wonders_Limit;
  int Destroyed_Units_Limit;
  int Advances_Limit;
  int Captured_Cities_Limit;
  int Victory_Point_Price;
  int Princess_Price;
  int Princess_Ransom;
};

struct Default_Game_Limits
{
  int Turns_Limit;
  int Points_Limit;
  int Destroyed_Cities_Limit;
  int City_Culture_Limit;
  int Civ_Culture_Limit;
  int Population_Limit;
  int Territory_Limit;
  int Wonders_Limit;
  int Destroyed_Units_Limit;
  int Advances_Limit;
  int Captured_Cities_Limit;
  int Victory_Point_Price;
  int Princess_Price;
  int Princess_Ransom;
};

struct Campain_Records_Labels
{
  String260 field_0;
  String260 field_104;
  String260 field_208;
  String260 field_30C;
  String260 field_410;
  String260 field_514;
  String260 field_618;
  String260 field_71C;
  String260 field_820;
  String260 field_924;
  String260 field_A28[9];
};

struct City_Form_Labels
{
  char * Strategic_Resources;
  char * Improvements;
  char * Production;
  char * Food;
  char * Trade_Income;
  char * Luxury;
  char * Culture;
  char * Pollution;
  char * Per_Turn;
  char * To_Growth;
  char * To_Build;
  char * Single_Turn;
  char * Multiple_Turns;
  char * Percent;
  char * Edit;
  char * Hurry;
  char * Granary;
  char * Deficit;
  char * Zero_Income;
  char * Year_Of_Found;
  char * Population;
  char * Currency_Name;
  char * Per_Turn2;
  char * Total;
  char * Orders_Queue;
  char * Insert_Orders;
  char * Empty_Order;
  char * City_Units;
  char * Activate;
  char * Fortify;
  char * Delete_Unit;
  char * Upgdate_Unit;
};

struct Unit_Move_Target
{
  int X;
  int Y;
  int Unit_CivID;
  int Bulding_OwnerID;
  int CivID;
};

struct Hash_Table
{
  Hash_Table_vtable *vtable;
  Hash_Table_Item *Items;
  int Key_Count;
  int Capacity;
  int Hash_Bits;
};

struct Hash_Table_vtable
{
  int m00;
  void (__fastcall * Init_Items) (Hash_Table *);
  void (__fastcall * Enlarge_Items) (Hash_Table *);
  int m03;
  char (__fastcall * Is_Capacity_Low) (Hash_Table *);
  int (__fastcall * is_capacity_high) (Hash_Table *);
  unsigned (__fastcall * Get_Key_Index) (Hash_Table *, __, unsigned);
};

struct Hash_Table_Item
{
  int Key;
  int Value;
};

struct Leader_Data_10
{
  int vtable;
  int field_4;
  int field_8;
  int field_C;
};

struct Item_List2
{
  int vtable;
  int field_4[61];
  int Last;
};

struct Culture
{
  Base Base;
  int field_1C;
  int field_20;
  int field_24;
  int CivID;
};

struct Espionage
{
  Base Base;
  int field_1C;
  int field_20;
  int field_24;
  int Type;
  int field_2C;
  int field_30;
  int field_34;
  int field_38;
};

struct CTPG
{
  Base Base;
  int field_1C;
  int field_20;
  int field_24;
  int field_28;
  int field_2C;
};

struct Default_Game_Settings
{
  int Top_Menu;
  int Preferences;
  int Difficulty;
  int Rules;
  int Factions[31];
  Default_Game_Limits Game_Limits;
  int Aggression;
  int Main_Volume;
  int Sound_Volume;
  int Music_Volume;
  int Clean_Map;
  int Governor_Common;
  int Governor_Prod_Never;
  int Governor_Prod_Often;
  char Custom_Leader_Name[32];
  char Custom_Leader_Title[24];
  char Custom_Formal_Name[40];
  char Custom_Noun[40];
  char Custom_Adjective[40];
  int Custom_Gender;
  int Unknown_Value;
  int World_Aridity;
  int Barbarian_Activity;
  int World_Landmass;
  int World_Ocean_Coverage;
  int World_Temperature;
  int World_Age;
  int World_Size;
  int World_Seed;
  char World_Seed_Name[12];
  int Actual_Word_Aridity;
  int Actual_Barbarian_Activity;
  int Actual_World_Landmass;
  int Actual_World_Ocean_Coverage;
  int Actual_World_Temperature;
  int Actual_World_Age;
  int Actual_World_Size;
  int Actual_Civ_Array[31];
  int Seafaring;
};

struct ComboBox_Text_Control
{
  int vtable;
  int field_4[352];
  int field_584;
};

struct Replay_Turn
{
  Base Base;
  int Turn;
  int field_20;
  int field_24;
  Replay_Entry_Node *Node;
  int field_2C;
  int Data_Ptr1;
};

struct Replay_Turn_Node
{
  Replay_Turn_Node *Node1;
  Replay_Turn_Node *Node2;
  Replay_Turn *Turn;
};

struct Replay_Entry
{
  Base Base;
  char Type;
  char CivID;
  short X;
  short Y;
  char field_22;
  char field_23;
  char *Text;
};

struct Replays_Body
{
  int field_0;
  Replay_Turn_Node *Node1;
  int Value1;
  Replay_Turn_Node *Node2;
  int field_10;
  int field_14;
  int field_18;
  int field_1C;
  int field_20;
};

struct Replay_Entry_Node
{
  Replay_Entry_Node *Node1;
  Replay_Entry_Node *Node2;
  Replay_Entry *Entry;
};

struct World_Features
{
  int World_Aridity;
  int Final_World_Aridity;
  int Barbarian_Activity;
  int Final_Barbarians_Activity;
  int World_Landmass;
  int Final_World_Landmass;
  int Ocean_Coverage;
  int Final_Ocean_Coverage;
  int World_Temperature;
  int Final_World_Temperature;
  int World_Age;
  int Final_World_Age;
  int World_Size;
  int Continents_Data;
  int Tiles_Data;
};

struct Starting_Location
{
  int Owner_Type;
  int Owner_ID;
  int X;
  int Y;
};

struct Scenario_Player
{
  int field_0[13];
  int *Starting_Units;
  int field_38;
  int Free_Techs;
  int field_40;
  int field_44;
  int Starting_Treasury;
  int Government_ID;
  int Free_Techs_Count;
  int Starting_Units_Count;
  int field_58;
  int field_5C;
  int field_60;
  int field_64;
  int Last;
};

struct Navigator_Cell
{
  short X;
  short Y;
  int field_4;
  Navigator_Point **First_Point;
  Navigator_Point **Last_Point;
  int field_10;
  int field_14;
};

struct Navigator_Point
{
  short X;
  short Y;
};

struct JGL_Image
{
  JGL_Image_vtable *vtable;
  int field_4;
  int field_8;
  RTL_CRITICAL_SECTION Critical_Section;
  int BitCount;
  int field_28[6];
  int Width2;
  RECT Clip_Rect;
  RECT Image_Rect;
  int field_64[6];
  int field_7C;
  PCX_Image *PCX;
  BITMAPINFO Bitmap_Info;
  int field_B0[255];
  HRGN hRegion;
  HBITMAP Prev_hBitmap;
  HBITMAP hBitmap;
  HDC Current_DC;
  HDC DC;
  void *Bits_Data;
  int DC_Links;
  int Bits_Data_Links;
  int Current_Bits_Data;
  int field_4D0;
};

struct JGL_Image_vtable
{
  int m00;
  int m01;
//  void (__thiscall *m02_Clear_DC)(JGL_Image *);
  void *m02_Clear_DC;
//  char *(__thiscall *m03_Get_Pixel_Data)(JGL_Image *this, int, int);
  void *m03_Get_Pixel_Data;
//  void *(__thiscall *m04_Get_Bits_Data)(JGL_Image *this);
  void *m04_Get_Bits_Data;
//  char *(__thiscall *m05_m07_Get_Pixel)(JGL_Image *this, int X, int Y);
  void *m05_m07_Get_Pixel;
  int m06;
//  char *(__thiscall *m07_m05_Get_Pixel)(JGL_Image *this, int X, int Y);
  void *m07_m05_Get_Pixel;
  int m08_Get_Bits_Data;
  int m09;
//  HDC (__thiscall *m10_Get_DC)(JGL_Image *);
  void *m10_Get_DC;
//  void (__thiscall *m11_Release_DC)(JGL_Image *, int);
  void *m11_Release_DC;
  int m12;
  int m13_Set_Clip_Region;
  int m14;
  int m15;
//  int (__thiscall *m16_Copy_To_Dest)(JGL_Image *this, JGL_Image *Dest, RECT *SrcRect, RECT *DestRect);
  void *m16_Copy_To_Dest;
  int m17_Register_Rect;
  int m18;
  int m19;
  int m20;
  int m21;
  int m22;
  int m23;
  int m24;
  int m25;
  int m26;
  int m27;
  int m28;
  int m29;
  int m30;
  int m31;
  int m32;
  int m33;
  int m34;
  int m35;
  int m36;
  int m37;
  int m38;
  int m39;
  int m40;
  int m41;
  int m42;
  int m43;
  int m44;
  int m45;
  int m46;
  int m47;
  int m48_Get_Field_4D0;
  int m49_Clip_Full_Image;
  int m50_Get_Clip_Rect;
  int m51;
  int m52_Get_Image_Rect;
  int m53;
//  int (__thiscall *m54_Get_Width)(JGL_Image *this);
  void *m54_Get_Width;
//  int (__thiscall *m55_Get_Height)(JGL_Image *this);
  void *m55_Get_Height;
  int m56;
//  int (__thiscall *m57_Get_Bit_Count)(JGL_Image *);
  void *m57_Get_Bit_Count;
  int m58;
  int m59_Set_Color_Table;
};

struct JGL_Graphics
{
  JGL_Graphics_vtable *vtable;
  int field_4[81];
  int Last;
};

struct Window_Controller
{
  Window_Controller_vtable *vtable;
};

struct Window_Controller_vtable
{
  int m00;
  int m01;
  int m02;
  int m03;
  int m04;
  int m05;
  int m06;
  int m07;
  int m08;
  int m09;
  int m10;
  int m11;
  int m12;
  int m13;
};

struct JGL_Image_Info
{
  JGL_Image_Info_vtable *vtable;
  int field_4;
  int field_8;
  int field_C;
  int field_10;
  void *Bits_Data;
  int field_18;
  int field_1C;
  int BitCount;
  int field_24;
  int field_28;
  int Width2;
  int Width;
  int Height;
  RTL_CRITICAL_SECTION Critical_Section;
};

struct JGL_Color_Table_vtable
{
  int m00;
  int m01;
  int m02;
  int m03;
  int m04_Get_Palette_Colors;
  int m05_Set_Palette_Colors;
  int m06;
  int m07;
  int m08_Get_Palette_Color;
};

struct JGL_Color_Table
{
  JGL_Color_Table_vtable *vtable;
  int field_4;
  int field_8;
  int field_C[512];
  int PCX_Color_Table;
  int field_810;
  int field_814;
  int field_818;
  int field_81C;
};

struct JGL_Image_Info_vtable
{
  int m00;
  int m01;
  int m02;
  int m03;
  int m04_Set_Color_Table;
  int m05;
  int m06;
  int m07;
  int m08;
  int m09;
  int m10;
  int m11;
  int m12;
  int m13;
  int m14;
  int m15;
  int m16;
  int m17;
  int m18;
  int m19;
  int m20;
  int m21;
  int m22;
  int m23;
  int m24;
  int m25;
  int m26;
  int m27;
  int m28;
  int m29;
  int m30;
  int m31;
  int m32;
  int m33;
  int m34;
  int m35;
  int m36;
  int m37;
  int m38;
  int m39;
  int m40;
};

struct Units_Image_Data
{
  int vtable;
  PCX_Color_Table *Color_Tables[32];
  float Timing;
  float Timing2;
  int field_8C;
  void *field_90;
  int field_94;
  int field_98;
  void *Node;
  int Node_Count;
};

struct Animation_Info
{
  Animation_Info_vtable *vtable;
  int Data;
  int Color_Table;
  int field_C[33];
  int field_90;
  int field_94;
  int field_98[41];
  Flic_Anim_Info **Animations;
  int field_140[17];
  int field_184[21];
  float *field_1D8;
  int field_1DC;
  int field_1E0;
  int field_1E4;
  int *Frame_Counts;
  int field_1EC[8];
  int field_20C;
  int field_210;
  int field_214;
  int field_218;
  int Last;
};

struct Animation_Info_vtable
{
  int m00_Clear;
  int m01;
  int m02_Load_INI;
  int m03;
  int m04;
  int m05;
  int m06;
  int m07_Tile_m39;
};

struct Animation_Data_60
{
  Animation_Data_60_vtable *vtable;
  int field_4;
  int field_8[12];
  int Animation_Time;
  int field_3C;
  int Ptr1;
  int field_44;
  int Prev;
  int Next;
  int field_50;
  int field_54;
  int field_58;
  int field_5C;
};

struct Animation_Data_60_vtable
{
  int m00;
  int m01;
  int m02;
  int m03;
  int m04;
  int m05;
  int m06;
  int m07;
  int m08;
  int m09;
  int m10;
  int m11;
  int m12;
  int m13;
  int m14;
  int m15;
  int m16;
  int m17;
  int m18;
  int m19;
  int m20;
  int m21;
  int m22;
  int m23;
  int m24;
  int m25;
  int m26;
  int m27;
  int m28;
  int m29;
  int m30;
  int m31;
  int m32;
  int m33;
  int m34;
  int m35;
  int m36;
  int m37;
  int m38;
  int m39;
  int m40;
  int m41;
  int m42;
  int m43;
  int m44;
  int m45;
  int m46;
  int m47;
  int m48;
  int m49;
  int m50;
  int m51;
  int m52;
  int m53;
  int m54;
  int m55;
  int m56;
  int m57;
  int m58;
  int m59;
  int m60;
  int m61;
  int m62;
  int m63;
  int m64;
  int m65;
  int m66;
  int m67;
  int m68;
  int m69;
  int m70;
  int m71;
  int m72;
  int m73;
  int m74;
  int m75;
  int m76;
  int m77;
};

struct IDLS
{
  Base Base;
  IntList escorters;
};

struct FLIC_HEADER
{
  int size;
  short type;
  short frames;
  short width;
  short height;
  short depth;
  short flags;
  int speed;
  short reserved1;
  int created;
  int creator;
  int updated;
  int updater;
  short aspect_dx;
  short aspect_dy;
  short ext_flags;
  short keyframes;
  short totalframes;
  int req_memory;
  short max_regions;
  short transp_num;
  unsigned char reserved2[24];
  int oframe1;
  int oframe2;
  int field_58;
  int field_5C;
  short Total_Directions;
  short Frames_Per_Direction;
  short Left;
  short Top;
  short Original_Width;
  short Original_Height;
  int Animation_Time;
  int field_70;
  int field_74;
  int field_78;
  int field_7C;
};

struct FLIC_CHUNK_HEADER
{
  int size;
  short type;
};

struct FLIC_FRAME_HEADER
{
  FLIC_CHUNK_HEADER Header;
  short chunks;
  short delay;
  short reserved;
  short width;
  short height;
};

struct FLIC_COLOR_256_HEADER
{
  FLIC_CHUNK_HEADER Header;
  short packets;
  char skip_count;
  char copy_count;
};

union FLIC_CHUNK
{
  FLIC_CHUNK_HEADER Chunk;
  FLIC_FRAME_HEADER Frame;
  FLIC_COLOR_256_HEADER Color_256;
};

struct FLC_Direction_Start_Frames
{
  FLIC_CHUNK *Frames[8];
};

struct Animation_Node
{
  Animation_Node *Prev;
  Animation_Node *Next;
  Animation_Info *Info;
};

struct Era_Type
{
  int V;
  String64 Name;
  String32 Civ_Entry;
  String32 Reseachers_Names[5];
  int V1;
  int V2;
};

struct Civ_Treaties
{
  int vtable;
  int Count;
  Civ_Treaty *First;
  Civ_Treaty *Last;
};

struct Civ_Treaty
{
  int vtable;
  int field_4;
  int field_8;
  int field_C;
  Civ_Treaty *Next;
  Civ_Treaty *Prev;
};

struct Difficulty_Level
{
  int V;
  String64 Name;
  int Content_Citizens;
  int Transition_Time;
  int Defencive_Land_Units;
  int Offencive_Type_Units;
  int Start_Units_1;
  int Start_Units_2;
  int Additional_Free_Support;
  int Bonus_For_Each_City;
  int Attack_Bonus;
  int Cost_Factor;
  int Optimal_Cities;
  int AI_Trade_Rate;
  int Corruption_Level;
  int Quelled_Citizens;
};

struct Control_Tooltips
{
  int vtable;
  int field_4;
  int field_8;
  int field_C;
  int field_10[4];
  Form_Data_30 Data_30;
  Control_Tooltip *Items;
  int Capacity;
  int Count;
  int field_5C;
};

struct Control_Tooltip
{
  int vtable;
  int Left;
  int Top;
  int Right;
  int Bottom;
  int field_14;
  int ID;
  char *Tooltip;
};

struct City_Order
{
  int OrderID;
  int OrderType;
};

struct Trade_Net_Distance_Info
{
  short field_0;
  char Remained_Moves;
  char Direction;
  short field_4;
  short Moves;
  short field_8;
  short field_A;
  short Tile_Index;
  short Radius_Neighbour_Index;
};

struct City_Form_vtable
{
  Base_Form_vtable m00;
};

struct Reputation
{
	int field_0;
	int cancelled_deal;
	int field_8;
	int field_C;
	int caught_spy;
	int field_14;
	int tribute;
	int field_1C;
	int field_20;
	int gift;
	int field_28;
	int icbm;
	int icbm_other;
	int recent_ww_given;
	int recent_ww_taken; // war weariness inflicted on us (= Leader object) by this player (= index in the reputations array) during the last turn
	int field_3C[4];
};

struct Leader
{
  Leader_vtable *vtable;
  int field_4[6];
  int ID;
  int RaceID;
  int field_24[2];
  int CapitalID;
  int field_30;
  int field_34;
  int field_38;
  int Golden_Age_End;
  int Status;
  int Gold_Decrement;
  int Gold_Encoded;
  int field_4C[20];
  int anarchy_turns_remaining;
  int GovernmentType;
  int Mobilization_Level;
  int Tiles_Discovered;
  int field_AC[14];
  int field_E4;
  int field_E8[3];
  int Era;
  int Research_Bulbs;
  int Current_Research_ID;
  int Current_Research_Turns;
  int Future_Techs_Count;
  short AI_Strategy_Unit_Counts[20];
  int field_130[22];
  int Armies_Count;
  int Unit_Count;
  int Military_Units_Count;
  int Cities_Count;
  int field_198;
  int field_19C;
  int field_1A0;
  int luxury_slider;
  int science_slider;
  int gold_slider;
  Reputation reputations[32];
  int field_B30[96];
  int war_weariness[32];
  char At_War[32];
  char field_D50[32];
  char field_D70[32];
  int field_D90[40];
  int gpt_to_other_civs[32];
  int Contacts[32];
  int Relation_Treaties[32];
  int Military_Allies[32];
  int Trade_Embargos[32];
  int field_10B0[18];
  int Color_Table_ID;
  int field_10FC;
  int field_1100[7];
  int field_111C[36];
  int field_11AC[8];
  int field_11CC;
  int field_11D0[252];
  int field_15C0;
  int field_15C4;
  int field_15C8;
  int field_15CC;
  int Science_Age_Status;
  int Science_Age_End;
  int field_15D8;
  short *Improvement_Counts;
  int field_15E0;
  int Improvements_Counts;
  int *Small_Wonders;
  int field_15EC;
  short *Units_Counts;
  int field_15F4;
  int field_15F8;
  short *Spaceship_Parts_Count;
  int *ContinentStat4;
  int ContinentStat3;
  int *ContinentCities;
  int ContinentStat2;
  int *ContinentStat1;
  unsigned char *Available_Resources;
  unsigned char *Available_Resources_Counts;
  Civ_Treaties Treaties[32];
  Culture Culture;
  Espionage Espionage_1;
  Espionage Espionage_2;
  int field_18C0[260];
  Leader_Data_10 Data_10_Array2[32];
  Leader_Data_10 Data_10_Array3[32];
  Hash_Table Auto_Improvements;
};

struct Government
{
  int vtable;
  int field_4;
  int field_8;
  int b_Default_Type;
  int b_Transition_Type;
  int b_Requires_Maintenance;
  int b_Flag1;
  int b_Standard_Tile_Penalty;
  int b_Standard_Trade_Bonus;
  int b_Xenophobic;
  int b_Force_Resettlement;
  String64 Name;
  char Civilipedia_Entry[32];
  RulerTitle RulerTitles[4];
  enum CorruptionAndWasteTypes CorruptionAndWaste;
  int ImmuneTo;
  int Diplomats_Experience;
  int Spies_Experience;
  void *PropagandaData;
  int HurryProduction;
  int AssimilationChange;
  int DratfLimit;
  int Military_Police_Limit;
  int field_1B0;
  int Ruler_Titles_Count;
  int RequireID;
  int RateCap;
  int WorkRate;
  int field_1C4;
  int field_1C8;
  int field_1CC;
  int All_Units_Free;
  int City_Class_Free_Units[3];
  int Unit_Support_Cost;
  int Last;
};

struct Tile_Body
{
  Base Base;
  int Fog_Of_War;
  int FOWStatus;
  int V3;
  int Visibility;
  int CircuitFlags;
  short CityAreaID;
  short Values[32];
  char Visibile_Overlays[32];
  short field_92;
  int field_D0_Visibility;
  int field_D4;
  _1A4 *field_D8;
};

struct Race
{
  Race_vtable *vtable;
  int field_4[4];
  String24 *CityNames;
  String32 *MilitaryLeaders;
  char LeaderName[32];
  char Title[24];
  char Civilopedia_Entry[32];
  char AdjectiveName[40];
  char CountryName[40];
  char SingularName[40];
  RaceEraAnimations EraAnimations[4];
  int CultureGroupID;
  int LeaderGender;
  int CivGender;
  int AggressionLevel;
  int ID;
  int ShunnedGovernment;
  int FavoriteGovernment;
  int CityNamesCount;
  int MilitaryLeadersCount;
  int DefaultColor;
  int AlternateColor;
  int StartupAdvances[4];
  int Bonuses;
  int GovernorSettings;
  int BuildNever;
  int BuildOften;
  int Plurality;
  int LeaderUnitID;
  int Flavours;
  int field_964;
  int DiplomacyTextIndex;
  int ScientificLeadersCount;
  int ScientificLeaders;
};

struct City_Body
{
  void *Body_vtable;
  int ID;
  short X;
  short Y;
  char CivID;
  char field_D;
  char field_E;
  char field_F;
  int Improvements_Maintenance;
  int Status;
  int Common_Policy;
  int Production_Policy;
  int Production_Policy_Often;
  int StoredFood;
  int StoredProduction;
  int Improvements_Pollution;
  int Order_ID;
  int Order_Type;
  int field_38[6];
  int DraftCount;
  int field_70[11];
  int Available_Resources;
  int field_84;
  int field_A4;
  Buildings_Info Buildings;
  Citizens Citizens;
  int field_F4[9];
  Population Population;
  int CultureIncome;
  int Total_Cultures[32];
  int field_1A4;
  int Rioting_Change_Value;
  int Tiles_Food;
  int Tiles_Production;
  int Tiles_Commerce;
  int field_1B8;
  int rally_point_x;
  int rally_point_y;
  char CityName[20];
  int field_1D8;
  int Order_Queue_Count;
  City_Order Orders_Queue[9];
  int FoodRequired;
  int ProductionLoss;
  int Corruption;
  int FoodIncome;
  int ProductionIncome;
  int CashIncome;
  int Luxury;
  int Science;
  int AddCash;
  int AddLuxury;
  int AddScience;
  int AddTaxes;
  City_Improvements Improvements_1;
  City_Improvements Improvements_2;
  Date Found_Date;
  int field_350;
  int field_354;
  int Improvement_Count;
  int field_35C;
  int *Unit_Produce_Times;
  int field_364;
  int field_368;
  CTPG CTPG;
  int Current_Improvement_Shields;
  int struct_198;
  int field_3A4[96];
  int Last;
};

struct Continent
{
  Base Base;
  Continent_Body Body;
};

struct Map_Renderer
{
  Map_Renderer_vtable *vtable;
  int field_4[173];
  int MapGrid_Flag;
  int b_Action_Mode;
  int field_2C0;
  int field_2C4;
  int field_2C8;
  int field_2CC;
  int Action_Mode_Range;
  int Action_Mode_CenterX;
  int Action_Mode_CenterY;
  int Flags;
  Map *Map;
  int field_2E4[50];
  String260 PCX_Airfields_and_Detect;
  String260 PCX_Airfields_and_Detect_Shadow;
  String260 PCX_Flood_Plains;
  String260 PCX_Fog_of_War;
  String260 PCX_Grassland_Forests;
  String260 PCX_Plains_Forests;
  String260 PCX_Tundra_Forests;
  String260 PCX_Goody_Huts;
  String260 PCX_Hills;
  String260 PCX_Hills_Forests;
  String260 PCX_Hills_Jungles;
  String260 PCX_Irrigation_Desert;
  String260 PCX_Irrigation;
  String260 PCX_Irrigation_Plains;
  String260 PCX_Irrigation_Tundra;
  String260 PCX_Mountains;
  String260 PCX_Mountain_Forests;
  String260 PCX_Mountain_Jungles;
  String260 PCX_Mountains_Show;
  String260 PCX_LM_Hills;
  String260 PCX_LM_Mountains;
  String260 PCX_LM_Forests;
  String260 PCX_LM_Terrain_Images[9];
  String260 PCX_Marsh;
  String260 PCX_Volcanos;
  String260 PCX_Volcanos_Forests;
  String260 PCX_Volcanos_Jungles;
  String260 PCX_Volcanos_Snow;
  String260 PCX_Polar_Icecaps;
  String260 PCX_Pollution;
  String260 PCX_Craters;
  String260 PCX_Railroads;
  String260 PCX_Roads;
  String260 PCX_Terrain_Buildings;
  String260 PCX_Territory;
  String260 PCX_Tnt;
  String260 PCX_Victory;
  String260 PCX_Waterfalls;
  String260 PCX_LM_Terrain;
  String260 PCX_Delta_Rivers;
  String260 PCX_Rivers;
  String260 PCX_Std_Terrain_Images[9];
  int field_3E94[4];
  int field_3EA4;
  int field_3EA8;
  Tile_Image_Info *Resources;
  Tile_Image_Info *ResourcesShadows;
  Tile_Image_Info Terrain_Buldings_Barbarian_Camp;
  Tile_Image_Info Terrain_Buldings_Mines;
  Tile_Image_Info Victory_Image;
  Tile_Image_Info Flood_Plains_Images[16];
  Tile_Image_Info Fog_Of_War_Images[81];
  Tile_Image_Info Polar_Icecaps_Images[32];
  Tile_Image_Info Railroads_Images[272];
  Tile_Image_Info Roads_Images[256];
  Tile_Image_Info Terrain_Buldings_Airfields[2];
  Tile_Image_Info Terrain_Buldings_Airfields_Shadow[2];
  Tile_Image_Info Terrain_Buldings_Camp[4];
  Tile_Image_Info Terrain_Buldings_Fortress[4];
  Tile_Image_Info Terrain_Buldings_Barricade[4];
  Tile_Image_Info Goody_Huts_Images[8];
  Tile_Image_Info Terrain_Buldings_Outposts[3];
  Tile_Image_Info Terrain_Buldings_Outposts_Shadow[3];
  Tile_Image_Info Pollution[25];
  Tile_Image_Info Craters[25];
  Tile_Image_Info Terrain_Buldings_Radar;
  Tile_Image_Info Terrain_Buldings_Radar_Shadow;
  Tile_Image_Info Tnt_Images[18];
  Tile_Image_Info Waterfalls_Images[4];
  Tile_Image_Info LM_Terrain[7];
  Tile_Image_Info Marsh_Large[8];
  Tile_Image_Info Marsh_Small[10];
  Tile_Image_Info field_C650;
  Tile_Image_Info field_C67C;
  Tile_Image_Info Volcanos_Images[16];
  Tile_Image_Info Volcanos_Forests_Images[16];
  Tile_Image_Info Volcanos_Jungles_Images[16];
  Tile_Image_Info Volcanos_Snow_Images[16];
  Tile_Image_Info Grassland_Forests_Large[8];
  Tile_Image_Info Plains_Forests_Large[8];
  Tile_Image_Info Tundra_Forests_Large[8];
  Tile_Image_Info Grassland_Forests_Small[10];
  Tile_Image_Info Plains_Forests_Small[10];
  Tile_Image_Info Tundra_Forests_Small[10];
  Tile_Image_Info Grassland_Forests_Pines[12];
  Tile_Image_Info Plains_Forests_Pines[12];
  Tile_Image_Info Tundra_Forests_Pines[12];
  Tile_Image_Info Irrigation_Desert_Images[16];
  Tile_Image_Info Irrigation_Plains_Images[16];
  Tile_Image_Info Irrigation_Images[16];
  Tile_Image_Info Irrigation_Tundra_Images[16];
  Tile_Image_Info Grassland_Jungles_Large[8];
  Tile_Image_Info Grassland_Jungles_Small[12];
  Tile_Image_Info Mountains_Images[16];
  Tile_Image_Info Mountains_Forests_Images[16];
  Tile_Image_Info Mountains_Jungles_Images[16];
  Tile_Image_Info Mountains_Snow_Images[16];
  Tile_Image_Info Hills_Images[16];
  Tile_Image_Info Hills_Forests_Images[16];
  Tile_Image_Info Hills_Jungle_Images[16];
  Tile_Image_Info_List Std_Terrain_Images[9];
  Tile_Image_Info Delta_Rivers_Images[16];
  Tile_Image_Info Mountain_Rivers_Images[16];
  Tile_Image_Info Territory_Images[8];
  Tile_Image_Info LM_Mountains_Images[16];
  Tile_Image_Info LM_Forests_Large_Images[8];
  Tile_Image_Info LM_Forests_Small_Images[10];
  Tile_Image_Info LM_Forests_Pines_Images[12];
  Tile_Image_Info LM_Hills_Images[16];
  Tile_Image_Info_List LM_Terrain_Images[9];
};

struct Tile_Type
{
  int V0;
  int field_4;
  int Ptr1;
  String32 Name;
  String32 Civilopedia_Entry;
  int IrrigationBonus;
  int MiningBonus;
  int RoadsBonus;
  int DefenceBonus;
  int MoveCost;
  int field_60;
  int FoodBase;
  int ProductionBase;
  int TradeBase;
  int WorkerJobID;
  int PollutionsEffect;
  Tile_Type_Flags Flags;
  int field_80;
  int field_84;
  int LM_FoodBase;
  int LM_ProductionBase;
  int LM_TradeBase;
  int LM_IrrigationBonus;
  int LM_MiningBonus;
  int LM_Roads_Bonus;
  int LM_MoveCost;
  int LM_DefenceBonus;
  String32 LM_Name;
  String32 LM_Entry;
  int field_E8;
  int field_EC;
};

struct PCX_Image
{
  int vtable;
  JGL_Renderer JGL;
};

struct FLC_Animation
{
  _188_vtable *vtable;
  _188_48 struct_48;
  int field_34[9];
  FLC_Frame_Image Frame_1;
  FLC_Frame_Image Frame_2;
  int field_E0;
  Animation_Info *Animation_Info;
  int field_E8[5];
  int field_FC;
  int field_100[7];
  int Direction3;
  int Direction4;
  int field_124;
  int field_128;
  int field_12C;
  int field_130;
  Unit *Unit;
  _188_4C struct_4C;
  int Last;
};

struct Airfield
{
  Tile_Building Base;
  int Used;
};

struct Colony
{
  Tile_Building Base;
};

struct Radar_Tower
{
  Tile_Building Base;
};

struct Outpost
{
  Tile_Building Base;
};

struct Advisor_Hints
{
  int vtable;
  String256 Empty_Message;
  int field_104;
  String256 Advisor_Messages[6];
  int Advisor_Current_Messages_ID[6];
};

struct Advisor_Form_vtable
{
  Base_Form_vtable Base;
  int m92_Get_AdvisorID;
  int m93;
  int m94;
  int m95;
  int m96;
  int m97;
  int m98;
};

struct Base_Form_Data
{
  int field_0[15];
  int Status_Flags;
  int field_40[22];
  int Status1;
  int Status2;
  int field_A0[6];
  Control_Tooltips Tooltips;
  int field_118[5];
  Base_Form *Parent;
  int field_130[30];
  int Left;
  int Top;
  int Right;
  int Bottom;
  int field_1B8;
  int field_1BC;
  int field_1C0[17];
  int Cursor_Image;
  int field_208;
  int field_20C;
  int field_210[24];
  PCX_Image Canvas;
  int field_528[17];
  int Last;
};

struct Button
{
  Base_Form_vtable *vtable;
  Base_Form_Data Base_Data;
  int field_574[29];
  int ControlID;
  int field_5EC;
  int field_5F0;
  int Text;
  char *ToolTip;
  int field_5FC[11];
  void (__cdecl * activation_handler) (int control_id);
  void (__cdecl * mouse_drag_handler) (int control_id);
  int field_630[6];
  Tile_Image_Info **Border_Images[3];
  Tile_Image_Info *Images[4]; // Norm, Rollover, Highlighted, Alpha
  int field_664;
  int field_668[26];
  int Last;
};

struct Advisor_Science_Form
{
  Advisor_Form_vtable *vtable;
  Base_Form_Data Base_Data;
  int field_574[2974];
  int Last;
};

struct Advisor_Base_Form
{
  Advisor_Form_vtable *vtable;
  Base_Form_Data Base_Data;
  int field_574[2090];
  int Last;
};

struct Main_Screen_Data_1AD4
{
  int field_0;
  int field_4;
  Unit *Units[1024];
  int Units_Count;
  Unit *Units2[512];
  int Units2_Count;
  int field_1810[51];
  int field_18DC;
  int field_18E0;
  int field_18E4[22];
  FLC_Animation Unit_Cursor_Animation;
  int field_1AC4;
  int field_1AC8;
  int field_1ACC;
  int field_1AD0;
  int field_1AD4;
  int field_1AD8;
  int field_1ADC;
  int field_1AE0;
  int field_1AE4;
  int field_1AE8;
  int field_1AEC;
};

struct Command_Button
{
  Button Button;
  int Command;
  int field_6D8;
};

struct GUI_Form_1
{
  GUI_Form_1_vtable *vtable;
  Base_Form_Data Base_Data;
  int field_574[26];
  Button OK_Btn;
  Button Cancel_Btn;
  int field_1384[188];
  int field_1674;
  int field_1678[652];
  int field_20A8[34];
  int Last;
};

struct ComboBox
{
  int vtable;
  int field_4[6];
  int Selected_Index;
  int field_20[2127];
  ComboBox_Text_Control TextBox;
  Button Button;
  int ItemList;
  int field_2DBC[25];
  int ListForm;
  int field_2E24[349];
  int field_3398;
  int field_339C[72];
  int ID;
};

struct Navigator_Data
{
  PCX_Image *Main_PCX;
  PCX_Image *PCX_0;
  RECT Rect;
  PCX_Image PCX_1;
  PCX_Image PCX_2;
  PCX_Image PCX_3;
  int field_840;
  Navigator_Cell *field_844;
  int field_848;
  int field_84C;
  int Mini_Map_Width;
  int Mini_Map_Height;
  PCX_Image PCX_4;
  int Mini_Map_Width2;
  int Mini_Map_Height2;
  int field_B18;
};

struct PCX_Animation
{
  int vtable;
  PCX_Image PCX;
  Flic_Anim_Info field_2BC;
  FLC_Frame_Image field_340;
  Flic_Anim_Info field_384;
  FLC_Frame_Image field_408;
};

struct ComboBox_2
{
  ComboBox Base;
  int field_34C0[5];
  int Last;
};

struct ComboBox1
{
  int vtable;
  int field_4[6];
  int Selected_Index;
  int field_20[2127];
  ComboBox_Text_Control TextBox;
  Button Button;
  int ItemList;
  int field_2DBC[25];
  int ListForm;
  int field_2E24[349];
  int field_3398;
  int field_339C[72];
};

struct Replays
{
  Base Base;
  Replays_Body Body;
};

struct City
{
  City_Base_vtable *vtable;
  int BaseBody[6];
  City_Body Body;
};

struct Unit_Body
{
  int vtable;
  int ID;
  int X;
  int Y;
  int PrevMoveX;
  int PrevMoveY;
  int CivID;
  int RaceID;
  int field_20;
  int UnitTypeID;
  int Combat_Experience;
  int Status;
  int Damage;
  int Moves;
  int Job_Value;
  int Job_ID;
  short Active;
  short field_42;
  int Container_Unit;
  int UnitState;
  int field_4C[3];
  String56 Custom_Name;
  int field_98;
  int path_dest_x;
  int path_dest_y;
  int action_target_x; // Action target is city location for settlers, drop location for transports
  int action_target_y;
  unsigned char path[256]; // List of neighbor indices, consumed back to front
  int path_len;
  int escortee;
  int Auto_CityID;
  int field_1B0[8];
  char carrying_princess_of_race;
  byte charmed;
  byte field_1D2;
  byte field_1D3;
  int field_1D4;
  LeaderKind leader_kind;
  IDLS IDLS;
  int field_210[12];
  RECT Rect;
  int field_250[4];
  FLC_Animation Animation;
};

struct Tile
{
  Tile_vtable *vtable;
  char River_Code;
  char Territory_OwnerID;
  short field_6;
  int ResourceType;
  int TileUnitID;
  short SquareParts;
  short VictoryPoint;
  int field_14;
  short Barbarian_TribeID;
  short CityID;
  short Tile_BuildingID;
  short ContinentID;
  int field_20;
  int Ruins;
  int Overlays;
  int SquareType;
  int Terrain_Overlays;
  int field_34;
  int field_38;
  Tile_Body Body;
};

struct Map
{
  Map_vtable *vtable;
  World_Features World;
  int TileCount;
  Map_Worker_Job_Info WorkerJobs[13];
  Tile **Tiles;
  int *ResourceCounts;
  int field_150;
  int Height;
  int Civ_Distance;
  int Civ_Count;
  int TileCount_Stat;
  int field_164;
  int Width;
  int Starting_Locations[32];
  int Seed;
  int Flags;
  Map_Body Body;
  int field_210;
  Continent *Continents;
  int field_218;
  int field_21C;
  int field_220;
  int field_224;
  int field_228;
  int field_22C;
  int Continent_Count;
  Map_Renderer Renderer;
};

struct BIC
{
  Base Base;
  int field_1C[523];
  int field_848;
  int field_84C;
  int unk0[3];
  int default_citizen_type;
  int unk1[6];
  int ImprovementsCount;
  int CitiesCount;
  int CitizenTypeCount;
  int ColoniesCount;
  int CulturalOpinionCount;
  int DifficultyLevelCount;
  int ErasCount;
  int DiplomatMissionCount;
  int CombatExperienceCount;
  int ResourceTypeCount;
  int GovernmentsCount;
  int Player_Count;
  int UnitTypeCount;
  int RacesCount;
  int StartingLocationsCount;
  int AdvanceCount;
  int WorkerJobCount;
  int TileTypesCount;
  int UnitsCount;
  int WorldSizesCount;
  int field_8C8;
  int Header;
  int field_8D0;
  int field_8D4[180];
  Improvement *Improvements;
  Citizen_Type *CitizenTypes;
  int Cities;
  int Colonies;
  int CulturalOpinions;
  Difficulty_Level *DifficultyLevels;
  Era_Type *Eras;
  int DiplomatMissions;
  Combat_Experience *CombatExperience;
  int Game;
  int field_BCC;
  int field_BD0;
  int field_BD4;
  int field_BD8[3132];
  Race *Races;
  Resource_Type *ResourceTypes;
  Government *Governments;
  Scenario_Player *Players;
  UnitType *UnitTypes;
  General General;
  Starting_Location *StartingLocations;
  Advance *Advances;
  Worker_Job *WorkerJobs;
  Tile_Type *TileTypes;
  int Units;
  World_Size *WorldSizes;
  int Flavours;
  int ScreenWidth;
  int ScreenHeight;
  int field_3E38;
  int field_3E3C;
  Fighter fighter;
  Map Map;
};

struct _1A4
{
  int V[3];
  FLC_Animation struct_188;
  int field_194[4];
};

struct Scroll_Bar
{
  Base_Form_vtable *vtable;
  Base_Form_Data Base_Data;
  int field_574;
  int field_578[29];
  int field_5EC[9];
  Button Up_Btn;
  Button Down_Btn;
  int field_13B8;
  int field_13BC;
};

struct Base_Form
{
  Base_Form_vtable *vtable;
  Base_Form_Data Data;
};

struct Advisor_Military_Form
{
  Advisor_Form_vtable *vtable;
  Base_Form_Data Base_Data;
  int field_574[17];
  int field_5B8[7];
  int field_5D4[9];
  String260 Labels[46];
  String260 field_34B0;
  Unit *Unit_1;
  Unit *Unit_2;
  City *City_1;
  City *City_2;
  int Selected_Civ;
  int field_35C8;
  int field_35CC;
  int field_35D0;
  int field_35D4;
  int Player_Units_Scroll_Position;
  int Target_Civ_Units_Scroll_Position;
  int Player_Cities_Count;
  int Target_Civ_Cities_Count;
  int field_35E8;
  int field_35EC;
  short field_35F0;
  char Player_Civ_Tab;
  char Target_Civ_Tab;
  int field_35F4;
  int field_35F8;
  int field_35FC;
  Navigator_Data *Navigator;
  Button Close_Btn;
  int Player_Units_Scroll_Positions[6];
  int Target_Civ_Units_Scroll_Positions[6];
  int field_3D08[12];
  int Lost_Units_Scroll_Positions[4];
  Button Buttons1[6];
  int field_6640[12];
  Button Buttons2[6];
  int field_8F68[12];
  Button Buttons3[6];
  int field_B890[12];
  Button Buttons4[6];
  int field_E1B8[12];
  Button Buttons5[2];
  int field_EF90[4];
  Button Buttons6[2];
  RECT field_FD48;
  RECT Player_List_Items_Rects[6];
  RECT Player_List_Labels_Rects[6];
  RECT Player_List_Units_Rects[6];
  RECT Target_Civ_List_Items_Rects[6];
  RECT Target_Civ_List_Labels_Rects[6];
  RECT Target_Civ_List_Units_Rects[6];
  RECT Tabs_Rects[4];
  RECT Player_List_Label_Rect;
  RECT Target_Civ_List_Label_Rect;
  RECT field_FFF8;
  RECT field_10008;
  RECT field_10018;
  RECT field_10028;
  RECT field_10038;
  RECT field_10048;
  RECT field_10058;
  RECT field_10068;
  RECT field_10078;
  RECT field_10088;
  RECT field_10098;
  Control_Tooltips Tooltips;
  Tile_Image_Info Images1[5];
  Scroll_Bar ScrollBar1;
  Scroll_Bar ScrollBar2;
  Scroll_Bar ScrollBar3;
  Scroll_Bar ScrollBar4;
  Scroll_Bar ScrollBars1[6];
  Scroll_Bar ScrollBars2[6];
  Scroll_Bar ScrollBar5;
  Scroll_Bar ScrollBar6;
  Scroll_Bar ScrollBar7;
  Scroll_Bar ScrollBar8;
  int field_28CE4[22];
  int Last;
};

struct Advisor_Trade_Form
{
  Advisor_Form_vtable *vtable;
  Base_Form_Data Base_Data;
  int field_574[3];
  Tile_Image_Info Image1;
  int field_5AC;
  int field_5B0;
  int field_5B4;
  String260 Labels[28];
  int field_2228[48];
  Button CloseBtn;
  int field_29BC[12];
  RECT Local_Resources_Rect;
  RECT Import_Resource_Rect;
  RECT Export_Resources_Rect;
  RECT Non_Trade_Cities_Rect[7];
  RECT field_2A8C;
  RECT Target_Civs_Rect[8];
  RECT field_2B1C;
  int field_2B2C[24];
  Scroll_Bar ScrollBar1;
  Scroll_Bar ScrollBar2;
  Scroll_Bar ScrollBar3;
};

struct Advisor_Internal_Form
{
  Advisor_Form_vtable *vtable;
  Base_Form_Data Base_Data;
  int field_574[17];
  String260 Labels[55];
  RECT Cities_Names_Rect[9];
  RECT Cities_Citizens_Rects[9];
  int Pointer1;
  int field_3EB8;
  int field_3EBC[7];
  int field_3ED8;
  int field_3EDC[80];
  short Start_City_Index;
  short Cities_Count;
  int field_4020[24];
  short City_ID_List[512];
  RECT Tax_Science_Thumb_Rect;
  RECT Tax_Luxury_Thumb_Rect;
  RECT Gov_Btn_Rect;
  RECT Conscription_Btn_Rect;
  int field_44C0[44];
  Tile_Image_Info Image1;
  void *Tax_Button_Images[3];
  Scroll_Bar ScrollBar;
  RECT Tax_Science_Plus_Btn_Rect;
  RECT Tax_Science_Minus_Btn_Rect;
  RECT Tax_Luxury_Plus_Btn_Rect;
  RECT Tax_Luxury_Minus_Btn_Rect;
  RECT Tax_Science_Scroll_Bar_Rect;
  RECT Tax_Luxury_Scroll_Bar_Rect;
  Tile_Image_Info *Plus_Btn_Images;
  Tile_Image_Info *Minus_Btn_Images;
  Button Science_Plus_Btn;
  Button Luxury_Plus_Btn;
  Button Science_Minus_Btn;
  Button Luxury_Minus_Btn;
  Button Close_Btn;
};

struct Advisor_GUI
{
  Base_Form Base;
  Advisor_Base_Form *pAdvisors[6];
  int field_58C;
  int field_590;
  String260 Advisor_Labels[7];
  String260 Moods[4];
  String260 Current_Advisor_Label;
  String260 PCX_Files[18];
  String260 TXT_Labels;
  int Current_Advisor_ID;
  int field_2514[9];
  Tile_Image_Info Backgrounds_Images[10];
  Tile_Image_Info Advisor_Buttons_Images[18];
  Tile_Image_Info Close_Button_Images[3];
  Button field_2A8C;
  Tile_Image_Info Faces_Images[96];
  Navigator_Data Navigator;
  int Last;
};

struct City_View_Form
{
  Base_Form Base;
  Button Exit_Btn;
  int field_C48[3];
  PCX_Image PCX;
  int field_F0C[10];
  Tile_Image_Info Background_Image;
  int field_F60[198];
  int field_1278[158];
  int field_14F0[182];
  Tile_Image_Info *Roads_Images[15];
  Tile_Image_Info *Roads_Images2[3];
  int field_1810[4];
  Tile_Image_Info field_1820;
  Tile_Image_Info field_184C;
  Tile_Image_Info Terrain_Parts[28];
  int field_1D48[257];
  int field_214C[255];
  City *City;
  int Era_ID;
  int Culture_GroupID;
  int field_2554[255];
  int field_2950;
};

struct Context_Menu
{
  Base_Form Base;
  int field_574;
  Context_Menu_Item Items[256];
  int field_2178[3];
  int Item_Count;
  int widest_item;
  int ItemHeight;
  int Selected_Item;
  int separator_item_count;
  int field_2198[6];
  int Last;
};

struct Base_List_Control
{
  Base_Form Base;
  int field_574;
  int List2;
  int field_57C[61];
  int Last;
};

struct Base_List_Box
{
  int vtable;
  Control_Data_Offsets *Offsets;
  int field_8[7];
  int mouse_x;
  int mouse_y;
  int first_item_shown;
  int shown_item_count;
  int field_34[12];
  int Data1;
  int field_68;
  int Parent;
  int field_70;
  Base_List_Control Control;
};

struct CheckBox_Control
{
  Base_Form Base;
  int field_574;
  int List[62];
  int Last;
};

struct CheckBox
{
  Control_Data_Offsets *Offsets;
  int field_4[7];
  CheckBox_Control Control;
};

struct Parameters_Form
{
  Base_Form Base;
  Tile_Image_Info Image1;
  Button OK_Btn;
  Button Cancel_Btn;
  CheckBox CheckBoxes[33];
  int field_EC5C;
  int Dialog_Result;
  ComboBox ComboBox;
  int field_12124[6];
  Scroll_Bar AllSound_ScrollBar;
  Scroll_Bar Music_ScrollBar;
  Scroll_Bar Effects_ScrollBar;
};

struct Common_Dialog_Form
{
  Base_Form Base;
  int field_574[26];
  Button OK_Btn;
  Button Cancel_Btn;
  int field_1384[188];
  int field_1674[577];
  int field_1F78[111];
  int field_2134;
  Base_Form SubForm;
  int field_26AC;
  int field_26B0;
  int Last;
};

struct Civilopedia_Form
{
  Base_Form Base;
  int field_574[65];
  Common_Dialog_Form Sub_Dialog;
  int field_2D30[6];
  Tile_Image_Info Backgrounds_Images[8];
  Tile_Image_Info Relations_Images[8];
  Tile_Image_Info Color_Bars_Images[17];
  Tile_Image_Info Next_Btn_Images[3];
  Tile_Image_Info Prev_Btn_Images[3];
  Tile_Image_Info Description_Btn_Images[3];
  Tile_Image_Info Navigator_Buttons_Images[9];
  int Current_Article_ID;
  int field_3610;
  int field_3614;
  int field_3618;
  int field_361C[16];
  Button Close_Btn;
  Button Up_Btn;
  Button Right_Btn;
  Button Left_Btn;
  Button Next_Btn;
  Button Prev_Btn;
  Button Description_Btn;
  int History;
  char gap_662C[696];
  int field_68E4;
  Scroll_Bar Scroll_Bar_1;
  Scroll_Bar Scroll_Bar_2;
  int field_9068;
  int field_906C;
  int Max_Article_ID;
  String260 TXT_Civilopedia;
  Civilopedia_Article **Articles;
};

struct TextBox
{
  Base_Form Base;
  Scroll_Bar Scroll_Bar;
  int field_1934[16];
  Form_Data_30 field_1974;
};

struct File_Dialog_Body
{
  int field_0;
  int field_678;
  Tile_Image_Info Image;
  String260 Root_Path;
  String260 Game_Root_Path;
  int field_8B0[278];
  Button OK_Btn;
  Button Cancel_Btn;
  TextBox File_Edit;
  char gap_2DE0[100];
  Base_Form File_List;
  int field_3A2C[65];
};

struct Main_Menu_Form
{
  GUI_Form_1 Base;
  int field_2134;
  Base_Form Sub_Form;
  int field_26AC[6];
  int Last;
};

struct Espionage_Form
{
  Base_Form Base;
  Tile_Image_Info Image;
  Button OK_Btn;
  Button Cancel_Btn;
  Tile_Image_Info field_1348[3];
  Tile_Image_Info field_13CC[4];
  Tile_Image_Info field_147C[4];
  Tile_Image_Info field_152C[2];
  int field_1584;
  int field_1588;
  Scroll_Bar Scroll_Bar;
  int field_294C[1311];
  int field_3DC8[174];
  int field_4080[8];
  int field_40A0;
  int Last;
};

struct Palace_View_Form
{
  Base_Form Base;
  Palace_View_Info *Palace_Info;
  RECT CloseBtn_Rect;
  Button CloseBtn;
  Button Culture_Buttons[5];
  Button_Images_3 Icon_Images[5];
  Tile_Image_Info Background_Image;
  Tile_Image_Info *Item_Images[5];
  PCX_Image PCX;
  Point *Items_Points;
  int Selected_CultureID;
  int Item_Counts[5];
  int *Item_Rules[5];
  int Max_Item_Image_Count;
  int Current_Active_Item;
  int field_3444;
  int field_3448;
  int Last;
};

struct Spaceship_Form
{
  Base_Form Base;
  PCX_Image PCX;
  int field_82C;
  int field_830;
  Tile_Image_Info Background_Image;
  Tile_Image_Info Parts_Images[17];
  Tile_Image_Info ProgressBar_Image;
  Tile_Image_Info Launch_Btn_Images[6];
  Tile_Image_Info Big_Brother_Images[3];
  int field_D04[54];
  int Cities[10];
  int Stat_1[10];
  float Ratio_1[10];
  int field_E54;
};

struct Demographics_Form
{
  Base_Form Base;
  Tile_Image_Info *Background_Image;
  String260 Strings[25];
  int Top_Cities[5];
  float Stat_Values[13];
  int Stat_Ranks[13];
  int field_1F58;
  Button CloseBtn;
};

struct Victory_Conditions_Form
{
  Base_Form Base;
  int field_574[5];
  Tile_Image_Info Image1;
  Tile_Image_Info Image2;
  int field_5E0[6];
  Scroll_Bar ScrollBar;
  Button CloseBtn;
  int field_208C;
  ComboBox ComboBox1;
  int field_5550;
  ComboBox ComboBox2;
  int field_8A14;
};

struct Wonders_Form
{
  Base_Form Base;
  Tile_Image_Info Background_Image;
  Tile_Image_Info Box_Image;
  Tile_Image_Info Box_Overlay_Image;
  Scroll_Bar Scroll_Bar;
  int field_19B8[24];
  String260 Labels[5];
  int field_1F2C[10];
  Button Close_Btn;
};

struct MP_LAN_Game_Parameters_Form
{
  Base_Form Base;
  Tile_Image_Info Button4_Images[3];
  Button Button1;
  int field_CCC;
  ComboBox ComboBoxList1[11];
  int field_25110;
  Button Button2;
  Tile_Image_Info Background_Image;
  Tile_Image_Info Button1_Images[3];
  Tile_Image_Info Button2_Images[3];
  Tile_Image_Info Button3_Images[3];
  PCX_Image PCX1;
  Button Buttons2[8];
  ComboBox ComboBoxList2[8];
  ComboBox ComboBoxList3[8];
  TextBox TextBox1;
  Scroll_Bar ScrollBar1;
  Scroll_Bar ScrollBar2;
};

struct MP_Internet_Game_Parameters_Form
{
  Base_Form Base;
  Button Button1;
  Tile_Image_Info Image1;
  Tile_Image_Info Images1[3];
  Tile_Image_Info Images2[3];
  Tile_Image_Info Images3[3];
  Tile_Image_Info Image2;
  ComboBox ComboBoxList[2];
  TextBox TextBox;
  Scroll_Bar ScrollBar;
  Button Buttons1[8];
};

struct MP_Manager_Form
{
  Base_Form Base;
  Button Close_Btn;
  Tile_Image_Info Lobby_Background_Image;
  Tile_Image_Info Lobby_Bar_Image;
  Tile_Image_Info Collapse_Btn_Image;
  Tile_Image_Info Expand_Btn_Image;
  Tile_Image_Info Game_Status_Images[3];
  Tile_Image_Info Image1;
  RECT Rects[14];
  _12C _12C_Array1[2048];
  _12C _12C_Array2[2048];
  TextBox Message_TextBox;
  Scroll_Bar Messages_ScrollBar;
  Scroll_Bar Games_ScrollBar;
  int field_130FAC[16923];
  Tile_Image_Info Description_Btn_Images[3];
  Button Direct_Connection_Btn;
};

struct Wonder_Form
{
  Base_Form Base;
  Tile_Image_Info *Background;
  Tile_Image_Info *Image1;
  Tile_Image_Info View_Btn_Images[3];
  int field_600[69];
  int field_714[423];
  Button View_Btn;
  Button CloseBtn;
  City *City;
  RECT Wonder_Image_Rect;
  int Last;
};

struct New_Game_Map_Form
{
  Base_Form Base;
  int field_574;
  int field_578;
  int field_57C;
  Tile_Image_Info Background_Image;
  Tile_Image_Info Landmass_Water_Large_Images[9];
  Tile_Image_Info Landmass_Water_Small_Images[27];
  Tile_Image_Info Images3[3];
  Tile_Image_Info Climate_Images[9];
  Tile_Image_Info Images5[3];
  Tile_Image_Info Images6[9];
  Tile_Image_Info Images7[3];
  Tile_Image_Info Images8[9];
  TextBox MapSeed_TextBox;
  Button OkBtn;
  Button CloseBtn;
  PCX_Image PCX;
  int World_Size;
  int Barbarian_Activity;
  int Ocean_Coverage;
  int World_Landmass;
  int World_Aridity;
  int World_Temparature;
  int World_Age;
  int field_3C2C;
  int Dialog_Result;
  String256 Str_Continue;
  String256 Str_Back;
};

struct New_Game_Player_Form
{
  Base_Form Base;
  int field_574;
  String256 Str_Continue;
  String256 Str_Back;
  PCX_Image PCX;
  PCX_Animation PCX_Animation;
  int field_E7C[13];
  Tile_Image_Info Background_Image;
  Tile_Image_Info WhoLeader_Image;
  Tile_Image_Info Description_Button_Images[3];
  Tile_Image_Info Custom_Leader_Button_Images[3];
  Button OkBtn;
  Button CancelBtn;
  Button DescriptionBtn;
  Button ConstraintsBtn;
  PCX_Image PCX2;
  int field_2E18[4];
  Game_Limits Turns_Limit;
  int field_2E64;
  int field_2E68;
  int field_2E6C;
  int field_2E70[46];
  ComboBox_2 ComboBox_Array[32];
  int Dialog_Result;
  int field_6CA2C;
  int Last;
};

struct Campain_Records_Form
{
  Base_Form Base;
  Tile_Image_Info Background_Image;
  Button Ok_Btn;
  Button Cancel_Btn;
  Tile_Image_Info Next_Btn_Images[3];
  Button Next_Btn;
  int field_1AA0;
  int field_1AA4;
  String260 BIQ_File;
  int field_1BAC;
};

struct Load_Scenario_Form
{
  Base_Form Base;
  int field_574[10];
  PCX_Image PCX;
  int field_854[65];
  String260 Labels[11];
  int field_1484;
  Button Cancel_Btn;
  Button Ok_Btn;
  int Scenario_ListBox[349];
  int field_27A4[89];
  int field_2908;
};

struct Authors_Form
{
  Base_Form Base;
  int field_574[10];
  Button Close_Btn;
  int field_C70[522];
  Tile_Image_Info Image1;
  Base_Form Sub_Form;
  int field_1A38;
  PCX_Image PCX;
  Button Button1;
  Form_Data_30 Data_30;
};

struct MP_Filters_Form
{
  Base_Form Base;
  Tile_Image_Info Background;
  Button Ok_Btn;
  Button Cancel_Btn;
  ComboBox Server_ComboBox;
  String260 Label_OK;
  String260 Label_Cancel;
  RECT Rects[15];
  int field_4B00[5];
  int field_4B14;
};

struct Radio_Button_List
{
  int vtable;
  Tile_Image_Info *Button_Images[3];
  int field_10;
  int field_14[5];
  Base_Form Control;
  int field_59C;
  Item_List2 Item_List;
};

struct Game_Results_Form
{
  Base_Form Base;
  Tile_Image_Info Background;
  Button_Images_3 Prev_Turn_Btn_Images;
  Button_Images_3 Stop_Btn_Images;
  Button_Images_3 Play_Btn_Images;
  Button_Images_3 Next_Turn_Btn_Images;
  Button_Images_3 First_Turn_Btn_Images;
  Tile_Image_Info ProgressBar_Image;
  ComboBox1 ComboBox;
  Replay_Turn *Current_Replay_Turn;
  int ItemList1[438];
  Button Close_Btn;
  CheckBox CheckBox;
  RECT Prev_Turn_Btn_Rect;
  RECT Stop_Btn_Rect;
  RECT Play_Btn_Rect;
  RECT Next_Turn_Btn_Rect;
  RECT Mini_Map_Rect;
  RECT First_Turn_Btn_Rect;
  int Form_Left;
  int Form_Top;
  int field_51C8;
  int field_51CC;
  int Time;
  int field_51D4;
  int field_51D8;
  int Status;
  Common_Dialog_Form *Common_Dialog;
  Navigator_Data Navigator_Data;
};

struct Game_Spy_Connection_Form
{
  Base_Form Base;
  Button Cancel_Btn;
  Tile_Image_Info Calcen_Btn_Images[3];
  int field_CCC[6];
  char B1;
  char B2;
  short Last;
};

struct City_Form
{
  Base_Form Base;
  City_Screen_FileNames Files;
  byte field_19C4;
  byte production_selector_open;
  byte field_19C6;
  byte field_19C7;
  int field_19C8[3];
  int Current_Era;
  int Current_Era2;
  int Current_Culture_Group2;
  int Current_Culture_Group;
  int field_19E4;
  Tile *Current_Tile;
  City *CurrentCity;
  Button Current_Order_Btn;
  Button City_View_Btn;
  Button Exit_Btn;
  Button Next_Btn;
  Button Prev_Btn;
  Button Hurry_Btn;
  Button Draft_Btn;
  Button Governor_Btn;
  Button Resources_View_Btn;
  Tile_Image_Info Background_Image;
  Tile_Image_Info HurryButton_Images[3];
  Tile_Image_Info Govern_Images[3];
  Tile_Image_Info DraftButton_Images[3];
  Tile_Image_Info ProdButton_Images[3];
  Tile_Image_Info PopheadHilite_Image;
  PCX_Image PCX1;
  City_Icon_Images City_Icons_Images;
  Tile_Image_Info City_Icons_Images_42[4];
  Tile_Image_Info Top_Buttons_Images[12];
  Tile_Image_Info City_View_Btn_Image;
  Tile_Image_Info Close_Btn_Images[3];
  Tile_Image_Info BottomFadeBar_Image;
  Tile_Image_Info BottomFadeBarAlpha;
  Tile_Image_Info TopFadeBar_Image;
  Tile_Image_Info TopFadeBarAlpha_Image;
  Tile_Image_Info ProductionQueueBox_Image;
  Tile_Image_Info ProductionQueueBar_Image;
  Tile_Image_Info *Improvement_Images_Small;
  Tile_Image_Info *Improvement_Images_Large;
  void *Luxury_Res_Images[8];
  int field_6570[61];
  Tile_Image_Info Image_1;
  Tile_Image_Info Image_2;
  RECT Citizens_Rect;
  RECT Production_Rect;
  RECT Rect1;
  RECT Food_Storage_Rect;
  RECT Food_Consumption_Rect;
  RECT Income_Rect;
  RECT Gold_Income_Rect;
  RECT Science_Income_Rect;
  RECT Luxury_Income_Rect;
  RECT Pollution_Wastes_Rect;
  RECT Food_Storage_Indicator_Rect;
  RECT Production_Storage_Indicator;
  RECT field_677C;
  RECT field_678C;
  RECT field_679C;
  RECT field_67AC;
  RECT field_67BC;
  RECT field_67CC;
  RECT field_67DC;
  RECT field_67EC;
  RECT field_67FC;
  RECT field_680C;
  RECT field_681C;
  RECT field_682C;
  RECT Culture_Rect;
  RECT Units_Rect;
  RECT Pollution_Rect;
  RECT Rects_2[5];
  Scroll_Bar Units_Scroll_Bar;
  Base_List_Box Order_List;
  Base_List_Box Order_Queue_List;
  Base_List_Box Improvement_List;
  Base_Form Order_Queue_Dialog;
  int field_96A8;
  int Units_Start_Index;
  int field_96B0;
  int field_96B4[5];
  FLC_Animation struct_188;
  City_Form_Labels Labels;
  int field_98D0[67];
  Tile_Image_Info QueueBase_Image;
};

struct Unit
{
  Unit_vtable *vtable;
  int field_4;
  char str_UNIT[4];
  int field_C[4];
  Unit_Body Body;
};

struct Tile_Info_Form
{
  Base_Form Base;
  Tile_Image_Info Image;
  Button Close_Btn;
  int field_C74;
};

struct Main_GUI
{
  Base_Form Base;
  int field_574[4];
  int field_584;
  int field_588;
  int field_58C[769];
  String260 Main_Commands[29];
  int field_2F04;
  int field_2F08;
  String260 PCX_Files[25];
  String260 Unit_Commands[59];
  Button Advisors_Btn;
  Button Menu_Btn;
  Button Geography_Btn;
  Button Civilopedia_Btn;
  Button Map_Navigator_Buttons[8];
  Button Right_Panel_Buttons[7];
  int field_10618[8];
  int b_Navigate_City_Mode;
  int field_1063C[3];
  int b_Move_Identical_Groups;
  int field_1064C[3];
  int b_Navigate_Unit_Mode;
  int field_1065C[49];
  Navigator_Data Navigator_Data;
  GUI_Form_1 Message_Box;
  int field_13370;
  Base_Form Sub_Form_1;
  int field_138E8[392];
  RECT Rects1[3];
  RECT Mini_Map_Drag_Rect;
  RECT Rects2[10];
  RECT Unit_Status_Rect;
  RECT Mini_Map_Click_Rect;
  RECT Rects3[5];
  int field_14058[32];
  Tile_Image_Info Images[252];
  Tile_Image_Info Image2;
  Tile_Image_Info Image3;
  Form_Data_30 Data_30_1;
  Form_Data_30 Data_30_2;
  int field_16CE0;
  Command_Button Unit_Command_Buttons[42];
};

struct Advisor_Culture_Form
{
  Advisor_Form_vtable *vtable;
  Base_Form_Data Base_Data;
  int field_574[2294];
  int Cities_Count;
  int Cities_ID[512];
  int Selected_City_Index;
  int field_3154;
  int field_3158;
  int field_315C[7];
  int field_3178[21];
  Button Buttons[6];
  int field_5AC4[68];
  int field_5BD4;
  int field_5BD8[27];
  Scroll_Bar Cities_ScrollBar;
  Scroll_Bar City_Improvements_ScrollBar;
};

struct Advisor_Foreign_Form
{
  Advisor_Form_vtable *vtable;
  Base_Form_Data Base_Data;
  int field_574[3];
  Tile_Image_Info Image1;
  int field_5AC;
  int field_5B0;
  int field_5B4;
  Base FAXX_Object;
  int Current_Tab;
  int Report_Civ_List[8];
  unsigned char Draw_Civ_Index_List[8];
  char Relation_CheckBoxes_Values[6];
  char Treaties_CheckBoxes_Values[3];
  char field_609[3];
  int field_60C[8];
  String260 Labels[46];
  String260 Str1;
  int Object_663CFC_1;
  int Object_663CFC_2;
  int Object_663CFC_3;
  int Image2;
  int Image3;
  int field_35FC[6];
  int Leaders_X_Positions[8];
  int Leaders_Y_Positions[8];
  int field_3654[64];
  int field_3754[35];
  int field_37E0;
  int field_37E4[2];
  int ScrollBar_Position;
  int field_37F0;
  int field_37F4;
  int field_37F8;
  int field_37FC;
  RECT Tabs_Rects[3];
  RECT Leaders_Rects[8];
  int field_38B0[4];
  RECT Show_All_Btn_Rect;
  RECT Hide_All_Btn_Rect;
  RECT Button1_Rect;
  Button Button1;
  Button Button2;
  CheckBox Relations_CheckBoxes[6];
  CheckBox Treaties_CheckBoxes[3];
  Scroll_Bar ScrollBar;
  int Selected_Details_Civ;
};

struct Main_Screen_Form
{
  Base_Form_vtable *vtable;
  Base_Form_Data Base_Data;
  Base Console;
  Main_Screen_Data_170 Data_170_Array[50];
  int field_4D70;
  Unit *Current_Unit;
  int field_4D78;
  City *Selected_City;
  int field_4D80;
  int field_4D84;
  Form_Data_30 Data_30_1;
  int field_4DB8;
  int Player_CivID;
  int field_4DC0[25];
  Form_Data_30 Data_30_2;
  int field_4E54[7];
  int TileX_Min;
  int TileX_Max;
  int TileY_Min;
  int TileY_Max;
  int field_4E80[19];
  int Mode_Action;
  int field_4ED0;
  int Mode_Action_Range;
  Base_Form Units_Control;
  int field_544C;
  Main_GUI GUI;
  int field_2E14C[6];
  Form_Data_30 Data_30_3;
  int field_2E194;
  char turn_end_flag;
  char field_2E199;
  char field_2E19A;
  char field_2E19B;
  int field_2E19C;
  Navigator_Point *First_Ptr;
  Navigator_Point *Second_Ptr;
  int field_2E1A8[2];
  int mouse_x;
  int mouse_y;
  Main_Screen_Data_1AD4 Data_1AD4;
};

struct Governor_Form
{
  Base_Form_vtable *vtable;
  Base_Form_Data Base_Data;
  CheckBox CheckBox;
  Tile_Image_Info Image1;
  int Current_City_ID;
  int field_C38;
  int field_C3C;
  int field_C40;
  int field_C44;
  int field_C48;
  String260 Labels[37];
  int Page;
  ComboBox Common_Policy_ComboBox_Array[14];
  ComboBox Production_Policy_ComboBox_Array[30];
  int Selected_Values_Common[14];
  int Selected_Values_Production[30];
  int field_94394;
  RECT Common_Link_Rect;
  RECT Production_Link_Rect;
  RECT OK_Btn_Rect;
  RECT Cancel_Btn_Rect;
};

struct File_Dialog_Form
{
  Base_Form Base;
  String256 Label;
  File_Dialog_Body Body;
  Scroll_Bar Scroll_Bar;
};

struct New_Era_Form
{
  Base_Form Base;
  Tile_Image_Info *Image1;
  Tile_Image_Info *Image2;
  String256 field_57C;
  int field_67C;
  int Old_Era;
  int field_684;
  int field_688;
  int field_68C;
  int field_690;
  Radio_Button_List Radio_List;
  Button OK_Btn;
  Button Close_Btn;
};





typedef struct Object_66C3FC Object_66C3FC;
typedef struct Object_66D520 Object_66D520;
typedef struct PopupSelection PopupSelection;
typedef struct PopupFormVTable PopupFormVTable;
typedef struct PopupForm PopupForm;
typedef struct StrWithJunk StrWithJunk;
typedef struct TradeOfferVTable TradeOfferVTable;
typedef struct TradeOffer TradeOffer;
typedef struct TradeOfferList TradeOfferList;
typedef struct Object_667188 Object_667188;
typedef struct DiploForm DiploForm;

// Contains font info for a particular size & style
struct Object_66C3FC
{
	void * vtable;
	int unk[7];
};

// Contains a list of font infos. Maybe just for Lucida (are there any other fonts used in the game?)
struct Object_66D520
{
	void * vtable;
	int unk[4];
	Object_66C3FC * object_66C3FCs;
};

struct PopupSelection
{
	void * vtable;
	int unk[481];
};

struct PopupFormVTable
{
	PopupForm * (__fastcall * destruct) (PopupForm *, __, byte);
	void * unk0[91];
	void (__fastcall * set_text_key_and_flags) (PopupForm *, __, char *, char *, int, int, int, int);
	void * unk1[14];
};

// Note: This is the type of the statically allocated global Popup thing that you get if you call get_popup_form. I don't think
// it's the same as the popup object allocated on the stack in many places, which I believe is the type Antal1987 called
// GUI_Form_1. Also, PopupForm inherits from GUI_Form_1, that's why the first fields are the same.
struct PopupForm
{
	PopupFormVTable * vtable;
	Base_Form_Data base_data;
	int unk0[26];
	Button ok_button;
	Button cancel_button;
	int unk1[57];
	PopupSelection selection;
	int unk2[338];
};

struct StrWithJunk
{
	char * str;
	byte junk_1[4];
	int junk_2;
};

struct TradeOfferVTable
{
	TradeOffer * (__fastcall * destruct) (TradeOffer *, __, byte);
};

struct TradeOffer
{
	TradeOfferVTable * vtable;
	int kind;
	int param_1;
	int param_2;
	TradeOffer * next;
	TradeOffer * prev;
};

struct TradeOfferList
{
	void * vtable;
	int length;
	TradeOffer * first;
	TradeOffer * last;
};

struct Object_667188
{
	void * vtable; // = 0x667188
	int field_4[1628];
};

struct DiploForm
{
	Base_Form_vtable * vtable; // = 0x66C0D8
	int field_4[933];
	int other_party_civ_id;
	int field_E9C[15];
	int mode;
	int field_EDC[41];
	TradeOfferList their_offer_lists[32];
	TradeOfferList our_offer_lists[32];
	int field_1380[4];
	int field_1390[3];
	StrWithJunk * headings;
	StrWithJunk * agreements;
	StrWithJunk * alliances;
	StrWithJunk * embargoes;
	void * field_13AC;
	StrWithJunk * communications;
	int field_13B4;
	int field_13B8;
	int field_13BC;
	int field_13C0;
	int field_13C4;
	int field_13C8;
	char * attitudes[5]; // polite, annoyed, etc.
	int field_13E0[24];
	Tile_Image_Info talk_offer_img;
	Tile_Image_Info consider_img;
	Tile_Image_Info counter_img;
	Tile_Image_Info uparrow_img;
	Tile_Image_Info downarrow_img;
	Tile_Image_Info MPcounter_img;
	Tile_Image_Info large_icons[11];
	Tile_Image_Info small_icons[11];
	Tile_Image_Info diplowinbox_img;
	PCX_Image field_193C;
	int field_1BF4[287];
	Object_667188 field_2070;
};
