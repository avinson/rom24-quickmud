/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik Strfeldt, Tom Madsen, and Katja Nyboe.    *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 **************************************************************************/

/***************************************************************************
 *   ROM 2.4 is copyright 1993-1998 Russ Taylor                            *
 *   ROM has been brought to you by the ROM consortium                     *
 *       Russ Taylor (rtaylor@hypercube.org)                               *
 *       Gabrielle Taylor (gtaylor@hypercube.org)                          *
 *       Brian Moore (zump@rom.org)                                        *
 *   By using this code, you have agreed to follow the terms of the        *
 *   ROM license, in the file Rom24/doc/rom.license                        *
 **************************************************************************/

#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <time.h>
#include "merc.h"
#include "magic.h"
#include "interp.h"


/* item type list */
const struct item_type item_table[] = {
    {ITEM_LIGHT, "light"},
    {ITEM_SCROLL, "scroll"},
    {ITEM_WAND, "wand"},
    {ITEM_STAFF, "staff"},
    {ITEM_WEAPON, "weapon"},
    {ITEM_TREASURE, "treasure"},
    {ITEM_ARMOR, "armor"},
    {ITEM_POTION, "potion"},
    {ITEM_CLOTHING, "clothing"},
    {ITEM_FURNITURE, "furniture"},
    {ITEM_TRASH, "trash"},
    {ITEM_CONTAINER, "container"},
    {ITEM_DRINK_CON, "drink"},
    {ITEM_KEY, "key"},
    {ITEM_FOOD, "food"},
    {ITEM_MONEY, "money"},
    {ITEM_BOAT, "boat"},
    {ITEM_CORPSE_NPC, "npc_corpse"},
    {ITEM_CORPSE_PC, "pc_corpse"},
    {ITEM_FOUNTAIN, "fountain"},
    {ITEM_PILL, "pill"},
    {ITEM_PROTECT, "protect"},
    {ITEM_MAP, "map"},
    {ITEM_PORTAL, "portal"},
    {ITEM_WARP_STONE, "warp_stone"},
    {ITEM_ROOM_KEY, "room_key"},
    {ITEM_GEM, "gem"},
    {ITEM_JEWELRY, "jewelry"},
    {ITEM_JUKEBOX, "jukebox"},
    {0, NULL}
};


/* weapon selection table */
const struct weapon_type weapon_table[] = {
    {"sword", OBJ_VNUM_SCHOOL_SWORD, WEAPON_SWORD, &gsn_sword},
    {"mace", OBJ_VNUM_SCHOOL_MACE, WEAPON_MACE, &gsn_mace},
    {"dagger", OBJ_VNUM_SCHOOL_DAGGER, WEAPON_DAGGER, &gsn_dagger},
    {"axe", OBJ_VNUM_SCHOOL_AXE, WEAPON_AXE, &gsn_axe},
    {"staff", OBJ_VNUM_SCHOOL_STAFF, WEAPON_SPEAR, &gsn_spear},
    {"flail", OBJ_VNUM_SCHOOL_FLAIL, WEAPON_FLAIL, &gsn_flail},
    {"whip", OBJ_VNUM_SCHOOL_WHIP, WEAPON_WHIP, &gsn_whip},
    {"polearm", OBJ_VNUM_SCHOOL_POLEARM, WEAPON_POLEARM, &gsn_polearm},
    {NULL, 0, 0, NULL}
};



/* wiznet table and prototype for future flag setting */
const struct wiznet_type wiznet_table[] = {
    {"on", WIZ_ON, IM},
    {"prefix", WIZ_PREFIX, IM},
    {"ticks", WIZ_TICKS, IM},
    {"logins", WIZ_LOGINS, IM},
    {"sites", WIZ_SITES, L4},
    {"links", WIZ_LINKS, L7},
    {"newbies", WIZ_NEWBIE, IM},
    {"spam", WIZ_SPAM, L5},
    {"deaths", WIZ_DEATHS, IM},
    {"resets", WIZ_RESETS, L4},
    {"mobdeaths", WIZ_MOBDEATHS, L4},
    {"flags", WIZ_FLAGS, L5},
    {"penalties", WIZ_PENALTIES, L5},
    {"saccing", WIZ_SACCING, L5},
    {"levels", WIZ_LEVELS, IM},
    {"load", WIZ_LOAD, L2},
    {"restore", WIZ_RESTORE, L2},
    {"snoops", WIZ_SNOOPS, L2},
    {"switches", WIZ_SWITCHES, L2},
    {"secure", WIZ_SECURE, L1},
    {NULL, 0, 0}
};

/* attack table  -- not very organized :( */
const struct attack_type attack_table[MAX_DAMAGE_MESSAGE] = {
    {"none", "hit", -1},        /*  0 */
    {"slice", "slice", DAM_SLASH},
    {"stab", "stab", DAM_PIERCE},
    {"slash", "slash", DAM_SLASH},
    {"whip", "whip", DAM_SLASH},
    {"claw", "claw", DAM_SLASH},    /*  5 */
    {"blast", "blast", DAM_BASH},
    {"pound", "pound", DAM_BASH},
    {"crush", "crush", DAM_BASH},
    {"grep", "grep", DAM_SLASH},
    {"bite", "bite", DAM_PIERCE},    /* 10 */
    {"pierce", "pierce", DAM_PIERCE},
    {"suction", "suction", DAM_BASH},
    {"beating", "beating", DAM_BASH},
    {"digestion", "digestion", DAM_ACID},
    {"charge", "charge", DAM_BASH},    /* 15 */
    {"slap", "slap", DAM_BASH},
    {"punch", "punch", DAM_BASH},
    {"wrath", "wrath", DAM_ENERGY},
    {"magic", "magic", DAM_ENERGY},
    {"divine", "divine power", DAM_HOLY},    /* 20 */
    {"cleave", "cleave", DAM_SLASH},
    {"scratch", "scratch", DAM_PIERCE},
    {"peck", "peck", DAM_PIERCE},
    {"peckb", "peck", DAM_BASH},
    {"chop", "chop", DAM_SLASH},    /* 25 */
    {"sting", "sting", DAM_PIERCE},
    {"smash", "smash", DAM_BASH},
    {"shbite", "shocking bite", DAM_LIGHTNING},
    {"flbite", "flaming bite", DAM_FIRE},
    {"frbite", "freezing bite", DAM_COLD},    /* 30 */
    {"acbite", "acidic bite", DAM_ACID},
    {"chomp", "chomp", DAM_PIERCE},
    {"drain", "life drain", DAM_NEGATIVE},
    {"thrust", "thrust", DAM_PIERCE},
    {"slime", "slime", DAM_ACID},
    {"shock", "shock", DAM_LIGHTNING},
    {"thwack", "thwack", DAM_BASH},
    {"flame", "flame", DAM_FIRE},
    {"chill", "chill", DAM_COLD},
    {NULL, NULL, 0}
};

/* race table */
const struct race_type race_table[] = {
/*
    {
    name,        pc_race?,
    act bits,    aff_by bits,    off bits,
    imm,        res,        vuln,
    form,        parts 
    },
*/
    {"unique", FALSE, 0, 0, 0, 0, 0, 0, 0, 0},

    {
     "human", TRUE,
     0, 0, 0,
     0, 0, 0,
     A | H | M | V, A | B | C | D | E | F | G | H | I | J | K},

    {
     "elf", TRUE,
     0, AFF_INFRARED, 0,
     0, RES_CHARM, VULN_IRON,
     A | H | M | V, A | B | C | D | E | F | G | H | I | J | K},

    {
     "dwarf", TRUE,
     0, AFF_INFRARED, 0,
     0, RES_POISON | RES_DISEASE, VULN_DROWNING,
     A | H | M | V, A | B | C | D | E | F | G | H | I | J | K},

    {
     "giant", TRUE,
     0, 0, 0,
     0, RES_FIRE | RES_COLD, VULN_MENTAL | VULN_LIGHTNING,
     A | H | M | V, A | B | C | D | E | F | G | H | I | J | K},

    {
     "bat", FALSE,
     0, AFF_FLYING | AFF_DARK_VISION, OFF_DODGE | OFF_FAST,
     0, 0, VULN_LIGHT,
     A | G | V, A | C | D | E | F | H | J | K | P},

    {
     "bear", FALSE,
     0, 0, OFF_CRUSH | OFF_DISARM | OFF_BERSERK,
     0, RES_BASH | RES_COLD, 0,
     A | G | V, A | B | C | D | E | F | H | J | K | U | V},

    {
     "cat", FALSE,
     0, AFF_DARK_VISION, OFF_FAST | OFF_DODGE,
     0, 0, 0,
     A | G | V, A | C | D | E | F | H | J | K | Q | U | V},

    {
     "centipede", FALSE,
     0, AFF_DARK_VISION, 0,
     0, RES_PIERCE | RES_COLD, VULN_BASH,
     A | B | G | O, A | C | K},

    {
     "dog", FALSE,
     0, 0, OFF_FAST,
     0, 0, 0,
     A | G | V, A | C | D | E | F | H | J | K | U | V},

    {
     "doll", FALSE,
     0, 0, 0,
     IMM_COLD | IMM_POISON | IMM_HOLY | IMM_NEGATIVE | IMM_MENTAL |
     IMM_DISEASE | IMM_DROWNING, RES_BASH | RES_LIGHT,
     VULN_SLASH | VULN_FIRE | VULN_ACID | VULN_LIGHTNING | VULN_ENERGY,
     E | J | M | cc, A | B | C | G | H | K},

    {"dragon", FALSE,
     0, AFF_INFRARED | AFF_FLYING, 0,
     0, RES_FIRE | RES_BASH | RES_CHARM,
     VULN_PIERCE | VULN_COLD,
     A | H | Z, A | C | D | E | F | G | H | I | J | K | P | Q | U | V | X},

    {
     "fido", FALSE,
     0, 0, OFF_DODGE | ASSIST_RACE,
     0, 0, VULN_MAGIC,
     A | B | G | V, A | C | D | E | F | H | J | K | Q | V},

    {
     "fox", FALSE,
     0, AFF_DARK_VISION, OFF_FAST | OFF_DODGE,
     0, 0, 0,
     A | G | V, A | C | D | E | F | H | J | K | Q | V},

    {
     "goblin", FALSE,
     0, AFF_INFRARED, 0,
     0, RES_DISEASE, VULN_MAGIC,
     A | H | M | V, A | B | C | D | E | F | G | H | I | J | K},

    {
     "hobgoblin", FALSE,
     0, AFF_INFRARED, 0,
     0, RES_DISEASE | RES_POISON, 0,
     A | H | M | V, A | B | C | D | E | F | G | H | I | J | K | Y},

    {
     "kobold", FALSE,
     0, AFF_INFRARED, 0,
     0, RES_POISON, VULN_MAGIC,
     A | B | H | M | V, A | B | C | D | E | F | G | H | I | J | K | Q},

    {
     "lizard", FALSE,
     0, 0, 0,
     0, RES_POISON, VULN_COLD,
     A | G | X | cc, A | C | D | E | F | H | K | Q | V},

    {
     "modron", FALSE,
     0, AFF_INFRARED, ASSIST_RACE | ASSIST_ALIGN,
     IMM_CHARM | IMM_DISEASE | IMM_MENTAL | IMM_HOLY | IMM_NEGATIVE,
     RES_FIRE | RES_COLD | RES_ACID, 0,
     H, A | B | C | G | H | J | K},

    {
     "orc", FALSE,
     0, AFF_INFRARED, 0,
     0, RES_DISEASE, VULN_LIGHT,
     A | H | M | V, A | B | C | D | E | F | G | H | I | J | K},

    {
     "pig", FALSE,
     0, 0, 0,
     0, 0, 0,
     A | G | V, A | C | D | E | F | H | J | K},

    {
     "rabbit", FALSE,
     0, 0, OFF_DODGE | OFF_FAST,
     0, 0, 0,
     A | G | V, A | C | D | E | F | H | J | K},

    {
     "school monster", FALSE,
     ACT_NOALIGN, 0, 0,
     IMM_CHARM | IMM_SUMMON, 0, VULN_MAGIC,
     A | M | V, A | B | C | D | E | F | H | J | K | Q | U},

    {
     "snake", FALSE,
     0, 0, 0,
     0, RES_POISON, VULN_COLD,
     A | G | X | Y | cc, A | D | E | F | K | L | Q | V | X},

    {
     "song bird", FALSE,
     0, AFF_FLYING, OFF_FAST | OFF_DODGE,
     0, 0, 0,
     A | G | W, A | C | D | E | F | H | K | P},

    {
     "troll", FALSE,
     0, AFF_REGENERATION | AFF_INFRARED | AFF_DETECT_HIDDEN,
     OFF_BERSERK,
     0, RES_CHARM | RES_BASH, VULN_FIRE | VULN_ACID,
     A | B | H | M | V, A | B | C | D | E | F | G | H | I | J | K | U | V},

    {
     "water fowl", FALSE,
     0, AFF_SWIM | AFF_FLYING, 0,
     0, RES_DROWNING, 0,
     A | G | W, A | C | D | E | F | H | K | P},

    {
     "wolf", FALSE,
     0, AFF_DARK_VISION, OFF_FAST | OFF_DODGE,
     0, 0, 0,
     A | G | V, A | C | D | E | F | J | K | Q | V},

    {
     "wyvern", FALSE,
     0, AFF_FLYING | AFF_DETECT_INVIS | AFF_DETECT_HIDDEN,
     OFF_BASH | OFF_FAST | OFF_DODGE,
     IMM_POISON, 0, VULN_LIGHT,
     A | B | G | Z, A | C | D | E | F | H | J | K | Q | V | X},

    {
     "unique", FALSE,
     0, 0, 0,
     0, 0, 0,
     0, 0},


    {
     NULL, 0, 0, 0, 0, 0, 0}
};

const struct pc_race_type pc_race_table[] = {
    {"null race", "", 0, {100, 100, 100, 100},
     {""}, {13, 13, 13, 13, 13}, {18, 18, 18, 18, 18}, 0},

/*
    {
    "race name",     short name,     points,    { class multipliers },
    { bonus skills },
    { base stats },        { max stats },        size 
    },
*/
    {
     "human", "Human", 0, {100, 100, 100, 100},
     {""},
     {13, 13, 13, 13, 13}, {18, 18, 18, 18, 18}, SIZE_MEDIUM},

    {
     "elf", " Elf ", 5, {100, 125, 100, 120},
     {"sneak", "hide"},
     {12, 14, 13, 15, 11}, {16, 20, 18, 21, 15}, SIZE_SMALL},

    {
     "dwarf", "Dwarf", 8, {150, 100, 125, 100},
     {"berserk"},
     {14, 12, 14, 10, 15}, {20, 16, 19, 14, 21}, SIZE_MEDIUM},

    {
     "giant", "Giant", 6, {200, 150, 150, 105},
     {"bash", "fast healing"},
     {16, 11, 13, 11, 14}, {22, 15, 18, 15, 20}, SIZE_LARGE}
};




/*
 * Class table.
 */
const struct class_type class_table[MAX_CLASS] = {
    {
     "mage", "Mag", STAT_INT, OBJ_VNUM_SCHOOL_DAGGER,
     {3018, 9618}, 75, 20, 6, 6, 8, TRUE,
     "mage basics", "mage default"},

    {
     "cleric", "Cle", STAT_WIS, OBJ_VNUM_SCHOOL_MACE,
     {3003, 9619}, 75, 20, 2, 7, 10, TRUE,
     "cleric basics", "cleric default"},

    {
     "thief", "Thi", STAT_DEX, OBJ_VNUM_SCHOOL_DAGGER,
     {3028, 9639}, 75, 20, -4, 8, 13, FALSE,
     "thief basics", "thief default"},

    {
     "warrior", "War", STAT_STR, OBJ_VNUM_SCHOOL_SWORD,
     {3022, 9633}, 75, 20, -10, 11, 15, FALSE,
     "warrior basics", "warrior default"}
};



/*
 * Titles.
 */
char *const title_table[MAX_CLASS][MAX_LEVEL + 1][2] = {
    {
     {"Man", "Woman"},

     {"Apprentice of Magic", "Apprentice of Magic"},
     {"Spell Student", "Spell Student"},
     {"Scholar of Magic", "Scholar of Magic"},
     {"Delver in Spells", "Delveress in Spells"},
     {"Medium of Magic", "Medium of Magic"},

     {"Scribe of Magic", "Scribess of Magic"},
     {"Seer", "Seeress"},
     {"Sage", "Sage"},
     {"Illusionist", "Illusionist"},
     {"Abjurer", "Abjuress"},

     {"Invoker", "Invoker"},
     {"Enchanter", "Enchantress"},
     {"Conjurer", "Conjuress"},
     {"Magician", "Witch"},
     {"Creator", "Creator"},

     {"Savant", "Savant"},
     {"Magus", "Craftess"},
     {"Wizard", "Wizard"},
     {"Warlock", "War Witch"},
     {"Sorcerer", "Sorceress"},

     {"Elder Sorcerer", "Elder Sorceress"},
     {"Grand Sorcerer", "Grand Sorceress"},
     {"Great Sorcerer", "Great Sorceress"},
     {"Golem Maker", "Golem Maker"},
     {"Greater Golem Maker", "Greater Golem Maker"},

     {"Maker of Stones", "Maker of Stones",},
     {"Maker of Potions", "Maker of Potions",},
     {"Maker of Scrolls", "Maker of Scrolls",},
     {"Maker of Wands", "Maker of Wands",},
     {"Maker of Staves", "Maker of Staves",},

     {"Demon Summoner", "Demon Summoner"},
     {"Greater Demon Summoner", "Greater Demon Summoner"},
     {"Dragon Charmer", "Dragon Charmer"},
     {"Greater Dragon Charmer", "Greater Dragon Charmer"},
     {"Master of all Magic", "Master of all Magic"},

     {"Master Mage", "Master Mage"},
     {"Master Mage", "Master Mage"},
     {"Master Mage", "Master Mage"},
     {"Master Mage", "Master Mage"},
     {"Master Mage", "Master Mage"},

     {"Master Mage", "Master Mage"},
     {"Master Mage", "Master Mage"},
     {"Master Mage", "Master Mage"},
     {"Master Mage", "Master Mage"},
     {"Master Mage", "Master Mage"},

     {"Master Mage", "Master Mage"},
     {"Master Mage", "Master Mage"},
     {"Master Mage", "Master Mage"},
     {"Master Mage", "Master Mage"},
     {"Master Mage", "Master Mage"},

     {"Mage Hero", "Mage Heroine"},
     {"Avatar of Magic", "Avatar of Magic"},
     {"Angel of Magic", "Angel of Magic"},
     {"Demigod of Magic", "Demigoddess of Magic"},
     {"Immortal of Magic", "Immortal of Magic"},
     {"God of Magic", "Goddess of Magic"},
     {"Deity of Magic", "Deity of Magic"},
     {"Supremity of Magic", "Supremity of Magic"},
     {"Creator", "Creator"},
     {"Implementor", "Implementress"}
     },

    {
     {"Man", "Woman"},

     {"Believer", "Believer"},
     {"Attendant", "Attendant"},
     {"Acolyte", "Acolyte"},
     {"Novice", "Novice"},
     {"Missionary", "Missionary"},

     {"Adept", "Adept"},
     {"Deacon", "Deaconess"},
     {"Vicar", "Vicaress"},
     {"Priest", "Priestess"},
     {"Minister", "Lady Minister"},

     {"Canon", "Canon"},
     {"Levite", "Levitess"},
     {"Curate", "Curess"},
     {"Monk", "Nun"},
     {"Healer", "Healess"},

     {"Chaplain", "Chaplain"},
     {"Expositor", "Expositress"},
     {"Bishop", "Bishop"},
     {"Arch Bishop", "Arch Lady of the Church"},
     {"Patriarch", "Matriarch"},

     {"Elder Patriarch", "Elder Matriarch"},
     {"Grand Patriarch", "Grand Matriarch"},
     {"Great Patriarch", "Great Matriarch"},
     {"Demon Killer", "Demon Killer"},
     {"Greater Demon Killer", "Greater Demon Killer"},

     {"Cardinal of the Sea", "Cardinal of the Sea"},
     {"Cardinal of the Earth", "Cardinal of the Earth"},
     {"Cardinal of the Air", "Cardinal of the Air"},
     {"Cardinal of the Ether", "Cardinal of the Ether"},
     {"Cardinal of the Heavens", "Cardinal of the Heavens"},

     {"Avatar of an Immortal", "Avatar of an Immortal"},
     {"Avatar of a Deity", "Avatar of a Deity"},
     {"Avatar of a Supremity", "Avatar of a Supremity"},
     {"Avatar of an Implementor", "Avatar of an Implementor"},
     {"Master of all Divinity", "Mistress of all Divinity"},

     {"Master Cleric", "Master Cleric"},
     {"Master Cleric", "Master Cleric"},
     {"Master Cleric", "Master Cleric"},
     {"Master Cleric", "Master Cleric"},
     {"Master Cleric", "Master Cleric"},

     {"Master Cleric", "Master Cleric"},
     {"Master Cleric", "Master Cleric"},
     {"Master Cleric", "Master Cleric"},
     {"Master Cleric", "Master Cleric"},
     {"Master Cleric", "Master Cleric"},

     {"Master Cleric", "Master Cleric"},
     {"Master Cleric", "Master Cleric"},
     {"Master Cleric", "Master Cleric"},
     {"Master Cleric", "Master Cleric"},
     {"Master Cleric", "Master Cleric"},

     {"Holy Hero", "Holy Heroine"},
     {"Holy Avatar", "Holy Avatar"},
     {"Angel", "Angel"},
     {"Demigod", "Demigoddess",},
     {"Immortal", "Immortal"},
     {"God", "Goddess"},
     {"Deity", "Deity"},
     {"Supreme Master", "Supreme Mistress"},
     {"Creator", "Creator"},
     {"Implementor", "Implementress"}
     },

    {
     {"Man", "Woman"},

     {"Pilferer", "Pilferess"},
     {"Footpad", "Footpad"},
     {"Filcher", "Filcheress"},
     {"Pick-Pocket", "Pick-Pocket"},
     {"Sneak", "Sneak"},

     {"Pincher", "Pincheress"},
     {"Cut-Purse", "Cut-Purse"},
     {"Snatcher", "Snatcheress"},
     {"Sharper", "Sharpress"},
     {"Rogue", "Rogue"},

     {"Robber", "Robber"},
     {"Magsman", "Magswoman"},
     {"Highwayman", "Highwaywoman"},
     {"Burglar", "Burglaress"},
     {"Thief", "Thief"},

     {"Knifer", "Knifer"},
     {"Quick-Blade", "Quick-Blade"},
     {"Killer", "Murderess"},
     {"Brigand", "Brigand"},
     {"Cut-Throat", "Cut-Throat"},

     {"Spy", "Spy"},
     {"Grand Spy", "Grand Spy"},
     {"Master Spy", "Master Spy"},
     {"Assassin", "Assassin"},
     {"Greater Assassin", "Greater Assassin"},

     {"Master of Vision", "Mistress of Vision"},
     {"Master of Hearing", "Mistress of Hearing"},
     {"Master of Smell", "Mistress of Smell"},
     {"Master of Taste", "Mistress of Taste"},
     {"Master of Touch", "Mistress of Touch"},

     {"Crime Lord", "Crime Mistress"},
     {"Infamous Crime Lord", "Infamous Crime Mistress"},
     {"Greater Crime Lord", "Greater Crime Mistress"},
     {"Master Crime Lord", "Master Crime Mistress"},
     {"Godfather", "Godmother"},

     {"Master Thief", "Master Thief"},
     {"Master Thief", "Master Thief"},
     {"Master Thief", "Master Thief"},
     {"Master Thief", "Master Thief"},
     {"Master Thief", "Master Thief"},

     {"Master Thief", "Master Thief"},
     {"Master Thief", "Master Thief"},
     {"Master Thief", "Master Thief"},
     {"Master Thief", "Master Thief"},
     {"Master Thief", "Master Thief"},

     {"Master Thief", "Master Thief"},
     {"Master Thief", "Master Thief"},
     {"Master Thief", "Master Thief"},
     {"Master Thief", "Master Thief"},
     {"Master Thief", "Master Thief"},

     {"Assassin Hero", "Assassin Heroine"},
     {"Avatar of Death", "Avatar of Death",},
     {"Angel of Death", "Angel of Death"},
     {"Demigod of Assassins", "Demigoddess of Assassins"},
     {"Immortal Assasin", "Immortal Assassin"},
     {"God of Assassins", "God of Assassins",},
     {"Deity of Assassins", "Deity of Assassins"},
     {"Supreme Master", "Supreme Mistress"},
     {"Creator", "Creator"},
     {"Implementor", "Implementress"}
     },

    {
     {"Man", "Woman"},

     {"Swordpupil", "Swordpupil"},
     {"Recruit", "Recruit"},
     {"Sentry", "Sentress"},
     {"Fighter", "Fighter"},
     {"Soldier", "Soldier"},

     {"Warrior", "Warrior"},
     {"Veteran", "Veteran"},
     {"Swordsman", "Swordswoman"},
     {"Fencer", "Fenceress"},
     {"Combatant", "Combatess"},

     {"Hero", "Heroine"},
     {"Myrmidon", "Myrmidon"},
     {"Swashbuckler", "Swashbuckleress"},
     {"Mercenary", "Mercenaress"},
     {"Swordmaster", "Swordmistress"},

     {"Lieutenant", "Lieutenant"},
     {"Champion", "Lady Champion"},
     {"Dragoon", "Lady Dragoon"},
     {"Cavalier", "Lady Cavalier"},
     {"Knight", "Lady Knight"},

     {"Grand Knight", "Grand Knight"},
     {"Master Knight", "Master Knight"},
     {"Paladin", "Paladin"},
     {"Grand Paladin", "Grand Paladin"},
     {"Demon Slayer", "Demon Slayer"},

     {"Greater Demon Slayer", "Greater Demon Slayer"},
     {"Dragon Slayer", "Dragon Slayer"},
     {"Greater Dragon Slayer", "Greater Dragon Slayer"},
     {"Underlord", "Underlord"},
     {"Overlord", "Overlord"},

     {"Baron of Thunder", "Baroness of Thunder"},
     {"Baron of Storms", "Baroness of Storms"},
     {"Baron of Tornadoes", "Baroness of Tornadoes"},
     {"Baron of Hurricanes", "Baroness of Hurricanes"},
     {"Baron of Meteors", "Baroness of Meteors"},

     {"Master Warrior", "Master Warrior"},
     {"Master Warrior", "Master Warrior"},
     {"Master Warrior", "Master Warrior"},
     {"Master Warrior", "Master Warrior"},
     {"Master Warrior", "Master Warrior"},

     {"Master Warrior", "Master Warrior"},
     {"Master Warrior", "Master Warrior"},
     {"Master Warrior", "Master Warrior"},
     {"Master Warrior", "Master Warrior"},
     {"Master Warrior", "Master Warrior"},

     {"Master Warrior", "Master Warrior"},
     {"Master Warrior", "Master Warrior"},
     {"Master Warrior", "Master Warrior"},
     {"Master Warrior", "Master Warrior"},
     {"Master Warrior", "Master Warrior"},

     {"Knight Hero", "Knight Heroine"},
     {"Avatar of War", "Avatar of War"},
     {"Angel of War", "Angel of War"},
     {"Demigod of War", "Demigoddess of War"},
     {"Immortal Warlord", "Immortal Warlord"},
     {"God of War", "God of War"},
     {"Deity of War", "Deity of War"},
     {"Supreme Master of War", "Supreme Mistress of War"},
     {"Creator", "Creator"},
     {"Implementor", "Implementress"}
     }
};



/*
 * Attribute bonus tables.
 */
const struct str_app_type str_app[26] = {
    {-5, -4, 0, 0},                /* 0  */
    {-5, -4, 3, 1},                /* 1  */
    {-3, -2, 3, 2},
    {-3, -1, 10, 3},            /* 3  */
    {-2, -1, 25, 4},
    {-2, -1, 55, 5},            /* 5  */
    {-1, 0, 80, 6},
    {-1, 0, 90, 7},
    {0, 0, 100, 8},
    {0, 0, 100, 9},
    {0, 0, 115, 10},            /* 10  */
    {0, 0, 115, 11},
    {0, 0, 130, 12},
    {0, 0, 130, 13},            /* 13  */
    {0, 1, 140, 14},
    {1, 1, 150, 15},            /* 15  */
    {1, 2, 165, 16},
    {2, 3, 180, 22},
    {2, 3, 200, 25},            /* 18  */
    {3, 4, 225, 30},
    {3, 5, 250, 35},            /* 20  */
    {4, 6, 300, 40},
    {4, 6, 350, 45},
    {5, 7, 400, 50},
    {5, 8, 450, 55},
    {6, 9, 500, 60}                /* 25   */
};



const struct int_app_type int_app[26] = {
    {3},                        /*  0 */
    {5},                        /*  1 */
    {7},
    {8},                        /*  3 */
    {9},
    {10},                        /*  5 */
    {11},
    {12},
    {13},
    {15},
    {17},                        /* 10 */
    {19},
    {22},
    {25},
    {28},
    {31},                        /* 15 */
    {34},
    {37},
    {40},                        /* 18 */
    {44},
    {49},                        /* 20 */
    {55},
    {60},
    {70},
    {80},
    {85}                        /* 25 */
};



const struct wis_app_type wis_app[26] = {
    {0},                        /*  0 */
    {0},                        /*  1 */
    {0},
    {0},                        /*  3 */
    {0},
    {1},                        /*  5 */
    {1},
    {1},
    {1},
    {1},
    {1},                        /* 10 */
    {1},
    {1},
    {1},
    {1},
    {2},                        /* 15 */
    {2},
    {2},
    {3},                        /* 18 */
    {3},
    {3},                        /* 20 */
    {3},
    {4},
    {4},
    {4},
    {5}                            /* 25 */
};



const struct dex_app_type dex_app[26] = {
    {60},                        /* 0 */
    {50},                        /* 1 */
    {50},
    {40},
    {30},
    {20},                        /* 5 */
    {10},
    {0},
    {0},
    {0},
    {0},                        /* 10 */
    {0},
    {0},
    {0},
    {0},
    {-10},                        /* 15 */
    {-15},
    {-20},
    {-30},
    {-40},
    {-50},                        /* 20 */
    {-60},
    {-75},
    {-90},
    {-105},
    {-120}                        /* 25 */
};


const struct con_app_type con_app[26] = {
    {-4, 20},                    /*  0 */
    {-3, 25},                    /*  1 */
    {-2, 30},
    {-2, 35},                    /*  3 */
    {-1, 40},
    {-1, 45},                    /*  5 */
    {-1, 50},
    {0, 55},
    {0, 60},
    {0, 65},
    {0, 70},                    /* 10 */
    {0, 75},
    {0, 80},
    {0, 85},
    {0, 88},
    {1, 90},                    /* 15 */
    {2, 95},
    {2, 97},
    {3, 99},                    /* 18 */
    {3, 99},
    {4, 99},                    /* 20 */
    {4, 99},
    {5, 99},
    {6, 99},
    {7, 99},
    {8, 99}                        /* 25 */
};



/*
 * Liquid properties.
 * Used in world.obj.
 */
const struct liq_type liq_table[] = {
/*    name            color    proof, full, thirst, food, ssize */
    {"water", "clear", {0, 1, 10, 0, 16}},
    {"beer", "amber", {12, 1, 8, 1, 12}},
    {"red wine", "burgundy", {30, 1, 8, 1, 5}},
    {"ale", "brown", {15, 1, 8, 1, 12}},
    {"dark ale", "dark", {16, 1, 8, 1, 12}},

    {"whisky", "golden", {120, 1, 5, 0, 2}},
    {"lemonade", "pink", {0, 1, 9, 2, 12}},
    {"firebreather", "boiling", {190, 0, 4, 0, 2}},
    {"local specialty", "clear", {151, 1, 3, 0, 2}},
    {"slime mold juice", "green", {0, 2, -8, 1, 2}},

    {"milk", "white", {0, 2, 9, 3, 12}},
    {"tea", "tan", {0, 1, 8, 0, 6}},
    {"coffee", "black", {0, 1, 8, 0, 6}},
    {"blood", "red", {0, 2, -1, 2, 6}},
    {"salt water", "clear", {0, 1, -2, 0, 1}},

    {"coke", "brown", {0, 2, 9, 2, 12}},
    {"root beer", "brown", {0, 2, 9, 2, 12}},
    {"elvish wine", "green", {35, 2, 8, 1, 5}},
    {"white wine", "golden", {28, 1, 8, 1, 5}},
    {"champagne", "golden", {32, 1, 8, 1, 5}},

    {"mead", "honey-colored", {34, 2, 8, 2, 12}},
    {"rose wine", "pink", {26, 1, 8, 1, 5}},
    {"benedictine wine", "burgundy", {40, 1, 8, 1, 5}},
    {"vodka", "clear", {130, 1, 5, 0, 2}},
    {"cranberry juice", "red", {0, 1, 9, 2, 12}},

    {"orange juice", "orange", {0, 2, 9, 3, 12}},
    {"absinthe", "green", {200, 1, 4, 0, 2}},
    {"brandy", "golden", {80, 1, 5, 0, 4}},
    {"aquavit", "clear", {140, 1, 5, 0, 2}},
    {"schnapps", "clear", {90, 1, 5, 0, 2}},

    {"icewine", "purple", {50, 2, 6, 1, 5}},
    {"amontillado", "burgundy", {35, 2, 8, 1, 5}},
    {"sherry", "red", {38, 2, 7, 1, 5}},
    {"framboise", "red", {50, 1, 7, 1, 5}},
    {"rum", "amber", {151, 1, 4, 0, 2}},

    {"cordial", "clear", {100, 1, 5, 0, 2}},
    {NULL, NULL, {0, 0, 0, 0, 0}}
};



/*
 * The skill and spell table.
 * Slot numbers must never be changed as they appear in #OBJECTS sections.
 */
#define SLOT(n)    n

const struct skill_type skill_table[MAX_SKILL] = {

/*
 * Magic spells.
 */

    {
     "reserved", {99, 99, 99, 99}, {99, 99, 99, 99},
     0, TAR_IGNORE, POS_STANDING,
     NULL, SLOT (0), 0, 0,
     "", "", ""},

    {
     "acid blast", {28, 53, 35, 32}, {1, 1, 2, 2},
     spell_acid_blast, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT (70), 20, 12,
     "acid blast", "!Acid Blast!"},

    {
     "armor", {7, 2, 10, 5}, {1, 1, 2, 2},
     spell_armor, TAR_CHAR_DEFENSIVE, POS_STANDING,
     NULL, SLOT (1), 5, 12,
     "", "You feel less armored.", ""},

    {
     "bless", {53, 7, 53, 8}, {1, 1, 2, 2},
     spell_bless, TAR_OBJ_CHAR_DEF, POS_STANDING,
     NULL, SLOT (3), 5, 12,
     "", "You feel less righteous.",
     "$p's holy aura fades."},

    {
     "blindness", {12, 8, 17, 15}, {1, 1, 2, 2},
     spell_blindness, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     &gsn_blindness, SLOT (4), 5, 12,
     "", "You can see again.", ""},

    {
     "burning hands", {7, 53, 10, 9}, {1, 1, 2, 2},
     spell_burning_hands, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT (5), 15, 12,
     "burning hands", "!Burning Hands!", ""},

    {
     "call lightning", {26, 18, 31, 22}, {1, 1, 2, 2},
     spell_call_lightning, TAR_IGNORE, POS_FIGHTING,
     NULL, SLOT (6), 15, 12,
     "lightning bolt", "!Call Lightning!", ""},

    {"calm", {48, 16, 50, 20}, {1, 1, 2, 2},
     spell_calm, TAR_IGNORE, POS_FIGHTING,
     NULL, SLOT (509), 30, 12,
     "", "You have lost your peace of mind.", ""},

    {
     "cancellation", {18, 26, 34, 34}, {1, 1, 2, 2},
     spell_cancellation, TAR_CHAR_DEFENSIVE, POS_FIGHTING,
     NULL, SLOT (507), 20, 12,
     "" "!cancellation!", ""},

    {
     "cause critical", {53, 13, 53, 19}, {1, 1, 2, 2},
     spell_cause_critical, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT (63), 20, 12,
     "spell", "!Cause Critical!", ""},

    {
     "cause light", {53, 1, 53, 3}, {1, 1, 2, 2},
     spell_cause_light, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT (62), 15, 12,
     "spell", "!Cause Light!", ""},

    {
     "cause serious", {53, 7, 53, 10}, {1, 1, 2, 2},
     spell_cause_serious, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT (64), 17, 12,
     "spell", "!Cause Serious!", ""},

    {
     "chain lightning", {33, 53, 39, 36}, {1, 1, 2, 2},
     spell_chain_lightning, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT (500), 25, 12,
     "lightning", "!Chain Lightning!", ""},

    {
     "change sex", {53, 53, 53, 53}, {1, 1, 2, 2},
     spell_change_sex, TAR_CHAR_DEFENSIVE, POS_FIGHTING,
     NULL, SLOT (82), 15, 12,
     "", "Your body feels familiar again.", ""},

    {
     "charm person", {20, 53, 25, 53}, {1, 1, 2, 2},
     spell_charm_person, TAR_CHAR_OFFENSIVE, POS_STANDING,
     &gsn_charm_person, SLOT (7), 5, 12,
     "", "You feel more self-confident.", ""},

    {
     "chill touch", {4, 53, 6, 6}, {1, 1, 2, 2},
     spell_chill_touch, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT (8), 15, 12,
     "chilling touch", "You feel less cold.", ""},

    {
     "colour spray", {16, 53, 22, 20}, {1, 1, 2, 2},
     spell_colour_spray, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT (10), 15, 12,
     "colour spray", "!Colour Spray!", ""},

    {
     "continual light", {6, 4, 6, 9}, {1, 1, 2, 2},
     spell_continual_light, TAR_IGNORE, POS_STANDING,
     NULL, SLOT (57), 7, 12,
     "", "!Continual Light!", ""},

    {
     "control weather", {15, 19, 28, 22}, {1, 1, 2, 2},
     spell_control_weather, TAR_IGNORE, POS_STANDING,
     NULL, SLOT (11), 25, 12,
     "", "!Control Weather!", ""},

    {
     "create food", {10, 5, 11, 12}, {1, 1, 2, 2},
     spell_create_food, TAR_IGNORE, POS_STANDING,
     NULL, SLOT (12), 5, 12,
     "", "!Create Food!", ""},

    {
     "create rose", {16, 11, 10, 24}, {1, 1, 2, 2},
     spell_create_rose, TAR_IGNORE, POS_STANDING,
     NULL, SLOT (511), 30, 12,
     "", "!Create Rose!", ""},

    {
     "create spring", {14, 17, 23, 20}, {1, 1, 2, 2},
     spell_create_spring, TAR_IGNORE, POS_STANDING,
     NULL, SLOT (80), 20, 12,
     "", "!Create Spring!", ""},

    {
     "create water", {8, 3, 12, 11}, {1, 1, 2, 2},
     spell_create_water, TAR_OBJ_INV, POS_STANDING,
     NULL, SLOT (13), 5, 12,
     "", "!Create Water!", ""},

    {
     "cure blindness", {53, 6, 53, 8}, {1, 1, 2, 2},
     spell_cure_blindness, TAR_CHAR_DEFENSIVE, POS_FIGHTING,
     NULL, SLOT (14), 5, 12,
     "", "!Cure Blindness!", ""},

    {
     "cure critical", {53, 13, 53, 19}, {1, 1, 2, 2},
     spell_cure_critical, TAR_CHAR_DEFENSIVE, POS_FIGHTING,
     NULL, SLOT (15), 20, 12,
     "", "!Cure Critical!", ""},

    {
     "cure disease", {53, 13, 53, 14}, {1, 1, 2, 2},
     spell_cure_disease, TAR_CHAR_DEFENSIVE, POS_STANDING,
     NULL, SLOT (501), 20, 12,
     "", "!Cure Disease!", ""},

    {
     "cure light", {53, 1, 53, 3}, {1, 1, 2, 2},
     spell_cure_light, TAR_CHAR_DEFENSIVE, POS_FIGHTING,
     NULL, SLOT (16), 10, 12,
     "", "!Cure Light!", ""},

    {
     "cure poison", {53, 14, 53, 16}, {1, 1, 2, 2},
     spell_cure_poison, TAR_CHAR_DEFENSIVE, POS_STANDING,
     NULL, SLOT (43), 5, 12,
     "", "!Cure Poison!", ""},

    {
     "cure serious", {53, 7, 53, 10}, {1, 1, 2, 2},
     spell_cure_serious, TAR_CHAR_DEFENSIVE, POS_FIGHTING,
     NULL, SLOT (61), 15, 12,
     "", "!Cure Serious!", ""},

    {
     "curse", {18, 18, 26, 22}, {1, 1, 2, 2},
     spell_curse, TAR_OBJ_CHAR_OFF, POS_FIGHTING,
     &gsn_curse, SLOT (17), 20, 12,
     "curse", "The curse wears off.",
     "$p is no longer impure."},

    {
     "demonfire", {53, 34, 53, 45}, {1, 1, 2, 2},
     spell_demonfire, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT (505), 20, 12,
     "torments", "!Demonfire!", ""},

    {
     "detect evil", {11, 4, 12, 53}, {1, 1, 2, 2},
     spell_detect_evil, TAR_CHAR_SELF, POS_STANDING,
     NULL, SLOT (18), 5, 12,
     "", "The red in your vision disappears.", ""},

    {
     "detect good", {11, 4, 12, 53}, {1, 1, 2, 2},
     spell_detect_good, TAR_CHAR_SELF, POS_STANDING,
     NULL, SLOT (513), 5, 12,
     "", "The gold in your vision disappears.", ""},

    {
     "detect hidden", {15, 11, 12, 53}, {1, 1, 2, 2},
     spell_detect_hidden, TAR_CHAR_SELF, POS_STANDING,
     NULL, SLOT (44), 5, 12,
     "", "You feel less aware of your surroundings.",
     ""},

    {
     "detect invis", {3, 8, 6, 53}, {1, 1, 2, 2},
     spell_detect_invis, TAR_CHAR_SELF, POS_STANDING,
     NULL, SLOT (19), 5, 12,
     "", "You no longer see invisible objects.",
     ""},

    {
     "detect magic", {2, 6, 5, 53}, {1, 1, 2, 2},
     spell_detect_magic, TAR_CHAR_SELF, POS_STANDING,
     NULL, SLOT (20), 5, 12,
     "", "The detect magic wears off.", ""},

    {
     "detect poison", {15, 7, 9, 53}, {1, 1, 2, 2},
     spell_detect_poison, TAR_OBJ_INV, POS_STANDING,
     NULL, SLOT (21), 5, 12,
     "", "!Detect Poison!", ""},

    {
     "dispel evil", {53, 15, 53, 21}, {1, 1, 2, 2},
     spell_dispel_evil, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT (22), 15, 12,
     "dispel evil", "!Dispel Evil!", ""},

    {
     "dispel good", {53, 15, 53, 21}, {1, 1, 2, 2},
     spell_dispel_good, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT (512), 15, 12,
     "dispel good", "!Dispel Good!", ""},

    {
     "dispel magic", {16, 24, 30, 30}, {1, 1, 2, 2},
     spell_dispel_magic, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT (59), 15, 12,
     "", "!Dispel Magic!", ""},

    {
     "earthquake", {53, 10, 53, 14}, {1, 1, 2, 2},
     spell_earthquake, TAR_IGNORE, POS_FIGHTING,
     NULL, SLOT (23), 15, 12,
     "earthquake", "!Earthquake!", ""},

    {
     "enchant armor", {16, 53, 53, 53}, {2, 2, 4, 4},
     spell_enchant_armor, TAR_OBJ_INV, POS_STANDING,
     NULL, SLOT (510), 100, 24,
     "", "!Enchant Armor!", ""},

    {
     "enchant weapon", {17, 53, 53, 53}, {2, 2, 4, 4},
     spell_enchant_weapon, TAR_OBJ_INV, POS_STANDING,
     NULL, SLOT (24), 100, 24,
     "", "!Enchant Weapon!", ""},

    {
     "energy drain", {19, 22, 26, 23}, {1, 1, 2, 2},
     spell_energy_drain, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT (25), 35, 12,
     "energy drain", "!Energy Drain!", ""},

    {
     "faerie fire", {6, 3, 5, 8}, {1, 1, 2, 2},
     spell_faerie_fire, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT (72), 5, 12,
     "faerie fire", "The pink aura around you fades away.",
     ""},

    {
     "faerie fog", {14, 21, 16, 24}, {1, 1, 2, 2},
     spell_faerie_fog, TAR_IGNORE, POS_STANDING,
     NULL, SLOT (73), 12, 12,
     "faerie fog", "!Faerie Fog!", ""},

    {
     "farsight", {14, 16, 16, 53}, {1, 1, 2, 2},
     spell_farsight, TAR_IGNORE, POS_STANDING,
     NULL, SLOT (521), 36, 20,
     "farsight", "!Farsight!", ""},

    {
     "fireball", {22, 53, 30, 26}, {1, 1, 2, 2},
     spell_fireball, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT (26), 15, 12,
     "fireball", "!Fireball!", ""},

    {
     "fireproof", {13, 12, 19, 18}, {1, 1, 2, 2},
     spell_fireproof, TAR_OBJ_INV, POS_STANDING,
     NULL, SLOT (523), 10, 12,
     "", "", "$p's protective aura fades."},

    {
     "flamestrike", {53, 20, 53, 27}, {1, 1, 2, 2},
     spell_flamestrike, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT (65), 20, 12,
     "flamestrike", "!Flamestrike!", ""},

    {
     "fly", {10, 18, 20, 22}, {1, 1, 2, 2},
     spell_fly, TAR_CHAR_DEFENSIVE, POS_STANDING,
     NULL, SLOT (56), 10, 18,
     "", "You slowly float to the ground.", ""},

    {
     "floating disc", {4, 10, 7, 16}, {1, 1, 2, 2},
     spell_floating_disc, TAR_IGNORE, POS_STANDING,
     NULL, SLOT (522), 40, 24,
     "", "!Floating disc!", ""},

    {
     "frenzy", {53, 24, 53, 26}, {1, 1, 2, 2},
     spell_frenzy, TAR_CHAR_DEFENSIVE, POS_STANDING,
     NULL, SLOT (504), 30, 24,
     "", "Your rage ebbs.", ""},

    {
     "gate", {27, 17, 32, 28}, {1, 1, 2, 2},
     spell_gate, TAR_IGNORE, POS_FIGHTING,
     NULL, SLOT (83), 80, 12,
     "", "!Gate!", ""},

    {
     "giant strength", {11, 53, 22, 20}, {1, 1, 2, 2},
     spell_giant_strength, TAR_CHAR_DEFENSIVE, POS_STANDING,
     NULL, SLOT (39), 20, 12,
     "", "You feel weaker.", ""},

    {
     "harm", {53, 23, 53, 28}, {1, 1, 2, 2},
     spell_harm, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT (27), 35, 12,
     "harm spell", "!Harm!,        " ""},

    {
     "haste", {21, 53, 26, 29}, {1, 1, 2, 2},
     spell_haste, TAR_CHAR_DEFENSIVE, POS_FIGHTING,
     NULL, SLOT (502), 30, 12,
     "", "You feel yourself slow down.", ""},

    {
     "heal", {53, 21, 33, 30}, {1, 1, 2, 2},
     spell_heal, TAR_CHAR_DEFENSIVE, POS_FIGHTING,
     NULL, SLOT (28), 50, 12,
     "", "!Heal!", ""},

    {
     "heat metal", {53, 16, 53, 23}, {1, 1, 2, 2},
     spell_heat_metal, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT (516), 25, 18,
     "spell", "!Heat Metal!", ""},

    {
     "holy word", {53, 36, 53, 42}, {2, 2, 4, 4},
     spell_holy_word, TAR_IGNORE, POS_FIGHTING,
     NULL, SLOT (506), 200, 24,
     "divine wrath", "!Holy Word!", ""},

    {
     "identify", {15, 16, 18, 53}, {1, 1, 2, 2},
     spell_identify, TAR_OBJ_INV, POS_STANDING,
     NULL, SLOT (53), 12, 24,
     "", "!Identify!", ""},

    {
     "infravision", {9, 13, 10, 16}, {1, 1, 2, 2},
     spell_infravision, TAR_CHAR_DEFENSIVE, POS_STANDING,
     NULL, SLOT (77), 5, 18,
     "", "You no longer see in the dark.", ""},

    {
     "invisibility", {5, 53, 9, 53}, {1, 1, 2, 2},
     spell_invis, TAR_OBJ_CHAR_DEF, POS_STANDING,
     &gsn_invis, SLOT (29), 5, 12,
     "", "You are no longer invisible.",
     "$p fades into view."},

    {
     "know alignment", {12, 9, 20, 53}, {1, 1, 2, 2},
     spell_know_alignment, TAR_CHAR_DEFENSIVE, POS_FIGHTING,
     NULL, SLOT (58), 9, 12,
     "", "!Know Alignment!", ""},

    {
     "lightning bolt", {13, 23, 18, 16}, {1, 1, 2, 2},
     spell_lightning_bolt, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT (30), 15, 12,
     "lightning bolt", "!Lightning Bolt!", ""},

    {
     "locate object", {9, 15, 11, 53}, {1, 1, 2, 2},
     spell_locate_object, TAR_IGNORE, POS_STANDING,
     NULL, SLOT (31), 20, 18,
     "", "!Locate Object!", ""},

    {
     "magic missile", {1, 53, 2, 2}, {1, 1, 2, 2},
     spell_magic_missile, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT (32), 15, 12,
     "magic missile", "!Magic Missile!", ""},

    {
     "mass healing", {53, 38, 53, 46}, {2, 2, 4, 4},
     spell_mass_healing, TAR_IGNORE, POS_STANDING,
     NULL, SLOT (508), 100, 36,
     "", "!Mass Healing!", ""},

    {
     "mass invis", {22, 25, 31, 53}, {1, 1, 2, 2},
     spell_mass_invis, TAR_IGNORE, POS_STANDING,
     &gsn_mass_invis, SLOT (69), 20, 24,
     "", "You are no longer invisible.", ""},

    {
     "nexus", {40, 35, 50, 45}, {2, 2, 4, 4},
     spell_nexus, TAR_IGNORE, POS_STANDING,
     NULL, SLOT (520), 150, 36,
     "", "!Nexus!", ""},

    {
     "pass door", {24, 32, 25, 37}, {1, 1, 2, 2},
     spell_pass_door, TAR_CHAR_SELF, POS_STANDING,
     NULL, SLOT (74), 20, 12,
     "", "You feel solid again.", ""},

    {
     "plague", {23, 17, 36, 26}, {1, 1, 2, 2},
     spell_plague, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     &gsn_plague, SLOT (503), 20, 12,
     "sickness", "Your sores vanish.", ""},

    {
     "poison", {17, 12, 15, 21}, {1, 1, 2, 2},
     spell_poison, TAR_OBJ_CHAR_OFF, POS_FIGHTING,
     &gsn_poison, SLOT (33), 10, 12,
     "poison", "You feel less sick.",
     "The poison on $p dries up."},

    {
     "portal", {35, 30, 45, 40}, {2, 2, 4, 4},
     spell_portal, TAR_IGNORE, POS_STANDING,
     NULL, SLOT (519), 100, 24,
     "", "!Portal!", ""},

    {
     "protection evil", {12, 9, 17, 11}, {1, 1, 2, 2},
     spell_protection_evil, TAR_CHAR_SELF, POS_STANDING,
     NULL, SLOT (34), 5, 12,
     "", "You feel less protected.", ""},

    {
     "protection good", {12, 9, 17, 11}, {1, 1, 2, 2},
     spell_protection_good, TAR_CHAR_SELF, POS_STANDING,
     NULL, SLOT (514), 5, 12,
     "", "You feel less protected.", ""},

    {
     "ray of truth", {53, 35, 53, 47}, {1, 1, 2, 2},
     spell_ray_of_truth, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT (518), 20, 12,
     "ray of truth", "!Ray of Truth!", ""},

    {
     "recharge", {9, 53, 53, 53}, {1, 1, 2, 2},
     spell_recharge, TAR_OBJ_INV, POS_STANDING,
     NULL, SLOT (517), 60, 24,
     "", "!Recharge!", ""},

    {
     "refresh", {8, 5, 12, 9}, {1, 1, 2, 2},
     spell_refresh, TAR_CHAR_DEFENSIVE, POS_STANDING,
     NULL, SLOT (81), 12, 18,
     "refresh", "!Refresh!", ""},

    {
     "remove curse", {53, 18, 53, 22}, {1, 1, 2, 2},
     spell_remove_curse, TAR_OBJ_CHAR_DEF, POS_STANDING,
     NULL, SLOT (35), 5, 12,
     "", "!Remove Curse!", ""},

    {
     "sanctuary", {36, 20, 42, 30}, {1, 1, 2, 2},
     spell_sanctuary, TAR_CHAR_DEFENSIVE, POS_STANDING,
     &gsn_sanctuary, SLOT (36), 75, 12,
     "", "The white aura around your body fades.",
     ""},

    {
     "shield", {20, 35, 35, 40}, {1, 1, 2, 2},
     spell_shield, TAR_CHAR_DEFENSIVE, POS_STANDING,
     NULL, SLOT (67), 12, 18,
     "", "Your force shield shimmers then fades away.",
     ""},

    {
     "shocking grasp", {10, 53, 14, 13}, {1, 1, 2, 2},
     spell_shocking_grasp, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT (53), 15, 12,
     "shocking grasp", "!Shocking Grasp!", ""},

    {
     "sleep", {10, 53, 11, 53}, {1, 1, 2, 2},
     spell_sleep, TAR_CHAR_OFFENSIVE, POS_STANDING,
     &gsn_sleep, SLOT (38), 15, 12,
     "", "You feel less tired.", ""},

    {
     "slow", {23, 30, 29, 32}, {1, 1, 2, 2},
     spell_slow, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT (515), 30, 12,
     "", "You feel yourself speed up.", ""},

    {
     "stone skin", {25, 40, 40, 45}, {1, 1, 2, 2},
     spell_stone_skin, TAR_CHAR_SELF, POS_STANDING,
     NULL, SLOT (66), 12, 18,
     "", "Your skin feels soft again.", ""},

    {
     "summon", {24, 12, 29, 22}, {1, 1, 2, 2},
     spell_summon, TAR_IGNORE, POS_STANDING,
     NULL, SLOT (40), 50, 12,
     "", "!Summon!", ""},

    {
     "teleport", {13, 22, 25, 36}, {1, 1, 2, 2},
     spell_teleport, TAR_CHAR_SELF, POS_FIGHTING,
     NULL, SLOT (2), 35, 12,
     "", "!Teleport!", ""},

    {
     "ventriloquate", {1, 53, 2, 53}, {1, 1, 2, 2},
     spell_ventriloquate, TAR_IGNORE, POS_STANDING,
     NULL, SLOT (41), 5, 12,
     "", "!Ventriloquate!", ""},

    {
     "weaken", {11, 14, 16, 17}, {1, 1, 2, 2},
     spell_weaken, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT (68), 20, 12,
     "spell", "You feel stronger.", ""},

    {
     "word of recall", {32, 28, 40, 30}, {1, 1, 2, 2},
     spell_word_of_recall, TAR_CHAR_SELF, POS_RESTING,
     NULL, SLOT (42), 5, 12,
     "", "!Word of Recall!", ""},

/*
 * Dragon breath
 */
    {
     "acid breath", {31, 32, 33, 34}, {1, 1, 2, 2},
     spell_acid_breath, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT (200), 100, 24,
     "blast of acid", "!Acid Breath!", ""},

    {
     "fire breath", {40, 45, 50, 51}, {1, 1, 2, 2},
     spell_fire_breath, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT (201), 200, 24,
     "blast of flame", "The smoke leaves your eyes.", ""},

    {
     "frost breath", {34, 36, 38, 40}, {1, 1, 2, 2},
     spell_frost_breath, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT (202), 125, 24,
     "blast of frost", "!Frost Breath!", ""},

    {
     "gas breath", {39, 43, 47, 50}, {1, 1, 2, 2},
     spell_gas_breath, TAR_IGNORE, POS_FIGHTING,
     NULL, SLOT (203), 175, 24,
     "blast of gas", "!Gas Breath!", ""},

    {
     "lightning breath", {37, 40, 43, 46}, {1, 1, 2, 2},
     spell_lightning_breath, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT (204), 150, 24,
     "blast of lightning", "!Lightning Breath!", ""},

/*
 * Spells for mega1.are from Glop/Erkenbrand.
 */
    {
     "general purpose", {53, 53, 53, 53}, {0, 0, 0, 0},
     spell_general_purpose, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT (401), 0, 12,
     "general purpose ammo", "!General Purpose Ammo!", ""},

    {
     "high explosive", {53, 53, 53, 53}, {0, 0, 0, 0},
     spell_high_explosive, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     NULL, SLOT (402), 0, 12,
     "high explosive ammo", "!High Explosive Ammo!", ""},


/* combat and weapons skills */


    {
     "axe", {1, 1, 1, 1}, {6, 6, 5, 4},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     &gsn_axe, SLOT (0), 0, 0,
     "", "!Axe!", ""},

    {
     "dagger", {1, 1, 1, 1}, {2, 3, 2, 2},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     &gsn_dagger, SLOT (0), 0, 0,
     "", "!Dagger!", ""},

    {
     "flail", {1, 1, 1, 1}, {6, 3, 6, 4},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     &gsn_flail, SLOT (0), 0, 0,
     "", "!Flail!", ""},

    {
     "mace", {1, 1, 1, 1}, {5, 2, 3, 3},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     &gsn_mace, SLOT (0), 0, 0,
     "", "!Mace!", ""},

    {
     "polearm", {1, 1, 1, 1}, {6, 6, 6, 4},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     &gsn_polearm, SLOT (0), 0, 0,
     "", "!Polearm!", ""},

    {
     "shield block", {1, 1, 1, 1}, {6, 4, 6, 2},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     &gsn_shield_block, SLOT (0), 0, 0,
     "", "!Shield!", ""},

    {
     "spear", {1, 1, 1, 1}, {4, 4, 4, 3},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     &gsn_spear, SLOT (0), 0, 0,
     "", "!Spear!", ""},

    {
     "sword", {1, 1, 1, 1}, {5, 6, 3, 2},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     &gsn_sword, SLOT (0), 0, 0,
     "", "!sword!", ""},

    {
     "whip", {1, 1, 1, 1}, {6, 5, 5, 4},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     &gsn_whip, SLOT (0), 0, 0,
     "", "!Whip!", ""},

    {
     "backstab", {53, 53, 1, 53}, {0, 0, 5, 0},
     spell_null, TAR_IGNORE, POS_STANDING,
     &gsn_backstab, SLOT (0), 0, 24,
     "backstab", "!Backstab!", ""},

    {
     "bash", {53, 53, 53, 1}, {0, 0, 0, 4},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     &gsn_bash, SLOT (0), 0, 24,
     "bash", "!Bash!", ""},

    {
     "berserk", {53, 53, 53, 18}, {0, 0, 0, 5},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     &gsn_berserk, SLOT (0), 0, 24,
     "", "You feel your pulse slow down.", ""},

    {
     "dirt kicking", {53, 53, 3, 3}, {0, 0, 4, 4},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     &gsn_dirt, SLOT (0), 0, 24,
     "kicked dirt", "You rub the dirt out of your eyes.", ""},

    {
     "disarm", {53, 53, 12, 11}, {0, 0, 6, 4},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     &gsn_disarm, SLOT (0), 0, 24,
     "", "!Disarm!", ""},

    {
     "dodge", {20, 22, 1, 13}, {8, 8, 4, 6},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     &gsn_dodge, SLOT (0), 0, 0,
     "", "!Dodge!", ""},

    {
     "enhanced damage", {45, 30, 25, 1}, {10, 9, 5, 3},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     &gsn_enhanced_damage, SLOT (0), 0, 0,
     "", "!Enhanced Damage!", ""},

    {
     "envenom", {53, 53, 10, 53}, {0, 0, 4, 0},
     spell_null, TAR_IGNORE, POS_RESTING,
     &gsn_envenom, SLOT (0), 0, 36,
     "", "!Envenom!", ""},

    {
     "hand to hand", {25, 10, 15, 6}, {8, 5, 6, 4},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     &gsn_hand_to_hand, SLOT (0), 0, 0,
     "", "!Hand to Hand!", ""},

    {
     "kick", {53, 12, 14, 8}, {0, 4, 6, 3},
     spell_null, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
     &gsn_kick, SLOT (0), 0, 12,
     "kick", "!Kick!", ""},

    {
     "parry", {22, 20, 13, 1}, {8, 8, 6, 4},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     &gsn_parry, SLOT (0), 0, 0,
     "", "!Parry!", ""},

    {
     "rescue", {53, 53, 53, 1}, {0, 0, 0, 4},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     &gsn_rescue, SLOT (0), 0, 12,
     "", "!Rescue!", ""},

    {
     "trip", {53, 53, 1, 15}, {0, 0, 4, 8},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     &gsn_trip, SLOT (0), 0, 24,
     "trip", "!Trip!", ""},

    {
     "second attack", {30, 24, 12, 5}, {10, 8, 5, 3},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     &gsn_second_attack, SLOT (0), 0, 0,
     "", "!Second Attack!", ""},

    {
     "third attack", {53, 53, 24, 12}, {0, 0, 10, 4},
     spell_null, TAR_IGNORE, POS_FIGHTING,
     &gsn_third_attack, SLOT (0), 0, 0,
     "", "!Third Attack!", ""},

/* non-combat skills */

    {
     "fast healing", {15, 9, 16, 6}, {8, 5, 6, 4},
     spell_null, TAR_IGNORE, POS_SLEEPING,
     &gsn_fast_healing, SLOT (0), 0, 0,
     "", "!Fast Healing!", ""},

    {
     "haggle", {7, 18, 1, 14}, {5, 8, 3, 6},
     spell_null, TAR_IGNORE, POS_RESTING,
     &gsn_haggle, SLOT (0), 0, 0,
     "", "!Haggle!", ""},

    {
     "hide", {53, 53, 1, 12}, {0, 0, 4, 6},
     spell_null, TAR_IGNORE, POS_RESTING,
     &gsn_hide, SLOT (0), 0, 12,
     "", "!Hide!", ""},

    {
     "lore", {10, 10, 6, 20}, {6, 6, 4, 8},
     spell_null, TAR_IGNORE, POS_RESTING,
     &gsn_lore, SLOT (0), 0, 36,
     "", "!Lore!", ""},

    {
     "meditation", {6, 6, 15, 15}, {5, 5, 8, 8},
     spell_null, TAR_IGNORE, POS_SLEEPING,
     &gsn_meditation, SLOT (0), 0, 0,
     "", "Meditation", ""},

    {
     "peek", {8, 21, 1, 14}, {5, 7, 3, 6},
     spell_null, TAR_IGNORE, POS_STANDING,
     &gsn_peek, SLOT (0), 0, 0,
     "", "!Peek!", ""},

    {
     "pick lock", {25, 25, 7, 25}, {8, 8, 4, 8},
     spell_null, TAR_IGNORE, POS_STANDING,
     &gsn_pick_lock, SLOT (0), 0, 12,
     "", "!Pick!", ""},

    {
     "sneak", {53, 53, 4, 10}, {0, 0, 4, 6},
     spell_null, TAR_IGNORE, POS_STANDING,
     &gsn_sneak, SLOT (0), 0, 12,
     "", "You no longer feel stealthy.", ""},

    {
     "steal", {53, 53, 5, 53}, {0, 0, 4, 0},
     spell_null, TAR_IGNORE, POS_STANDING,
     &gsn_steal, SLOT (0), 0, 24,
     "", "!Steal!", ""},

    {
     "scrolls", {1, 1, 1, 1}, {2, 3, 5, 8},
     spell_null, TAR_IGNORE, POS_STANDING,
     &gsn_scrolls, SLOT (0), 0, 24,
     "", "!Scrolls!", ""},

    {
     "staves", {1, 1, 1, 1}, {2, 3, 5, 8},
     spell_null, TAR_IGNORE, POS_STANDING,
     &gsn_staves, SLOT (0), 0, 12,
     "", "!Staves!", ""},

    {
     "wands", {1, 1, 1, 1}, {2, 3, 5, 8},
     spell_null, TAR_IGNORE, POS_STANDING,
     &gsn_wands, SLOT (0), 0, 12,
     "", "!Wands!", ""},

    {
     "recall", {1, 1, 1, 1}, {2, 2, 2, 2},
     spell_null, TAR_IGNORE, POS_STANDING,
     &gsn_recall, SLOT (0), 0, 12,
     "", "!Recall!", ""}
};

const struct group_type group_table[MAX_GROUP] = {

    {
     "rom basics", {0, 0, 0, 0},
     {"scrolls", "staves", "wands", "recall"}
     },

    {
     "mage basics", {0, -1, -1, -1},
     {"dagger"}
     },

    {
     "cleric basics", {-1, 0, -1, -1},
     {"mace"}
     },

    {
     "thief basics", {-1, -1, 0, -1},
     {"dagger", "steal"}
     },

    {
     "warrior basics", {-1, -1, -1, 0},
     {"sword", "second attack"}
     },

    {
     "mage default", {40, -1, -1, -1},
     {"lore", "beguiling", "combat", "detection", "enhancement", "illusion",
      "maladictions", "protective", "transportation", "weather"}
     },

    {
     "cleric default", {-1, 40, -1, -1},
     {"flail", "attack", "creation", "curative", "benedictions",
      "detection", "healing", "maladictions", "protective", "shield block",
      "transportation", "weather"}
     },

    {
     "thief default", {-1, -1, 40, -1},
     {"mace", "sword", "backstab", "disarm", "dodge", "second attack",
      "trip", "hide", "peek", "pick lock", "sneak"}
     },

    {
     "warrior default", {-1, -1, -1, 40},
     {"weaponsmaster", "shield block", "bash", "disarm", "enhanced damage",
      "parry", "rescue", "third attack"}
     },

    {
     "weaponsmaster", {40, 40, 40, 20},
     {"axe", "dagger", "flail", "mace", "polearm", "spear", "sword", "whip"}
     },

    {
     "attack", {-1, 5, -1, 8},
     {"demonfire", "dispel evil", "dispel good", "earthquake",
      "flamestrike", "heat metal", "ray of truth"}
     },

    {
     "beguiling", {4, -1, 6, -1},
     {"calm", "charm person", "sleep"}
     },

    {
     "benedictions", {-1, 4, -1, 8},
     {"bless", "calm", "frenzy", "holy word", "remove curse"}
     },

    {
     "combat", {6, -1, 10, 9},
     {"acid blast", "burning hands", "chain lightning", "chill touch",
      "colour spray", "fireball", "lightning bolt", "magic missile",
      "shocking grasp"}
     },

    {
     "creation", {4, 4, 8, 8},
     {"continual light", "create food", "create spring", "create water",
      "create rose", "floating disc"}
     },

    {
     "curative", {-1, 4, -1, 8},
     {"cure blindness", "cure disease", "cure poison"}
     },

    {
     "detection", {4, 3, 6, -1},
     {"detect evil", "detect good", "detect hidden", "detect invis",
      "detect magic", "detect poison", "farsight", "identify",
      "know alignment", "locate object"}
     },

    {
     "draconian", {8, -1, -1, -1},
     {"acid breath", "fire breath", "frost breath", "gas breath",
      "lightning breath"}
     },

    {
     "enchantment", {6, -1, -1, -1},
     {"enchant armor", "enchant weapon", "fireproof", "recharge"}
     },

    {
     "enhancement", {5, -1, 9, 9},
     {"giant strength", "haste", "infravision", "refresh"}
     },

    {
     "harmful", {-1, 3, -1, 6},
     {"cause critical", "cause light", "cause serious", "harm"}
     },

    {
     "healing", {-1, 3, -1, 6},
     {"cure critical", "cure light", "cure serious", "heal",
      "mass healing", "refresh"}
     },

    {
     "illusion", {4, -1, 7, -1},
     {"invis", "mass invis", "ventriloquate"}
     },

    {
     "maladictions", {5, 5, 9, 9},
     {"blindness", "change sex", "curse", "energy drain", "plague",
      "poison", "slow", "weaken"}
     },

    {
     "protective", {4, 4, 7, 8},
     {"armor", "cancellation", "dispel magic", "fireproof",
      "protection evil", "protection good", "sanctuary", "shield",
      "stone skin"}
     },

    {
     "transportation", {4, 4, 8, 9},
     {"fly", "gate", "nexus", "pass door", "portal", "summon", "teleport",
      "word of recall"}
     },

    {
     "weather", {4, 4, 8, 8},
     {"call lightning", "control weather", "faerie fire", "faerie fog",
      "lightning bolt"}
     }



};
