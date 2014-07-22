/***************************************************************************
 *  File: olc.c                                                            *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 *                                                                         *
 *  This code was freely distributed with the The Isles 1.1 source code,   *
 *  and has been used here for OLC - OLC would not be what it is without   *
 *  all the previous coders who released their source code.                *
 *                                                                         *
 ***************************************************************************/



#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "tables.h"
#include "olc.h"

/*
 * Local functions.
 */
AREA_DATA *get_area_data args ((int vnum));


/* Executed from comm.c.  Minimizes compiling when changes are made. */
bool run_olc_editor (DESCRIPTOR_DATA * d)
{
    switch (d->editor)
    {
        case ED_AREA:
            aedit (d->character, d->incomm);
            break;
        case ED_ROOM:
            redit (d->character, d->incomm);
            break;
        case ED_OBJECT:
            oedit (d->character, d->incomm);
            break;
        case ED_MOBILE:
            medit (d->character, d->incomm);
            break;
        case ED_MPCODE:
            mpedit (d->character, d->incomm);
            break;
        case ED_HELP:
            hedit (d->character, d->incomm);
            break;
        default:
            return FALSE;
    }
    return TRUE;
}



char *olc_ed_name (CHAR_DATA * ch)
{
    static char buf[10];

    buf[0] = '\0';
    switch (ch->desc->editor)
    {
        case ED_AREA:
            sprintf (buf, "AEdit");
            break;
        case ED_ROOM:
            sprintf (buf, "REdit");
            break;
        case ED_OBJECT:
            sprintf (buf, "OEdit");
            break;
        case ED_MOBILE:
            sprintf (buf, "MEdit");
            break;
        case ED_MPCODE:
            sprintf (buf, "MPEdit");
            break;
        case ED_HELP:
            sprintf (buf, "HEdit");
            break;
        default:
            sprintf (buf, " ");
            break;
    }
    return buf;
}



char *olc_ed_vnum (CHAR_DATA * ch)
{
    AREA_DATA *pArea;
    ROOM_INDEX_DATA *pRoom;
    OBJ_INDEX_DATA *pObj;
    MOB_INDEX_DATA *pMob;
    MPROG_CODE *pMprog;
    HELP_DATA *pHelp;
    static char buf[MIL];

    buf[0] = '\0';
    switch (ch->desc->editor)
    {
        case ED_AREA:
            pArea = (AREA_DATA *) ch->desc->pEdit;
            sprintf (buf, "%d", pArea ? pArea->vnum : 0);
            break;
        case ED_ROOM:
            pRoom = ch->in_room;
            sprintf (buf, "%d", pRoom ? pRoom->vnum : 0);
            break;
        case ED_OBJECT:
            pObj = (OBJ_INDEX_DATA *) ch->desc->pEdit;
            sprintf (buf, "%d", pObj ? pObj->vnum : 0);
            break;
        case ED_MOBILE:
            pMob = (MOB_INDEX_DATA *) ch->desc->pEdit;
            sprintf (buf, "%d", pMob ? pMob->vnum : 0);
            break;
        case ED_MPCODE:
            pMprog = (MPROG_CODE *) ch->desc->pEdit;
            sprintf (buf, "%d", pMprog ? pMprog->vnum : 0);
            break;
        case ED_HELP:
            pHelp = (HELP_DATA *) ch->desc->pEdit;
            sprintf (buf, "%s", pHelp ? pHelp->keyword : "");
            break;
        default:
            sprintf (buf, " ");
            break;
    }

    return buf;
}



/*****************************************************************************
 Name:        show_olc_cmds
 Purpose:    Format up the commands from given table.
 Called by:    show_commands(olc_act.c).
 ****************************************************************************/
void show_olc_cmds (CHAR_DATA * ch, const struct olc_cmd_type *olc_table)
{
    char buf[MAX_STRING_LENGTH];
    char buf1[MAX_STRING_LENGTH];
    int cmd;
    int col;

    buf1[0] = '\0';
    col = 0;
    for (cmd = 0; olc_table[cmd].name != NULL; cmd++)
    {
        sprintf (buf, "%-15.15s", olc_table[cmd].name);
        strcat (buf1, buf);
        if (++col % 5 == 0)
            strcat (buf1, "\n\r");
    }

    if (col % 5 != 0)
        strcat (buf1, "\n\r");

    send_to_char (buf1, ch);
    return;
}



/*****************************************************************************
 Name:        show_commands
 Purpose:    Display all olc commands.
 Called by:    olc interpreters.
 ****************************************************************************/
bool show_commands (CHAR_DATA * ch, char *argument)
{
    switch (ch->desc->editor)
    {
        case ED_AREA:
            show_olc_cmds (ch, aedit_table);
            break;
        case ED_ROOM:
            show_olc_cmds (ch, redit_table);
            break;
        case ED_OBJECT:
            show_olc_cmds (ch, oedit_table);
            break;
        case ED_MOBILE:
            show_olc_cmds (ch, medit_table);
            break;
        case ED_MPCODE:
            show_olc_cmds (ch, mpedit_table);
            break;
        case ED_HELP:
            show_olc_cmds (ch, hedit_table);
            break;
    }

    return FALSE;
}



/*****************************************************************************
 *                           Interpreter Tables.                             *
 *****************************************************************************/
const struct olc_cmd_type aedit_table[] = {
/*  {   command        function    }, */

    {"age", aedit_age},
    {"builder", aedit_builder},    /* s removed -- Hugin */
    {"commands", show_commands},
    {"create", aedit_create},
    {"filename", aedit_file},
    {"name", aedit_name},
/*  {   "recall",    aedit_recall    },   ROM OLC */
    {"reset", aedit_reset},
    {"security", aedit_security},
    {"show", aedit_show},
    {"vnum", aedit_vnum},
    {"lvnum", aedit_lvnum},
    {"uvnum", aedit_uvnum},
    {"credits", aedit_credits},

    {"?", show_help},
    {"version", show_version},

    {NULL, 0,}
};



const struct olc_cmd_type redit_table[] = {
/*  {   command        function    }, */

    {"commands", show_commands},
    {"create", redit_create},
    {"desc", redit_desc},
    {"ed", redit_ed},
    {"format", redit_format},
    {"name", redit_name},
    {"show", redit_show},
    {"heal", redit_heal},
    {"mana", redit_mana},
    {"clan", redit_clan},

    {"north", redit_north},
    {"south", redit_south},
    {"east", redit_east},
    {"west", redit_west},
    {"up", redit_up},
    {"down", redit_down},

    /* New reset commands. */
    {"mreset", redit_mreset},
    {"oreset", redit_oreset},
    {"mlist", redit_mlist},
    {"rlist", redit_rlist},
    {"olist", redit_olist},
    {"mshow", redit_mshow},
    {"oshow", redit_oshow},
    {"owner", redit_owner},
    {"room", redit_room},
    {"sector", redit_sector},

    {"?", show_help},
    {"version", show_version},

    {NULL, 0,}
};



const struct olc_cmd_type oedit_table[] = {
/*  {   command        function    }, */

    {"addaffect", oedit_addaffect},
    {"addapply", oedit_addapply},
    {"commands", show_commands},
    {"cost", oedit_cost},
    {"create", oedit_create},
    {"delaffect", oedit_delaffect},
    {"ed", oedit_ed},
    {"long", oedit_long},
    {"name", oedit_name},
    {"short", oedit_short},
    {"show", oedit_show},
    {"v0", oedit_value0},
    {"v1", oedit_value1},
    {"v2", oedit_value2},
    {"v3", oedit_value3},
    {"v4", oedit_value4},        /* ROM */
    {"weight", oedit_weight},

    {"extra", oedit_extra},        /* ROM */
    {"wear", oedit_wear},        /* ROM */
    {"type", oedit_type},        /* ROM */
    {"material", oedit_material},    /* ROM */
    {"level", oedit_level},        /* ROM */
    {"condition", oedit_condition},    /* ROM */

    {"?", show_help},
    {"version", show_version},

    {NULL, 0,}
};



const struct olc_cmd_type medit_table[] = {
/*  {   command        function    }, */

    {"alignment", medit_align},
    {"commands", show_commands},
    {"create", medit_create},
    {"desc", medit_desc},
    {"level", medit_level},
    {"long", medit_long},
    {"name", medit_name},
    {"shop", medit_shop},
    {"short", medit_short},
    {"show", medit_show},
    {"spec", medit_spec},

    {"sex", medit_sex},            /* ROM */
    {"act", medit_act},            /* ROM */
    {"affect", medit_affect},    /* ROM */
    {"armor", medit_ac},        /* ROM */
    {"form", medit_form},        /* ROM */
    {"part", medit_part},        /* ROM */
    {"imm", medit_imm},            /* ROM */
    {"res", medit_res},            /* ROM */
    {"vuln", medit_vuln},        /* ROM */
    {"material", medit_material},    /* ROM */
    {"off", medit_off},            /* ROM */
    {"size", medit_size},        /* ROM */
    {"hitdice", medit_hitdice},    /* ROM */
    {"manadice", medit_manadice},    /* ROM */
    {"damdice", medit_damdice},    /* ROM */
    {"race", medit_race},        /* ROM */
    {"position", medit_position},    /* ROM */
    {"wealth", medit_gold},        /* ROM */
    {"hitroll", medit_hitroll},    /* ROM */
    {"damtype", medit_damtype},    /* ROM */
    {"group", medit_group},        /* ROM */
    {"addmprog", medit_addmprog},    /* ROM */
    {"delmprog", medit_delmprog},    /* ROM */

    {"?", show_help},
    {"version", show_version},

    {NULL, 0,}
};

/*****************************************************************************
 *                          End Interpreter Tables.                          *
 *****************************************************************************/



/*****************************************************************************
 Name:        get_area_data
 Purpose:    Returns pointer to area with given vnum.
 Called by:    do_aedit(olc.c).
 ****************************************************************************/
AREA_DATA *get_area_data (int vnum)
{
    AREA_DATA *pArea;

    for (pArea = area_first; pArea; pArea = pArea->next)
    {
        if (pArea->vnum == vnum)
            return pArea;
    }

    return 0;
}



/*****************************************************************************
 Name:        edit_done
 Purpose:    Resets builder information on completion.
 Called by:    aedit, redit, oedit, medit(olc.c)
 ****************************************************************************/
bool edit_done (CHAR_DATA * ch)
{
    ch->desc->pEdit = NULL;
    ch->desc->editor = 0;
    return FALSE;
}



/*****************************************************************************
 *                              Interpreters.                                *
 *****************************************************************************/


/* Area Interpreter, called by do_aedit. */
void aedit (CHAR_DATA * ch, char *argument)
{
    AREA_DATA *pArea;
    char command[MAX_INPUT_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    int cmd;
    int value;

    EDIT_AREA (ch, pArea);
    smash_tilde (argument);
    strcpy (arg, argument);
    argument = one_argument (argument, command);

    if (!IS_BUILDER (ch, pArea))
    {
        send_to_char ("AEdit:  Insufficient security to modify area.\n\r",
                      ch);
        edit_done (ch);
        return;
    }

    if (!str_cmp (command, "done"))
    {
        edit_done (ch);
        return;
    }

    if (command[0] == '\0')
    {
        aedit_show (ch, argument);
        return;
    }

    if ((value = flag_value (area_flags, command)) != NO_FLAG)
    {
        TOGGLE_BIT (pArea->area_flags, value);

        send_to_char ("Flag toggled.\n\r", ch);
        return;
    }

    /* Search Table and Dispatch Command. */
    for (cmd = 0; aedit_table[cmd].name != NULL; cmd++)
    {
        if (!str_prefix (command, aedit_table[cmd].name))
        {
            if ((*aedit_table[cmd].olc_fun) (ch, argument))
            {
                SET_BIT (pArea->area_flags, AREA_CHANGED);
                return;
            }
            else
                return;
        }
    }

    /* Default to Standard Interpreter. */
    interpret (ch, arg);
    return;
}



/* Room Interpreter, called by do_redit. */
void redit (CHAR_DATA * ch, char *argument)
{
    AREA_DATA *pArea;
    ROOM_INDEX_DATA *pRoom;
    char arg[MAX_STRING_LENGTH];
    char command[MAX_INPUT_LENGTH];
    int cmd;

    EDIT_ROOM (ch, pRoom);
    pArea = pRoom->area;

    smash_tilde (argument);
    strcpy (arg, argument);
    argument = one_argument (argument, command);

    if (!IS_BUILDER (ch, pArea))
    {
        send_to_char ("REdit:  Insufficient security to modify room.\n\r",
                      ch);
        edit_done (ch);
        return;
    }

    if (!str_cmp (command, "done"))
    {
        edit_done (ch);
        return;
    }

    if (command[0] == '\0')
    {
        redit_show (ch, argument);
        return;
    }

    /* Search Table and Dispatch Command. */
    for (cmd = 0; redit_table[cmd].name != NULL; cmd++)
    {
        if (!str_prefix (command, redit_table[cmd].name))
        {
            if ((*redit_table[cmd].olc_fun) (ch, argument))
            {
                SET_BIT (pArea->area_flags, AREA_CHANGED);
                return;
            }
            else
                return;
        }
    }

    /* Default to Standard Interpreter. */
    interpret (ch, arg);
    return;
}



/* Object Interpreter, called by do_oedit. */
void oedit (CHAR_DATA * ch, char *argument)
{
    AREA_DATA *pArea;
    OBJ_INDEX_DATA *pObj;
    char arg[MAX_STRING_LENGTH];
    char command[MAX_INPUT_LENGTH];
    int cmd;

    smash_tilde (argument);
    strcpy (arg, argument);
    argument = one_argument (argument, command);

    EDIT_OBJ (ch, pObj);
    pArea = pObj->area;

    if (!IS_BUILDER (ch, pArea))
    {
        send_to_char ("OEdit: Insufficient security to modify area.\n\r", ch);
        edit_done (ch);
        return;
    }

    if (!str_cmp (command, "done"))
    {
        edit_done (ch);
        return;
    }

    if (command[0] == '\0')
    {
        oedit_show (ch, argument);
        return;
    }

    /* Search Table and Dispatch Command. */
    for (cmd = 0; oedit_table[cmd].name != NULL; cmd++)
    {
        if (!str_prefix (command, oedit_table[cmd].name))
        {
            if ((*oedit_table[cmd].olc_fun) (ch, argument))
            {
                SET_BIT (pArea->area_flags, AREA_CHANGED);
                return;
            }
            else
                return;
        }
    }

    /* Default to Standard Interpreter. */
    interpret (ch, arg);
    return;
}



/* Mobile Interpreter, called by do_medit. */
void medit (CHAR_DATA * ch, char *argument)
{
    AREA_DATA *pArea;
    MOB_INDEX_DATA *pMob;
    char command[MAX_INPUT_LENGTH];
    char arg[MAX_STRING_LENGTH];
    int cmd;

    smash_tilde (argument);
    strcpy (arg, argument);
    argument = one_argument (argument, command);

    EDIT_MOB (ch, pMob);
    pArea = pMob->area;

    if (!IS_BUILDER (ch, pArea))
    {
        send_to_char ("MEdit: Insufficient security to modify area.\n\r", ch);
        edit_done (ch);
        return;
    }

    if (!str_cmp (command, "done"))
    {
        edit_done (ch);
        return;
    }

    if (command[0] == '\0')
    {
        medit_show (ch, argument);
        return;
    }

    /* Search Table and Dispatch Command. */
    for (cmd = 0; medit_table[cmd].name != NULL; cmd++)
    {
        if (!str_prefix (command, medit_table[cmd].name))
        {
            if ((*medit_table[cmd].olc_fun) (ch, argument))
            {
                SET_BIT (pArea->area_flags, AREA_CHANGED);
                return;
            }
            else
                return;
        }
    }

    /* Default to Standard Interpreter. */
    interpret (ch, arg);
    return;
}




const struct editor_cmd_type editor_table[] = {
/*  {   command        function    }, */

    {"area", do_aedit},
    {"room", do_redit},
    {"object", do_oedit},
    {"mobile", do_medit},
    {"mpcode", do_mpedit},
    {"hedit", do_hedit},

    {NULL, 0,}
};


/* Entry point for all editors. */
void do_olc (CHAR_DATA * ch, char *argument)
{
    char command[MAX_INPUT_LENGTH];
    int cmd;

    if (IS_NPC (ch))
        return;

    argument = one_argument (argument, command);

    if (command[0] == '\0')
    {
        do_help (ch, "olc");
        return;
    }

    /* Search Table and Dispatch Command. */
    for (cmd = 0; editor_table[cmd].name != NULL; cmd++)
    {
        if (!str_prefix (command, editor_table[cmd].name))
        {
            (*editor_table[cmd].do_fun) (ch, argument);
            return;
        }
    }

    /* Invalid command, send help. */
    do_help (ch, "olc");
    return;
}



/* Entry point for editing area_data. */
void do_aedit (CHAR_DATA * ch, char *argument)
{
    AREA_DATA *pArea;
    int value;
    char arg[MAX_STRING_LENGTH];

    if (IS_NPC (ch))
        return;

    pArea = ch->in_room->area;

    argument = one_argument (argument, arg);

    if (is_number (arg))
    {
        value = atoi (arg);
        if (!(pArea = get_area_data (value)))
        {
            send_to_char ("That area vnum does not exist.\n\r", ch);
            return;
        }
    }
    else if (!str_cmp (arg, "create"))
    {
        if (ch->pcdata->security < 9)
        {
            send_to_char ("AEdit : Insufficient security to create area.\n\r",
                          ch);
            return;
        }

        aedit_create (ch, "");
        ch->desc->editor = ED_AREA;
        return;
    }

    if (!IS_BUILDER (ch, pArea))
    {
        send_to_char ("Insufficient security to edit areas.\n\r", ch);
        return;
    }

    ch->desc->pEdit = (void *) pArea;
    ch->desc->editor = ED_AREA;
    return;
}



/* Entry point for editing room_index_data. */
void do_redit (CHAR_DATA * ch, char *argument)
{
    ROOM_INDEX_DATA *pRoom;
    char arg1[MAX_STRING_LENGTH];

    if (IS_NPC (ch))
        return;

    argument = one_argument (argument, arg1);

    pRoom = ch->in_room;

    if (!str_cmp (arg1, "reset"))
    {                            /* redit reset */
        if (!IS_BUILDER (ch, pRoom->area))
        {
            send_to_char ("Insufficient security to modify room.\n\r", ch);
            return;
        }

        reset_room (pRoom);
        send_to_char ("Room reset.\n\r", ch);

        return;
    }
    else if (!str_cmp (arg1, "create"))
    {                            /* redit create <vnum> */
        if (argument[0] == '\0' || atoi (argument) == 0)
        {
            send_to_char ("Syntax:  edit room create [vnum]\n\r", ch);
            return;
        }

        if (redit_create (ch, argument))
        {                        /* pEdit == nuevo cuarto */
            ch->desc->editor = ED_ROOM;
            char_from_room (ch);
            char_to_room (ch, ch->desc->pEdit);
            SET_BIT (((ROOM_INDEX_DATA *) ch->desc->pEdit)->area->area_flags,
                     AREA_CHANGED);
        }

        return;
    }
    else if (!IS_NULLSTR (arg1))
    {                            /* redit <vnum> */
        pRoom = get_room_index (atoi (arg1));

        if (!pRoom)
        {
            send_to_char ("REdit : Nonexistant room.\n\r", ch);
            return;
        }

        if (!IS_BUILDER (ch, pRoom->area))
        {
            send_to_char ("REdit : Insufficient security to modify room.\n\r",
                          ch);
            return;
        }

        char_from_room (ch);
        char_to_room (ch, pRoom);
    }

    if (!IS_BUILDER (ch, pRoom->area))
    {
        send_to_char ("REdit : Insufficient security to modify room.\n\r",
                      ch);
        return;
    }

    ch->desc->pEdit = (void *) pRoom;
    ch->desc->editor = ED_ROOM;

    return;
}



/* Entry point for editing obj_index_data. */
void do_oedit (CHAR_DATA * ch, char *argument)
{
    OBJ_INDEX_DATA *pObj;
    AREA_DATA *pArea;
    char arg1[MAX_STRING_LENGTH];
    int value;

    if (IS_NPC (ch))
        return;

    argument = one_argument (argument, arg1);

    if (is_number (arg1))
    {
        value = atoi (arg1);
        if (!(pObj = get_obj_index (value)))
        {
            send_to_char ("OEdit:  That vnum does not exist.\n\r", ch);
            return;
        }

        if (!IS_BUILDER (ch, pObj->area))
        {
            send_to_char ("Insufficient security to modify objects.\n\r", ch);
            return;
        }

        ch->desc->pEdit = (void *) pObj;
        ch->desc->editor = ED_OBJECT;
        return;
    }
    else
    {
        if (!str_cmp (arg1, "create"))
        {
            value = atoi (argument);
            if (argument[0] == '\0' || value == 0)
            {
                send_to_char ("Syntax:  edit object create [vnum]\n\r", ch);
                return;
            }

            pArea = get_vnum_area (value);

            if (!pArea)
            {
                send_to_char
                    ("OEdit:  That vnum is not assigned an area.\n\r", ch);
                return;
            }

            if (!IS_BUILDER (ch, pArea))
            {
                send_to_char ("Insufficient security to modify objects.\n\r",
                              ch);
                return;
            }

            if (oedit_create (ch, argument))
            {
                SET_BIT (pArea->area_flags, AREA_CHANGED);
                ch->desc->editor = ED_OBJECT;
            }
            return;
        }
    }

    send_to_char ("OEdit:  There is no default object to edit.\n\r", ch);
    return;
}



/* Entry point for editing mob_index_data. */
void do_medit (CHAR_DATA * ch, char *argument)
{
    MOB_INDEX_DATA *pMob;
    AREA_DATA *pArea;
    int value;
    char arg1[MAX_STRING_LENGTH];

    argument = one_argument (argument, arg1);

    if (IS_NPC (ch))
        return;

    if (is_number (arg1))
    {
        value = atoi (arg1);
        if (!(pMob = get_mob_index (value)))
        {
            send_to_char ("MEdit:  That vnum does not exist.\n\r", ch);
            return;
        }

        if (!IS_BUILDER (ch, pMob->area))
        {
            send_to_char ("Insufficient security to modify mobs.\n\r", ch);
            return;
        }

        ch->desc->pEdit = (void *) pMob;
        ch->desc->editor = ED_MOBILE;
        return;
    }
    else
    {
        if (!str_cmp (arg1, "create"))
        {
            value = atoi (argument);
            if (arg1[0] == '\0' || value == 0)
            {
                send_to_char ("Syntax:  edit mobile create [vnum]\n\r", ch);
                return;
            }

            pArea = get_vnum_area (value);

            if (!pArea)
            {
                send_to_char
                    ("OEdit:  That vnum is not assigned an area.\n\r", ch);
                return;
            }

            if (!IS_BUILDER (ch, pArea))
            {
                send_to_char ("Insufficient security to modify mobs.\n\r",
                              ch);
                return;
            }

            if (medit_create (ch, argument))
            {
                SET_BIT (pArea->area_flags, AREA_CHANGED);
                ch->desc->editor = ED_MOBILE;
            }
            return;
        }
    }

    send_to_char ("MEdit:  There is no default mobile to edit.\n\r", ch);
    return;
}



void display_resets (CHAR_DATA * ch)
{
    ROOM_INDEX_DATA *pRoom;
    RESET_DATA *pReset;
    MOB_INDEX_DATA *pMob = NULL;
    char buf[MAX_STRING_LENGTH];
    char final[MAX_STRING_LENGTH];
    int iReset = 0;

    EDIT_ROOM (ch, pRoom);
    final[0] = '\0';

    send_to_char
        (" No.  Loads    Description       Location         Vnum   Mx Mn Description"
         "\n\r"
         "==== ======== ============= =================== ======== ===== ==========="
         "\n\r", ch);

    for (pReset = pRoom->reset_first; pReset; pReset = pReset->next)
    {
        OBJ_INDEX_DATA *pObj;
        MOB_INDEX_DATA *pMobIndex;
        OBJ_INDEX_DATA *pObjIndex;
        OBJ_INDEX_DATA *pObjToIndex;
        ROOM_INDEX_DATA *pRoomIndex;

        final[0] = '\0';
        sprintf (final, "[%2d] ", ++iReset);

        switch (pReset->command)
        {
            default:
                sprintf (buf, "Bad reset command: %c.", pReset->command);
                strcat (final, buf);
                break;

            case 'M':
                if (!(pMobIndex = get_mob_index (pReset->arg1)))
                {
                    sprintf (buf, "Load Mobile - Bad Mob %d\n\r",
                             pReset->arg1);
                    strcat (final, buf);
                    continue;
                }

                if (!(pRoomIndex = get_room_index (pReset->arg3)))
                {
                    sprintf (buf, "Load Mobile - Bad Room %d\n\r",
                             pReset->arg3);
                    strcat (final, buf);
                    continue;
                }

                pMob = pMobIndex;
                sprintf (buf,
                         "M[%5d] %-13.13s in room             R[%5d] %2d-%2d %-15.15s\n\r",
                         pReset->arg1, pMob->short_descr, pReset->arg3,
                         pReset->arg2, pReset->arg4, pRoomIndex->name);
                strcat (final, buf);

                /*
                 * Check for pet shop.
                 * -------------------
                 */
                {
                    ROOM_INDEX_DATA *pRoomIndexPrev;

                    pRoomIndexPrev = get_room_index (pRoomIndex->vnum - 1);
                    if (pRoomIndexPrev
                        && IS_SET (pRoomIndexPrev->room_flags, ROOM_PET_SHOP))
                        final[5] = 'P';
                }

                break;

            case 'O':
                if (!(pObjIndex = get_obj_index (pReset->arg1)))
                {
                    sprintf (buf, "Load Object - Bad Object %d\n\r",
                             pReset->arg1);
                    strcat (final, buf);
                    continue;
                }

                pObj = pObjIndex;

                if (!(pRoomIndex = get_room_index (pReset->arg3)))
                {
                    sprintf (buf, "Load Object - Bad Room %d\n\r",
                             pReset->arg3);
                    strcat (final, buf);
                    continue;
                }

                sprintf (buf, "O[%5d] %-13.13s in room             "
                         "R[%5d]       %-15.15s\n\r",
                         pReset->arg1, pObj->short_descr,
                         pReset->arg3, pRoomIndex->name);
                strcat (final, buf);

                break;

            case 'P':
                if (!(pObjIndex = get_obj_index (pReset->arg1)))
                {
                    sprintf (buf, "Put Object - Bad Object %d\n\r",
                             pReset->arg1);
                    strcat (final, buf);
                    continue;
                }

                pObj = pObjIndex;

                if (!(pObjToIndex = get_obj_index (pReset->arg3)))
                {
                    sprintf (buf, "Put Object - Bad To Object %d\n\r",
                             pReset->arg3);
                    strcat (final, buf);
                    continue;
                }

                sprintf (buf,
                         "O[%5d] %-13.13s inside              O[%5d] %2d-%2d %-15.15s\n\r",
                         pReset->arg1,
                         pObj->short_descr,
                         pReset->arg3,
                         pReset->arg2, pReset->arg4,
                         pObjToIndex->short_descr);
                strcat (final, buf);

                break;

            case 'G':
            case 'E':
                if (!(pObjIndex = get_obj_index (pReset->arg1)))
                {
                    sprintf (buf, "Give/Equip Object - Bad Object %d\n\r",
                             pReset->arg1);
                    strcat (final, buf);
                    continue;
                }

                pObj = pObjIndex;

                if (!pMob)
                {
                    sprintf (buf,
                             "Give/Equip Object - No Previous Mobile\n\r");
                    strcat (final, buf);
                    break;
                }

                if (pMob->pShop)
                {
                    sprintf (buf,
                             "O[%5d] %-13.13s in the inventory of S[%5d]       %-15.15s\n\r",
                             pReset->arg1,
                             pObj->short_descr, pMob->vnum,
                             pMob->short_descr);
                }
                else
                    sprintf (buf,
                             "O[%5d] %-13.13s %-19.19s M[%5d]       %-15.15s\n\r",
                             pReset->arg1,
                             pObj->short_descr,
                             (pReset->command == 'G') ?
                             flag_string (wear_loc_strings, WEAR_NONE)
                             : flag_string (wear_loc_strings, pReset->arg3),
                             pMob->vnum, pMob->short_descr);
                strcat (final, buf);

                break;

                /*
                 * Doors are set in rs_flags don't need to be displayed.
                 * If you want to display them then uncomment the new_reset
                 * line in the case 'D' in load_resets in db.c and here.
                 */
            case 'D':
                pRoomIndex = get_room_index (pReset->arg1);
                sprintf (buf, "R[%5d] %s door of %-19.19s reset to %s\n\r",
                         pReset->arg1,
                         capitalize (dir_name[pReset->arg2]),
                         pRoomIndex->name,
                         flag_string (door_resets, pReset->arg3));
                strcat (final, buf);

                break;
                /*
                 * End Doors Comment.
                 */
            case 'R':
                if (!(pRoomIndex = get_room_index (pReset->arg1)))
                {
                    sprintf (buf, "Randomize Exits - Bad Room %d\n\r",
                             pReset->arg1);
                    strcat (final, buf);
                    continue;
                }

                sprintf (buf, "R[%5d] Exits are randomized in %s\n\r",
                         pReset->arg1, pRoomIndex->name);
                strcat (final, buf);

                break;
        }
        send_to_char (final, ch);
    }

    return;
}



/*****************************************************************************
 Name:        add_reset
 Purpose:    Inserts a new reset in the given index slot.
 Called by:    do_resets(olc.c).
 ****************************************************************************/
void add_reset (ROOM_INDEX_DATA * room, RESET_DATA * pReset, int index)
{
    RESET_DATA *reset;
    int iReset = 0;

    if (!room->reset_first)
    {
        room->reset_first = pReset;
        room->reset_last = pReset;
        pReset->next = NULL;
        return;
    }

    index--;

    if (index == 0)
    {                            /* First slot (1) selected. */
        pReset->next = room->reset_first;
        room->reset_first = pReset;
        return;
    }

    /*
     * If negative slot( <= 0 selected) then this will find the last.
     */
    for (reset = room->reset_first; reset->next; reset = reset->next)
    {
        if (++iReset == index)
            break;
    }

    pReset->next = reset->next;
    reset->next = pReset;
    if (!pReset->next)
        room->reset_last = pReset;
    return;
}



void do_resets (CHAR_DATA * ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    char arg4[MAX_INPUT_LENGTH];
    char arg5[MAX_INPUT_LENGTH];
    char arg6[MAX_INPUT_LENGTH];
    char arg7[MAX_INPUT_LENGTH];
    RESET_DATA *pReset = NULL;

    argument = one_argument (argument, arg1);
    argument = one_argument (argument, arg2);
    argument = one_argument (argument, arg3);
    argument = one_argument (argument, arg4);
    argument = one_argument (argument, arg5);
    argument = one_argument (argument, arg6);
    argument = one_argument (argument, arg7);

    if (!IS_BUILDER (ch, ch->in_room->area))
    {
        send_to_char ("Resets: Invalid security for editing this area.\n\r",
                      ch);
        return;
    }

    /*
     * Display resets in current room.
     * -------------------------------
     */
    if (arg1[0] == '\0')
    {
        if (ch->in_room->reset_first)
        {
            send_to_char ("Resets: M = mobile, R = room, O = object, "
                          "P = pet, S = shopkeeper\n\r", ch);
            display_resets (ch);
        }
        else
            send_to_char ("No resets in this room.\n\r", ch);
    }


    /*
     * Take index number and search for commands.
     * ------------------------------------------
     */
    if (is_number (arg1))
    {
        ROOM_INDEX_DATA *pRoom = ch->in_room;

        /*
         * Delete a reset.
         * ---------------
         */
        if (!str_cmp (arg2, "delete"))
        {
            int insert_loc = atoi (arg1);

            if (!ch->in_room->reset_first)
            {
                send_to_char ("No resets in this area.\n\r", ch);
                return;
            }

            if (insert_loc - 1 <= 0)
            {
                pReset = pRoom->reset_first;
                pRoom->reset_first = pRoom->reset_first->next;
                if (!pRoom->reset_first)
                    pRoom->reset_last = NULL;
            }
            else
            {
                int iReset = 0;
                RESET_DATA *prev = NULL;

                for (pReset = pRoom->reset_first;
                     pReset; pReset = pReset->next)
                {
                    if (++iReset == insert_loc)
                        break;
                    prev = pReset;
                }

                if (!pReset)
                {
                    send_to_char ("Reset not found.\n\r", ch);
                    return;
                }

                if (prev)
                    prev->next = prev->next->next;
                else
                    pRoom->reset_first = pRoom->reset_first->next;

                for (pRoom->reset_last = pRoom->reset_first;
                     pRoom->reset_last->next;
                     pRoom->reset_last = pRoom->reset_last->next);
            }

            free_reset_data (pReset);
            send_to_char ("Reset deleted.\n\r", ch);
        }
        else
            /*
             * Add a reset.
             * ------------
             */
            if ((!str_cmp (arg2, "mob") && is_number (arg3))
                || (!str_cmp (arg2, "obj") && is_number (arg3)))
        {
            /*
             * Check for Mobile reset.
             * -----------------------
             */
            if (!str_cmp (arg2, "mob"))
            {
                if (get_mob_index (is_number (arg3) ? atoi (arg3) : 1) ==
                    NULL)
                {
                    send_to_char ("Mob doesn't exist.\n\r", ch);
                    return;
                }
                pReset = new_reset_data ();
                pReset->command = 'M';
                pReset->arg1 = atoi (arg3);
                pReset->arg2 = is_number (arg4) ? atoi (arg4) : 1;    /* Max # */
                pReset->arg3 = ch->in_room->vnum;
                pReset->arg4 = is_number (arg5) ? atoi (arg5) : 1;    /* Min # */
            }
            else
                /*
                 * Check for Object reset.
                 * -----------------------
                 */
            if (!str_cmp (arg2, "obj"))
            {
                pReset = new_reset_data ();
                pReset->arg1 = atoi (arg3);
                /*
                 * Inside another object.
                 * ----------------------
                 */
                if (!str_prefix (arg4, "inside"))
                {
                    OBJ_INDEX_DATA *temp;

                    temp = get_obj_index (is_number (arg5) ? atoi (arg5) : 1);
                    if ((temp->item_type != ITEM_CONTAINER) &&
                        (temp->item_type != ITEM_CORPSE_NPC))
                    {
                        send_to_char ("Object 2 is not a container.\n\r", ch);
                        return;
                    }
                    pReset->command = 'P';
                    pReset->arg2 = is_number (arg6) ? atoi (arg6) : 1;
                    pReset->arg3 = is_number (arg5) ? atoi (arg5) : 1;
                    pReset->arg4 = is_number (arg7) ? atoi (arg7) : 1;
                }
                else
                    /*
                     * Inside the room.
                     * ----------------
                     */
                if (!str_cmp (arg4, "room"))
                {
                    if (get_obj_index (atoi (arg3)) == NULL)
                    {
                        send_to_char ("Vnum doesn't exist.\n\r", ch);
                        return;
                    }
                    pReset->command = 'O';
                    pReset->arg2 = 0;
                    pReset->arg3 = ch->in_room->vnum;
                    pReset->arg4 = 0;
                }
                else
                    /*
                     * Into a Mobile's inventory.
                     * --------------------------
                     */
                {
                    if (flag_value (wear_loc_flags, arg4) == NO_FLAG)
                    {
                        send_to_char ("Resets: '? wear-loc'\n\r", ch);
                        return;
                    }
                    if (get_obj_index (atoi (arg3)) == NULL)
                    {
                        send_to_char ("Vnum doesn't exist.\n\r", ch);
                        return;
                    }
                    pReset->arg1 = atoi (arg3);
                    pReset->arg3 = flag_value (wear_loc_flags, arg4);
                    if (pReset->arg3 == WEAR_NONE)
                        pReset->command = 'G';
                    else
                        pReset->command = 'E';
                }
            }
            add_reset (ch->in_room, pReset, atoi (arg1));
            SET_BIT (ch->in_room->area->area_flags, AREA_CHANGED);
            send_to_char ("Reset added.\n\r", ch);
        }
        else if (!str_cmp (arg2, "random") && is_number (arg3))
        {
            if (atoi (arg3) < 1 || atoi (arg3) > 6)
            {
                send_to_char ("Invalid argument.\n\r", ch);
                return;
            }
            pReset = new_reset_data ();
            pReset->command = 'R';
            pReset->arg1 = ch->in_room->vnum;
            pReset->arg2 = atoi (arg3);
            add_reset (ch->in_room, pReset, atoi (arg1));
            SET_BIT (ch->in_room->area->area_flags, AREA_CHANGED);
            send_to_char ("Random exits reset added.\n\r", ch);
        }
        else
        {
            send_to_char ("Syntax: RESET <number> OBJ <vnum> <wear_loc>\n\r",
                          ch);
            send_to_char
                ("        RESET <number> OBJ <vnum> inside <vnum> [limit] [count]\n\r",
                 ch);
            send_to_char ("        RESET <number> OBJ <vnum> room\n\r", ch);
            send_to_char
                ("        RESET <number> MOB <vnum> [max #x area] [max #x room]\n\r",
                 ch);
            send_to_char ("        RESET <number> DELETE\n\r", ch);
            send_to_char ("        RESET <number> RANDOM [#x exits]\n\r", ch);
        }
    }

    return;
}



/*****************************************************************************
 Name:        do_alist
 Purpose:    Normal command to list areas and display area information.
 Called by:    interpreter(interp.c)
 ****************************************************************************/
void do_alist (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char result[MAX_STRING_LENGTH * 2];    /* May need tweaking. */
    AREA_DATA *pArea;

    if (IS_NPC (ch))
        return;

    sprintf (result, "[%3s] [%-27s] (%-5s-%5s) [%-10s] %3s [%-10s]\n\r",
             "Num", "Area Name", "lvnum", "uvnum", "Filename", "Sec",
             "Builders");

    for (pArea = area_first; pArea; pArea = pArea->next)
    {
        sprintf (buf,
                 "[%3d] %-29.29s (%-5d-%5d) %-12.12s [%d] [%-10.10s]\n\r",
                 pArea->vnum, pArea->name, pArea->min_vnum, pArea->max_vnum,
                 pArea->file_name, pArea->security, pArea->builders);
        strcat (result, buf);
    }

    send_to_char (result, ch);
    return;
}
