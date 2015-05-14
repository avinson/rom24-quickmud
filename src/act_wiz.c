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

/*   QuickMUD - The Lazy Man's ROM - $Id: act_wiz.c,v 1.3 2000/12/01 10:48:33 ring0 Exp $ */

#if defined(macintosh)
#include <types.h>
#include <time.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>                /* For execl in copyover() */
#include "merc.h"
#include "interp.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"

/*
 * Stolen from save.c for reading in QuickMUD config stuff
 */
#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value )			\
                if ( !str_cmp( word, literal ) )	\
		{					\
			field  = value;			\
			fMatch = TRUE;			\
			break;				\
		}

/*
 * Local functions.
 */
ROOM_INDEX_DATA *find_location args ((CHAR_DATA * ch, char *arg));

void do_wiznet (CHAR_DATA * ch, char *argument)
{
    int flag;
    char buf[MAX_STRING_LENGTH];

    if (argument[0] == '\0')
    {
        if (IS_SET (ch->wiznet, WIZ_ON))
        {
            send_to_char ("Signing off of Wiznet.\n\r", ch);
            REMOVE_BIT (ch->wiznet, WIZ_ON);
        }
        else
        {
            send_to_char ("Welcome to Wiznet!\n\r", ch);
            SET_BIT (ch->wiznet, WIZ_ON);
        }
        return;
    }

    if (!str_prefix (argument, "on"))
    {
        send_to_char ("Welcome to Wiznet!\n\r", ch);
        SET_BIT (ch->wiznet, WIZ_ON);
        return;
    }

    if (!str_prefix (argument, "off"))
    {
        send_to_char ("Signing off of Wiznet.\n\r", ch);
        REMOVE_BIT (ch->wiznet, WIZ_ON);
        return;
    }

    /* show wiznet status */
    if (!str_prefix (argument, "status"))
    {
        buf[0] = '\0';

        if (!IS_SET (ch->wiznet, WIZ_ON))
            strcat (buf, "off ");

        for (flag = 0; wiznet_table[flag].name != NULL; flag++)
            if (IS_SET (ch->wiznet, wiznet_table[flag].flag))
            {
                strcat (buf, wiznet_table[flag].name);
                strcat (buf, " ");
            }

        strcat (buf, "\n\r");

        send_to_char ("Wiznet status:\n\r", ch);
        send_to_char (buf, ch);
        return;
    }

    if (!str_prefix (argument, "show"))
        /* list of all wiznet options */
    {
        buf[0] = '\0';

        for (flag = 0; wiznet_table[flag].name != NULL; flag++)
        {
            if (wiznet_table[flag].level <= get_trust (ch))
            {
                strcat (buf, wiznet_table[flag].name);
                strcat (buf, " ");
            }
        }

        strcat (buf, "\n\r");

        send_to_char ("Wiznet options available to you are:\n\r", ch);
        send_to_char (buf, ch);
        return;
    }

    flag = wiznet_lookup (argument);

    if (flag == -1 || get_trust (ch) < wiznet_table[flag].level)
    {
        send_to_char ("No such option.\n\r", ch);
        return;
    }

    if (IS_SET (ch->wiznet, wiznet_table[flag].flag))
    {
        sprintf (buf, "You will no longer see %s on wiznet.\n\r",
                 wiznet_table[flag].name);
        send_to_char (buf, ch);
        REMOVE_BIT (ch->wiznet, wiznet_table[flag].flag);
        return;
    }
    else
    {
        sprintf (buf, "You will now see %s on wiznet.\n\r",
                 wiznet_table[flag].name);
        send_to_char (buf, ch);
        SET_BIT (ch->wiznet, wiznet_table[flag].flag);
        return;
    }

}

void wiznet (char *string, CHAR_DATA * ch, OBJ_DATA * obj,
             long flag, long flag_skip, int min_level)
{
    DESCRIPTOR_DATA *d;

    for (d = descriptor_list; d != NULL; d = d->next)
    {
        if (d->connected == CON_PLAYING && IS_IMMORTAL (d->character)
            && IS_SET (d->character->wiznet, WIZ_ON)
            && (!flag || IS_SET (d->character->wiznet, flag))
            && (!flag_skip || !IS_SET (d->character->wiznet, flag_skip))
            && get_trust (d->character) >= min_level && d->character != ch)
        {
            if (IS_SET (d->character->wiznet, WIZ_PREFIX))
                send_to_char ("{Z--> ", d->character);
            else
                send_to_char ("{Z", d->character);
            act_new (string, d->character, obj, ch, TO_CHAR, POS_DEAD);
            send_to_char ("{x", d->character);
        }
    }

    return;
}

void do_guild (CHAR_DATA * ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    int clan;

    argument = one_argument (argument, arg1);
    argument = one_argument (argument, arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0')
    {
        send_to_char ("Syntax: guild <char> <cln name>\n\r", ch);
        return;
    }
    if ((victim = get_char_world (ch, arg1)) == NULL)
    {
        send_to_char ("They aren't playing.\n\r", ch);
        return;
    }

    if (!str_prefix (arg2, "none"))
    {
        send_to_char ("They are now clanless.\n\r", ch);
        send_to_char ("You are now a member of no clan!\n\r", victim);
        victim->clan = 0;
        return;
    }

    if ((clan = clan_lookup (arg2)) == 0)
    {
        send_to_char ("No such clan exists.\n\r", ch);
        return;
    }

    if (clan_table[clan].independent)
    {
        sprintf (buf, "They are now a %s.\n\r", clan_table[clan].name);
        send_to_char (buf, ch);
        sprintf (buf, "You are now a %s.\n\r", clan_table[clan].name);
        send_to_char (buf, victim);
    }
    else
    {
        sprintf (buf, "They are now a member of clan %s.\n\r",
                 capitalize (clan_table[clan].name));
        send_to_char (buf, ch);
        sprintf (buf, "You are now a member of clan %s.\n\r",
                 capitalize (clan_table[clan].name));
    }

    victim->clan = clan;
}

/* equips a character */
void do_outfit (CHAR_DATA * ch, char *argument)
{
    OBJ_DATA *obj;
    int i, sn, vnum;

    if (ch->level > 5 || IS_NPC (ch))
    {
        send_to_char ("Find it yourself!\n\r", ch);
        return;
    }

    if ((obj = get_eq_char (ch, WEAR_LIGHT)) == NULL)
    {
        obj = create_object (get_obj_index (OBJ_VNUM_SCHOOL_BANNER), 0);
        obj->cost = 0;
        obj_to_char (obj, ch);
        equip_char (ch, obj, WEAR_LIGHT);
    }

    if ((obj = get_eq_char (ch, WEAR_BODY)) == NULL)
    {
        obj = create_object (get_obj_index (OBJ_VNUM_SCHOOL_VEST), 0);
        obj->cost = 0;
        obj_to_char (obj, ch);
        equip_char (ch, obj, WEAR_BODY);
    }

    /* do the weapon thing */
    if ((obj = get_eq_char (ch, WEAR_WIELD)) == NULL)
    {
        sn = 0;
        vnum = OBJ_VNUM_SCHOOL_SWORD;    /* just in case! */

        for (i = 0; weapon_table[i].name != NULL; i++)
        {
            if (ch->pcdata->learned[sn] <
                ch->pcdata->learned[*weapon_table[i].gsn])
            {
                sn = *weapon_table[i].gsn;
                vnum = weapon_table[i].vnum;
            }
        }

        obj = create_object (get_obj_index (vnum), 0);
        obj_to_char (obj, ch);
        equip_char (ch, obj, WEAR_WIELD);
    }

    if (((obj = get_eq_char (ch, WEAR_WIELD)) == NULL
         || !IS_WEAPON_STAT (obj, WEAPON_TWO_HANDS))
        && (obj = get_eq_char (ch, WEAR_SHIELD)) == NULL)
    {
        obj = create_object (get_obj_index (OBJ_VNUM_SCHOOL_SHIELD), 0);
        obj->cost = 0;
        obj_to_char (obj, ch);
        equip_char (ch, obj, WEAR_SHIELD);
    }

    send_to_char ("You have been equipped by Mota.\n\r", ch);
}


/* RT nochannels command, for those spammers */
void do_nochannels (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char ("Nochannel whom?", ch);
        return;
    }

    if ((victim = get_char_world (ch, arg)) == NULL)
    {
        send_to_char ("They aren't here.\n\r", ch);
        return;
    }

    if (get_trust (victim) >= get_trust (ch))
    {
        send_to_char ("You failed.\n\r", ch);
        return;
    }

    if (IS_SET (victim->comm, COMM_NOCHANNELS))
    {
        REMOVE_BIT (victim->comm, COMM_NOCHANNELS);
        send_to_char ("The gods have restored your channel priviliges.\n\r",
                      victim);
        send_to_char ("NOCHANNELS removed.\n\r", ch);
        sprintf (buf, "$N restores channels to %s", victim->name);
        wiznet (buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }
    else
    {
        SET_BIT (victim->comm, COMM_NOCHANNELS);
        send_to_char ("The gods have revoked your channel priviliges.\n\r",
                      victim);
        send_to_char ("NOCHANNELS set.\n\r", ch);
        sprintf (buf, "$N revokes %s's channels.", victim->name);
        wiznet (buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }

    return;
}


void do_smote (CHAR_DATA * ch, char *argument)
{
    CHAR_DATA *vch;
    char *letter, *name;
    char last[MAX_INPUT_LENGTH], temp[MAX_STRING_LENGTH];
    int matches = 0;

    if (!IS_NPC (ch) && IS_SET (ch->comm, COMM_NOEMOTE))
    {
        send_to_char ("You can't show your emotions.\n\r", ch);
        return;
    }

    if (argument[0] == '\0')
    {
        send_to_char ("Emote what?\n\r", ch);
        return;
    }

    if (strstr (argument, ch->name) == NULL)
    {
        send_to_char ("You must include your name in an smote.\n\r", ch);
        return;
    }

    send_to_char (argument, ch);
    send_to_char ("\n\r", ch);

    for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
    {
        if (vch->desc == NULL || vch == ch)
            continue;

        if ((letter = strstr (argument, vch->name)) == NULL)
        {
            send_to_char (argument, vch);
            send_to_char ("\n\r", vch);
            continue;
        }

        strcpy (temp, argument);
        temp[strlen (argument) - strlen (letter)] = '\0';
        last[0] = '\0';
        name = vch->name;

        for (; *letter != '\0'; letter++)
        {
            if (*letter == '\'' && matches == strlen (vch->name))
            {
                strcat (temp, "r");
                continue;
            }

            if (*letter == 's' && matches == strlen (vch->name))
            {
                matches = 0;
                continue;
            }

            if (matches == strlen (vch->name))
            {
                matches = 0;
            }

            if (*letter == *name)
            {
                matches++;
                name++;
                if (matches == strlen (vch->name))
                {
                    strcat (temp, "you");
                    last[0] = '\0';
                    name = vch->name;
                    continue;
                }
                strncat (last, letter, 1);
                continue;
            }

            matches = 0;
            strcat (temp, last);
            strncat (temp, letter, 1);
            last[0] = '\0';
            name = vch->name;
        }

        send_to_char (temp, vch);
        send_to_char ("\n\r", vch);
    }

    return;
}

void do_bamfin (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];

    if (!IS_NPC (ch))
    {
        smash_tilde (argument);

        if (argument[0] == '\0')
        {
            sprintf (buf, "Your poofin is %s\n\r", ch->pcdata->bamfin);
            send_to_char (buf, ch);
            return;
        }

        if (strstr (argument, ch->name) == NULL)
        {
            send_to_char ("You must include your name.\n\r", ch);
            return;
        }

        free_string (ch->pcdata->bamfin);
        ch->pcdata->bamfin = str_dup (argument);

        sprintf (buf, "Your poofin is now %s\n\r", ch->pcdata->bamfin);
        send_to_char (buf, ch);
    }
    return;
}

void do_bamfout (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];

    if (!IS_NPC (ch))
    {
        smash_tilde (argument);

        if (argument[0] == '\0')
        {
            sprintf (buf, "Your poofout is %s\n\r", ch->pcdata->bamfout);
            send_to_char (buf, ch);
            return;
        }

        if (strstr (argument, ch->name) == NULL)
        {
            send_to_char ("You must include your name.\n\r", ch);
            return;
        }

        free_string (ch->pcdata->bamfout);
        ch->pcdata->bamfout = str_dup (argument);

        sprintf (buf, "Your poofout is now %s\n\r", ch->pcdata->bamfout);
        send_to_char (buf, ch);
    }
    return;
}



void do_deny (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    one_argument (argument, arg);
    if (arg[0] == '\0')
    {
        send_to_char ("Deny whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_world (ch, arg)) == NULL)
    {
        send_to_char ("They aren't here.\n\r", ch);
        return;
    }

    if (IS_NPC (victim))
    {
        send_to_char ("Not on NPC's.\n\r", ch);
        return;
    }

    if (get_trust (victim) >= get_trust (ch))
    {
        send_to_char ("You failed.\n\r", ch);
        return;
    }

    SET_BIT (victim->act, PLR_DENY);
    send_to_char ("You are denied access!\n\r", victim);
    sprintf (buf, "$N denies access to %s", victim->name);
    wiznet (buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    send_to_char ("OK.\n\r", ch);
    save_char_obj (victim);
    stop_fighting (victim, TRUE);
    do_function (victim, &do_quit, "");

    return;
}



void do_disconnect (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    DESCRIPTOR_DATA *d;
    CHAR_DATA *victim;

    one_argument (argument, arg);
    if (arg[0] == '\0')
    {
        send_to_char ("Disconnect whom?\n\r", ch);
        return;
    }

    if (is_number (arg))
    {
        int desc;

        desc = atoi (arg);
        for (d = descriptor_list; d != NULL; d = d->next)
        {
            if (d->descriptor == desc)
            {
                close_socket (d);
                send_to_char ("Ok.\n\r", ch);
                return;
            }
        }
    }

    if ((victim = get_char_world (ch, arg)) == NULL)
    {
        send_to_char ("They aren't here.\n\r", ch);
        return;
    }

    if (victim->desc == NULL)
    {
        act ("$N doesn't have a descriptor.", ch, NULL, victim, TO_CHAR);
        return;
    }

    for (d = descriptor_list; d != NULL; d = d->next)
    {
        if (d == victim->desc)
        {
            close_socket (d);
            send_to_char ("Ok.\n\r", ch);
            return;
        }
    }

    bug ("Do_disconnect: desc not found.", 0);
    send_to_char ("Descriptor not found!\n\r", ch);
    return;
}



void do_pardon (CHAR_DATA * ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    argument = one_argument (argument, arg1);
    argument = one_argument (argument, arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0')
    {
        send_to_char ("Syntax: pardon <character> <killer|thief>.\n\r", ch);
        return;
    }

    if ((victim = get_char_world (ch, arg1)) == NULL)
    {
        send_to_char ("They aren't here.\n\r", ch);
        return;
    }

    if (IS_NPC (victim))
    {
        send_to_char ("Not on NPC's.\n\r", ch);
        return;
    }

    if (!str_cmp (arg2, "killer"))
    {
        if (IS_SET (victim->act, PLR_KILLER))
        {
            REMOVE_BIT (victim->act, PLR_KILLER);
            send_to_char ("Killer flag removed.\n\r", ch);
            send_to_char ("You are no longer a KILLER.\n\r", victim);
        }
        return;
    }

    if (!str_cmp (arg2, "thief"))
    {
        if (IS_SET (victim->act, PLR_THIEF))
        {
            REMOVE_BIT (victim->act, PLR_THIEF);
            send_to_char ("Thief flag removed.\n\r", ch);
            send_to_char ("You are no longer a THIEF.\n\r", victim);
        }
        return;
    }

    send_to_char ("Syntax: pardon <character> <killer|thief>.\n\r", ch);
    return;
}



void do_echo (CHAR_DATA * ch, char *argument)
{
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0')
    {
        send_to_char ("Global echo what?\n\r", ch);
        return;
    }

    for (d = descriptor_list; d; d = d->next)
    {
        if (d->connected == CON_PLAYING)
        {
            if (get_trust (d->character) >= get_trust (ch))
                send_to_char ("global> ", d->character);
            send_to_char (argument, d->character);
            send_to_char ("\n\r", d->character);
        }
    }

    return;
}



void do_recho (CHAR_DATA * ch, char *argument)
{
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0')
    {
        send_to_char ("Local echo what?\n\r", ch);

        return;
    }

    for (d = descriptor_list; d; d = d->next)
    {
        if (d->connected == CON_PLAYING
            && d->character->in_room == ch->in_room)
        {
            if (get_trust (d->character) >= get_trust (ch))
                send_to_char ("local> ", d->character);
            send_to_char (argument, d->character);
            send_to_char ("\n\r", d->character);
        }
    }

    return;
}

void do_zecho (CHAR_DATA * ch, char *argument)
{
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0')
    {
        send_to_char ("Zone echo what?\n\r", ch);
        return;
    }

    for (d = descriptor_list; d; d = d->next)
    {
        if (d->connected == CON_PLAYING
            && d->character->in_room != NULL && ch->in_room != NULL
            && d->character->in_room->area == ch->in_room->area)
        {
            if (get_trust (d->character) >= get_trust (ch))
                send_to_char ("zone> ", d->character);
            send_to_char (argument, d->character);
            send_to_char ("\n\r", d->character);
        }
    }
}

void do_pecho (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    argument = one_argument (argument, arg);

    if (argument[0] == '\0' || arg[0] == '\0')
    {
        send_to_char ("Personal echo what?\n\r", ch);
        return;
    }

    if ((victim = get_char_world (ch, arg)) == NULL)
    {
        send_to_char ("Target not found.\n\r", ch);
        return;
    }

    if (get_trust (victim) >= get_trust (ch) && get_trust (ch) != MAX_LEVEL)
        send_to_char ("personal> ", victim);

    send_to_char (argument, victim);
    send_to_char ("\n\r", victim);
    send_to_char ("personal> ", ch);
    send_to_char (argument, ch);
    send_to_char ("\n\r", ch);
}


ROOM_INDEX_DATA *find_location (CHAR_DATA * ch, char *arg)
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;

    if (is_number (arg))
        return get_room_index (atoi (arg));

    if ((victim = get_char_world (ch, arg)) != NULL)
        return victim->in_room;

    if ((obj = get_obj_world (ch, arg)) != NULL)
        return obj->in_room;

    return NULL;
}



void do_transfer (CHAR_DATA * ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    DESCRIPTOR_DATA *d;
    CHAR_DATA *victim;

    argument = one_argument (argument, arg1);
    argument = one_argument (argument, arg2);

    if (arg1[0] == '\0')
    {
        send_to_char ("Transfer whom (and where)?\n\r", ch);
        return;
    }

    if (!str_cmp (arg1, "all"))
    {
        for (d = descriptor_list; d != NULL; d = d->next)
        {
            if (d->connected == CON_PLAYING
                && d->character != ch
                && d->character->in_room != NULL
                && can_see (ch, d->character))
            {
                char buf[MAX_STRING_LENGTH];
                sprintf (buf, "%s %s", d->character->name, arg2);
                do_function (ch, &do_transfer, buf);
            }
        }
        return;
    }

    /*
     * Thanks to Grodyn for the optional location parameter.
     */
    if (arg2[0] == '\0')
    {
        location = ch->in_room;
    }
    else
    {
        if ((location = find_location (ch, arg2)) == NULL)
        {
            send_to_char ("No such location.\n\r", ch);
            return;
        }

        if (!is_room_owner (ch, location) && room_is_private (location)
            && get_trust (ch) < MAX_LEVEL)
        {
            send_to_char ("That room is private right now.\n\r", ch);
            return;
        }
    }

    if ((victim = get_char_world (ch, arg1)) == NULL)
    {
        send_to_char ("They aren't here.\n\r", ch);
        return;
    }

    if (victim->in_room == NULL)
    {
        send_to_char ("They are in limbo.\n\r", ch);
        return;
    }

    if (victim->fighting != NULL)
        stop_fighting (victim, TRUE);
    act ("$n disappears in a mushroom cloud.", victim, NULL, NULL, TO_ROOM);
    char_from_room (victim);
    char_to_room (victim, location);
    act ("$n arrives from a puff of smoke.", victim, NULL, NULL, TO_ROOM);
    if (ch != victim)
        act ("$n has transferred you.", ch, NULL, victim, TO_VICT);
    do_function (victim, &do_look, "auto");
    send_to_char ("Ok.\n\r", ch);
}



void do_at (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    ROOM_INDEX_DATA *original;
    OBJ_DATA *on;
    CHAR_DATA *wch;

    argument = one_argument (argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0')
    {
        send_to_char ("At where what?\n\r", ch);
        return;
    }

    if ((location = find_location (ch, arg)) == NULL)
    {
        send_to_char ("No such location.\n\r", ch);
        return;
    }

    if (!is_room_owner (ch, location) && room_is_private (location)
        && get_trust (ch) < MAX_LEVEL)
    {
        send_to_char ("That room is private right now.\n\r", ch);
        return;
    }

    original = ch->in_room;
    on = ch->on;
    char_from_room (ch);
    char_to_room (ch, location);
    interpret (ch, argument);

    /*
     * See if 'ch' still exists before continuing!
     * Handles 'at XXXX quit' case.
     */
    for (wch = char_list; wch != NULL; wch = wch->next)
    {
        if (wch == ch)
        {
            char_from_room (ch);
            char_to_room (ch, original);
            ch->on = on;
            break;
        }
    }

    return;
}



void do_goto (CHAR_DATA * ch, char *argument)
{
    ROOM_INDEX_DATA *location;
    CHAR_DATA *rch;
    int count = 0;

    if (argument[0] == '\0')
    {
        send_to_char ("Goto where?\n\r", ch);
        return;
    }

    if ((location = find_location (ch, argument)) == NULL)
    {
        send_to_char ("No such location.\n\r", ch);
        return;
    }

    count = 0;
    for (rch = location->people; rch != NULL; rch = rch->next_in_room)
        count++;

    if (!is_room_owner (ch, location) && room_is_private (location)
        && (count > 1 || get_trust (ch) < MAX_LEVEL))
    {
        send_to_char ("That room is private right now.\n\r", ch);
        return;
    }

    if (ch->fighting != NULL)
        stop_fighting (ch, TRUE);

    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
    {
        if (get_trust (rch) >= ch->invis_level)
        {
            if (ch->pcdata != NULL && ch->pcdata->bamfout[0] != '\0')
                act ("$t", ch, ch->pcdata->bamfout, rch, TO_VICT);
            else
                act ("$n leaves in a swirling mist.", ch, NULL, rch, TO_VICT);
        }
    }

    char_from_room (ch);
    char_to_room (ch, location);


    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
    {
        if (get_trust (rch) >= ch->invis_level)
        {
            if (ch->pcdata != NULL && ch->pcdata->bamfin[0] != '\0')
                act ("$t", ch, ch->pcdata->bamfin, rch, TO_VICT);
            else
                act ("$n appears in a swirling mist.", ch, NULL, rch,
                     TO_VICT);
        }
    }

    do_function (ch, &do_look, "auto");
    return;
}

void do_violate (CHAR_DATA * ch, char *argument)
{
    ROOM_INDEX_DATA *location;
    CHAR_DATA *rch;

    if (argument[0] == '\0')
    {
        send_to_char ("Goto where?\n\r", ch);
        return;
    }

    if ((location = find_location (ch, argument)) == NULL)
    {
        send_to_char ("No such location.\n\r", ch);
        return;
    }

    if (!room_is_private (location))
    {
        send_to_char ("That room isn't private, use goto.\n\r", ch);
        return;
    }

    if (ch->fighting != NULL)
        stop_fighting (ch, TRUE);

    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
    {
        if (get_trust (rch) >= ch->invis_level)
        {
            if (ch->pcdata != NULL && ch->pcdata->bamfout[0] != '\0')
                act ("$t", ch, ch->pcdata->bamfout, rch, TO_VICT);
            else
                act ("$n leaves in a swirling mist.", ch, NULL, rch, TO_VICT);
        }
    }

    char_from_room (ch);
    char_to_room (ch, location);


    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
    {
        if (get_trust (rch) >= ch->invis_level)
        {
            if (ch->pcdata != NULL && ch->pcdata->bamfin[0] != '\0')
                act ("$t", ch, ch->pcdata->bamfin, rch, TO_VICT);
            else
                act ("$n appears in a swirling mist.", ch, NULL, rch,
                     TO_VICT);
        }
    }

    do_function (ch, &do_look, "auto");
    return;
}

/* RT to replace the 3 stat commands */

void do_stat (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char *string;
    OBJ_DATA *obj;
    ROOM_INDEX_DATA *location;
    CHAR_DATA *victim;

    string = one_argument (argument, arg);
    if (arg[0] == '\0')
    {
        send_to_char ("Syntax:\n\r", ch);
        send_to_char ("  stat <name>\n\r", ch);
        send_to_char ("  stat obj <name>\n\r", ch);
        send_to_char ("  stat mob <name>\n\r", ch);
        send_to_char ("  stat room <number>\n\r", ch);
        return;
    }

    if (!str_cmp (arg, "room"))
    {
        do_function (ch, &do_rstat, string);
        return;
    }

    if (!str_cmp (arg, "obj"))
    {
        do_function (ch, &do_ostat, string);
        return;
    }

    if (!str_cmp (arg, "char") || !str_cmp (arg, "mob"))
    {
        do_function (ch, &do_mstat, string);
        return;
    }

    /* do it the old way */

    obj = get_obj_world (ch, argument);
    if (obj != NULL)
    {
        do_function (ch, &do_ostat, argument);
        return;
    }

    victim = get_char_world (ch, argument);
    if (victim != NULL)
    {
        do_function (ch, &do_mstat, argument);
        return;
    }

    location = find_location (ch, argument);
    if (location != NULL)
    {
        do_function (ch, &do_rstat, argument);
        return;
    }

    send_to_char ("Nothing by that name found anywhere.\n\r", ch);
}

void do_rstat (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    OBJ_DATA *obj;
    CHAR_DATA *rch;
    int door;

    one_argument (argument, arg);
    location = (arg[0] == '\0') ? ch->in_room : find_location (ch, arg);
    if (location == NULL)
    {
        send_to_char ("No such location.\n\r", ch);
        return;
    }

    if (!is_room_owner (ch, location) && ch->in_room != location
        && room_is_private (location) && !IS_TRUSTED (ch, IMPLEMENTOR))
    {
        send_to_char ("That room is private right now.\n\r", ch);
        return;
    }

    sprintf (buf, "Name: '%s'\n\rArea: '%s'\n\r",
             location->name, location->area->name);
    send_to_char (buf, ch);

    sprintf (buf,
             "Vnum: %d  Sector: %d  Light: %d  Healing: %d  Mana: %d\n\r",
             location->vnum,
             location->sector_type,
             location->light, location->heal_rate, location->mana_rate);
    send_to_char (buf, ch);

    sprintf (buf,
             "Room flags: %d.\n\rDescription:\n\r%s",
             location->room_flags, location->description);
    send_to_char (buf, ch);

    if (location->extra_descr != NULL)
    {
        EXTRA_DESCR_DATA *ed;

        send_to_char ("Extra description keywords: '", ch);
        for (ed = location->extra_descr; ed; ed = ed->next)
        {
            send_to_char (ed->keyword, ch);
            if (ed->next != NULL)
                send_to_char (" ", ch);
        }
        send_to_char ("'.\n\r", ch);
    }

    send_to_char ("Characters:", ch);
    for (rch = location->people; rch; rch = rch->next_in_room)
    {
        if (can_see (ch, rch))
        {
            send_to_char (" ", ch);
            one_argument (rch->name, buf);
            send_to_char (buf, ch);
        }
    }

    send_to_char (".\n\rObjects:   ", ch);
    for (obj = location->contents; obj; obj = obj->next_content)
    {
        send_to_char (" ", ch);
        one_argument (obj->name, buf);
        send_to_char (buf, ch);
    }
    send_to_char (".\n\r", ch);

    for (door = 0; door <= 5; door++)
    {
        EXIT_DATA *pexit;

        if ((pexit = location->exit[door]) != NULL)
        {
            sprintf (buf,
                     "Door: %d.  To: %d.  Key: %d.  Exit flags: %d.\n\rKeyword: '%s'.  Description: %s",
                     door,
                     (pexit->u1.to_room ==
                      NULL ? -1 : pexit->u1.to_room->vnum), pexit->key,
                     pexit->exit_info, pexit->keyword,
                     pexit->description[0] !=
                     '\0' ? pexit->description : "(none).\n\r");
            send_to_char (buf, ch);
        }
    }

    return;
}



void do_ostat (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    AFFECT_DATA *paf;
    OBJ_DATA *obj;

    one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char ("Stat what?\n\r", ch);
        return;
    }

    if ((obj = get_obj_world (ch, argument)) == NULL)
    {
        send_to_char ("Nothing like that in hell, earth, or heaven.\n\r", ch);
        return;
    }

    sprintf (buf, "Name(s): %s\n\r", obj->name);
    send_to_char (buf, ch);

    sprintf (buf, "Vnum: %d  Format: %s  Type: %s  Resets: %d\n\r",
             obj->pIndexData->vnum,
             obj->pIndexData->new_format ? "new" : "old",
             item_name (obj->item_type), obj->pIndexData->reset_num);
    send_to_char (buf, ch);

    sprintf (buf, "Short description: %s\n\rLong description: %s\n\r",
             obj->short_descr, obj->description);
    send_to_char (buf, ch);

    sprintf (buf, "Wear bits: %s\n\rExtra bits: %s\n\r",
             wear_bit_name (obj->wear_flags),
             extra_bit_name (obj->extra_flags));
    send_to_char (buf, ch);

    sprintf (buf, "Number: %d/%d  Weight: %d/%d/%d (10th pounds)\n\r",
             1, get_obj_number (obj),
             obj->weight, get_obj_weight (obj), get_true_weight (obj));
    send_to_char (buf, ch);

    sprintf (buf, "Level: %d  Cost: %d  Condition: %d  Timer: %d\n\r",
             obj->level, obj->cost, obj->condition, obj->timer);
    send_to_char (buf, ch);

    sprintf (buf,
             "In room: %d  In object: %s  Carried by: %s  Wear_loc: %d\n\r",
             obj->in_room == NULL ? 0 : obj->in_room->vnum,
             obj->in_obj == NULL ? "(none)" : obj->in_obj->short_descr,
             obj->carried_by == NULL ? "(none)" :
             can_see (ch, obj->carried_by) ? obj->carried_by->name
             : "someone", obj->wear_loc);
    send_to_char (buf, ch);

    sprintf (buf, "Values: %d %d %d %d %d\n\r",
             obj->value[0], obj->value[1], obj->value[2], obj->value[3],
             obj->value[4]);
    send_to_char (buf, ch);

    /* now give out vital statistics as per identify */

    switch (obj->item_type)
    {
        case ITEM_SCROLL:
        case ITEM_POTION:
        case ITEM_PILL:
            sprintf (buf, "Level %d spells of:", obj->value[0]);
            send_to_char (buf, ch);

            if (obj->value[1] >= 0 && obj->value[1] < MAX_SKILL)
            {
                send_to_char (" '", ch);
                send_to_char (skill_table[obj->value[1]].name, ch);
                send_to_char ("'", ch);
            }

            if (obj->value[2] >= 0 && obj->value[2] < MAX_SKILL)
            {
                send_to_char (" '", ch);
                send_to_char (skill_table[obj->value[2]].name, ch);
                send_to_char ("'", ch);
            }

            if (obj->value[3] >= 0 && obj->value[3] < MAX_SKILL)
            {
                send_to_char (" '", ch);
                send_to_char (skill_table[obj->value[3]].name, ch);
                send_to_char ("'", ch);
            }

            if (obj->value[4] >= 0 && obj->value[4] < MAX_SKILL)
            {
                send_to_char (" '", ch);
                send_to_char (skill_table[obj->value[4]].name, ch);
                send_to_char ("'", ch);
            }

            send_to_char (".\n\r", ch);
            break;

        case ITEM_WAND:
        case ITEM_STAFF:
            sprintf (buf, "Has %d(%d) charges of level %d",
                     obj->value[1], obj->value[2], obj->value[0]);
            send_to_char (buf, ch);

            if (obj->value[3] >= 0 && obj->value[3] < MAX_SKILL)
            {
                send_to_char (" '", ch);
                send_to_char (skill_table[obj->value[3]].name, ch);
                send_to_char ("'", ch);
            }

            send_to_char (".\n\r", ch);
            break;

        case ITEM_DRINK_CON:
            sprintf (buf, "It holds %s-colored %s.\n\r",
                     liq_table[obj->value[2]].liq_color,
                     liq_table[obj->value[2]].liq_name);
            send_to_char (buf, ch);
            break;


        case ITEM_WEAPON:
            send_to_char ("Weapon type is ", ch);
            switch (obj->value[0])
            {
                case (WEAPON_EXOTIC):
                    send_to_char ("exotic\n\r", ch);
                    break;
                case (WEAPON_SWORD):
                    send_to_char ("sword\n\r", ch);
                    break;
                case (WEAPON_DAGGER):
                    send_to_char ("dagger\n\r", ch);
                    break;
                case (WEAPON_SPEAR):
                    send_to_char ("spear/staff\n\r", ch);
                    break;
                case (WEAPON_MACE):
                    send_to_char ("mace/club\n\r", ch);
                    break;
                case (WEAPON_AXE):
                    send_to_char ("axe\n\r", ch);
                    break;
                case (WEAPON_FLAIL):
                    send_to_char ("flail\n\r", ch);
                    break;
                case (WEAPON_WHIP):
                    send_to_char ("whip\n\r", ch);
                    break;
                case (WEAPON_POLEARM):
                    send_to_char ("polearm\n\r", ch);
                    break;
                default:
                    send_to_char ("unknown\n\r", ch);
                    break;
            }
            if (obj->pIndexData->new_format)
                sprintf (buf, "Damage is %dd%d (average %d)\n\r",
                         obj->value[1], obj->value[2],
                         (1 + obj->value[2]) * obj->value[1] / 2);
            else
                sprintf (buf, "Damage is %d to %d (average %d)\n\r",
                         obj->value[1], obj->value[2],
                         (obj->value[1] + obj->value[2]) / 2);
            send_to_char (buf, ch);

            sprintf (buf, "Damage noun is %s.\n\r",
                     (obj->value[3] > 0
                      && obj->value[3] <
                      MAX_DAMAGE_MESSAGE) ? attack_table[obj->value[3]].noun :
                     "undefined");
            send_to_char (buf, ch);

            if (obj->value[4])
            {                    /* weapon flags */
                sprintf (buf, "Weapons flags: %s\n\r",
                         weapon_bit_name (obj->value[4]));
                send_to_char (buf, ch);
            }
            break;

        case ITEM_ARMOR:
            sprintf (buf,
                     "Armor class is %d pierce, %d bash, %d slash, and %d vs. magic\n\r",
                     obj->value[0], obj->value[1], obj->value[2],
                     obj->value[3]);
            send_to_char (buf, ch);
            break;

        case ITEM_CONTAINER:
            sprintf (buf, "Capacity: %d#  Maximum weight: %d#  flags: %s\n\r",
                     obj->value[0], obj->value[3],
                     cont_bit_name (obj->value[1]));
            send_to_char (buf, ch);
            if (obj->value[4] != 100)
            {
                sprintf (buf, "Weight multiplier: %d%%\n\r", obj->value[4]);
                send_to_char (buf, ch);
            }
            break;
    }


    if (obj->extra_descr != NULL || obj->pIndexData->extra_descr != NULL)
    {
        EXTRA_DESCR_DATA *ed;

        send_to_char ("Extra description keywords: '", ch);

        for (ed = obj->extra_descr; ed != NULL; ed = ed->next)
        {
            send_to_char (ed->keyword, ch);
            if (ed->next != NULL)
                send_to_char (" ", ch);
        }

        for (ed = obj->pIndexData->extra_descr; ed != NULL; ed = ed->next)
        {
            send_to_char (ed->keyword, ch);
            if (ed->next != NULL)
                send_to_char (" ", ch);
        }

        send_to_char ("'\n\r", ch);
    }

    for (paf = obj->affected; paf != NULL; paf = paf->next)
    {
        sprintf (buf, "Affects %s by %d, level %d",
                 affect_loc_name (paf->location), paf->modifier, paf->level);
        send_to_char (buf, ch);
        if (paf->duration > -1)
            sprintf (buf, ", %d hours.\n\r", paf->duration);
        else
            sprintf (buf, ".\n\r");
        send_to_char (buf, ch);
        if (paf->bitvector)
        {
            switch (paf->where)
            {
                case TO_AFFECTS:
                    sprintf (buf, "Adds %s affect.\n",
                             affect_bit_name (paf->bitvector));
                    break;
                case TO_WEAPON:
                    sprintf (buf, "Adds %s weapon flags.\n",
                             weapon_bit_name (paf->bitvector));
                    break;
                case TO_OBJECT:
                    sprintf (buf, "Adds %s object flag.\n",
                             extra_bit_name (paf->bitvector));
                    break;
                case TO_IMMUNE:
                    sprintf (buf, "Adds immunity to %s.\n",
                             imm_bit_name (paf->bitvector));
                    break;
                case TO_RESIST:
                    sprintf (buf, "Adds resistance to %s.\n\r",
                             imm_bit_name (paf->bitvector));
                    break;
                case TO_VULN:
                    sprintf (buf, "Adds vulnerability to %s.\n\r",
                             imm_bit_name (paf->bitvector));
                    break;
                default:
                    sprintf (buf, "Unknown bit %d: %d\n\r",
                             paf->where, paf->bitvector);
                    break;
            }
            send_to_char (buf, ch);
        }
    }

    if (!obj->enchanted)
        for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
        {
            sprintf (buf, "Affects %s by %d, level %d.\n\r",
                     affect_loc_name (paf->location), paf->modifier,
                     paf->level);
            send_to_char (buf, ch);
            if (paf->bitvector)
            {
                switch (paf->where)
                {
                    case TO_AFFECTS:
                        sprintf (buf, "Adds %s affect.\n",
                                 affect_bit_name (paf->bitvector));
                        break;
                    case TO_OBJECT:
                        sprintf (buf, "Adds %s object flag.\n",
                                 extra_bit_name (paf->bitvector));
                        break;
                    case TO_IMMUNE:
                        sprintf (buf, "Adds immunity to %s.\n",
                                 imm_bit_name (paf->bitvector));
                        break;
                    case TO_RESIST:
                        sprintf (buf, "Adds resistance to %s.\n\r",
                                 imm_bit_name (paf->bitvector));
                        break;
                    case TO_VULN:
                        sprintf (buf, "Adds vulnerability to %s.\n\r",
                                 imm_bit_name (paf->bitvector));
                        break;
                    default:
                        sprintf (buf, "Unknown bit %d: %d\n\r",
                                 paf->where, paf->bitvector);
                        break;
                }
                send_to_char (buf, ch);
            }
        }

    return;
}



void do_mstat (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    AFFECT_DATA *paf;
    CHAR_DATA *victim;

    one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char ("Stat whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_world (ch, argument)) == NULL)
    {
        send_to_char ("They aren't here.\n\r", ch);
        return;
    }

    sprintf (buf, "Name: %s\n\r", victim->name);
    send_to_char (buf, ch);

    sprintf (buf,
             "Vnum: %d  Format: %s  Race: %s  Group: %d  Sex: %s  Room: %d\n\r",
             IS_NPC (victim) ? victim->pIndexData->vnum : 0,
             IS_NPC (victim) ? victim->
             pIndexData->new_format ? "new" : "old" : "pc",
             race_table[victim->race].name,
             IS_NPC (victim) ? victim->group : 0, sex_table[victim->sex].name,
             victim->in_room == NULL ? 0 : victim->in_room->vnum);
    send_to_char (buf, ch);

    if (IS_NPC (victim))
    {
        sprintf (buf, "Count: %d  Killed: %d\n\r",
                 victim->pIndexData->count, victim->pIndexData->killed);
        send_to_char (buf, ch);
    }

    sprintf (buf,
             "Str: %d(%d)  Int: %d(%d)  Wis: %d(%d)  Dex: %d(%d)  Con: %d(%d)\n\r",
             victim->perm_stat[STAT_STR],
             get_curr_stat (victim, STAT_STR),
             victim->perm_stat[STAT_INT],
             get_curr_stat (victim, STAT_INT),
             victim->perm_stat[STAT_WIS],
             get_curr_stat (victim, STAT_WIS),
             victim->perm_stat[STAT_DEX],
             get_curr_stat (victim, STAT_DEX),
             victim->perm_stat[STAT_CON], get_curr_stat (victim, STAT_CON));
    send_to_char (buf, ch);

    sprintf (buf, "Hp: %d/%d  Mana: %d/%d  Move: %d/%d  Practices: %d\n\r",
             victim->hit, victim->max_hit,
             victim->mana, victim->max_mana,
             victim->move, victim->max_move,
             IS_NPC (ch) ? 0 : victim->practice);
    send_to_char (buf, ch);

    sprintf (buf,
             "Lv: %d  Class: %s  Align: %d  Gold: %ld  Silver: %ld  Exp: %d\n\r",
             victim->level,
             IS_NPC (victim) ? "mobile" : class_table[victim->class].name,
             victim->alignment, victim->gold, victim->silver, victim->exp);
    send_to_char (buf, ch);

    sprintf (buf, "Armor: pierce: %d  bash: %d  slash: %d  magic: %d\n\r",
             GET_AC (victim, AC_PIERCE), GET_AC (victim, AC_BASH),
             GET_AC (victim, AC_SLASH), GET_AC (victim, AC_EXOTIC));
    send_to_char (buf, ch);

    sprintf (buf,
             "Hit: %d  Dam: %d  Saves: %d  Size: %s  Position: %s  Wimpy: %d\n\r",
             GET_HITROLL (victim), GET_DAMROLL (victim), victim->saving_throw,
             size_table[victim->size].name,
             position_table[victim->position].name, victim->wimpy);
    send_to_char (buf, ch);

    if (IS_NPC (victim) && victim->pIndexData->new_format)
    {
        sprintf (buf, "Damage: %dd%d  Message:  %s\n\r",
                 victim->damage[DICE_NUMBER], victim->damage[DICE_TYPE],
                 attack_table[victim->dam_type].noun);
        send_to_char (buf, ch);
    }
    sprintf (buf, "Fighting: %s\n\r",
             victim->fighting ? victim->fighting->name : "(none)");
    send_to_char (buf, ch);

    if (!IS_NPC (victim))
    {
        sprintf (buf,
                 "Thirst: %d  Hunger: %d  Full: %d  Drunk: %d\n\r",
                 victim->pcdata->condition[COND_THIRST],
                 victim->pcdata->condition[COND_HUNGER],
                 victim->pcdata->condition[COND_FULL],
                 victim->pcdata->condition[COND_DRUNK]);
        send_to_char (buf, ch);
    }

    sprintf (buf, "Carry number: %d  Carry weight: %ld\n\r",
             victim->carry_number, get_carry_weight (victim) / 10);
    send_to_char (buf, ch);


    if (!IS_NPC (victim))
    {
        sprintf (buf,
                 "Age: %d  Played: %d  Last Level: %d  Timer: %d\n\r",
                 get_age (victim),
                 (int) (victim->played + current_time - victim->logon) / 3600,
                 victim->pcdata->last_level, victim->timer);
        send_to_char (buf, ch);
    }

    sprintf (buf, "Act: %s\n\r", act_bit_name (victim->act));
    send_to_char (buf, ch);

    if (victim->comm)
    {
        sprintf (buf, "Comm: %s\n\r", comm_bit_name (victim->comm));
        send_to_char (buf, ch);
    }

    if (IS_NPC (victim) && victim->off_flags)
    {
        sprintf (buf, "Offense: %s\n\r", off_bit_name (victim->off_flags));
        send_to_char (buf, ch);
    }

    if (victim->imm_flags)
    {
        sprintf (buf, "Immune: %s\n\r", imm_bit_name (victim->imm_flags));
        send_to_char (buf, ch);
    }

    if (victim->res_flags)
    {
        sprintf (buf, "Resist: %s\n\r", imm_bit_name (victim->res_flags));
        send_to_char (buf, ch);
    }

    if (victim->vuln_flags)
    {
        sprintf (buf, "Vulnerable: %s\n\r",
                 imm_bit_name (victim->vuln_flags));
        send_to_char (buf, ch);
    }

    sprintf (buf, "Form: %s\n\rParts: %s\n\r",
             form_bit_name (victim->form), part_bit_name (victim->parts));
    send_to_char (buf, ch);

    if (victim->affected_by)
    {
        sprintf (buf, "Affected by %s\n\r",
                 affect_bit_name (victim->affected_by));
        send_to_char (buf, ch);
    }

    sprintf (buf, "Master: %s  Leader: %s  Pet: %s\n\r",
             victim->master ? victim->master->name : "(none)",
             victim->leader ? victim->leader->name : "(none)",
             victim->pet ? victim->pet->name : "(none)");
    send_to_char (buf, ch);

    if (!IS_NPC (victim))
    {
        sprintf (buf, "Security: %d.\n\r", victim->pcdata->security);    /* OLC */
        send_to_char (buf, ch);    /* OLC */
    }

    sprintf (buf, "Short description: %s\n\rLong  description: %s",
             victim->short_descr,
             victim->long_descr[0] !=
             '\0' ? victim->long_descr : "(none)\n\r");
    send_to_char (buf, ch);

    if (IS_NPC (victim) && victim->spec_fun != 0)
    {
        sprintf (buf, "Mobile has special procedure %s.\n\r",
                 spec_name (victim->spec_fun));
        send_to_char (buf, ch);
    }

    for (paf = victim->affected; paf != NULL; paf = paf->next)
    {
        sprintf (buf,
                 "Spell: '%s' modifies %s by %d for %d hours with bits %s, level %d.\n\r",
                 skill_table[(int) paf->type].name,
                 affect_loc_name (paf->location),
                 paf->modifier,
                 paf->duration, affect_bit_name (paf->bitvector), paf->level);
        send_to_char (buf, ch);
    }

    return;
}

/* ofind and mfind replaced with vnum, vnum skill also added */

void do_vnum (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char *string;

    string = one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char ("Syntax:\n\r", ch);
        send_to_char ("  vnum obj <name>\n\r", ch);
        send_to_char ("  vnum mob <name>\n\r", ch);
        send_to_char ("  vnum skill <skill or spell>\n\r", ch);
        return;
    }

    if (!str_cmp (arg, "obj"))
    {
        do_function (ch, &do_ofind, string);
        return;
    }

    if (!str_cmp (arg, "mob") || !str_cmp (arg, "char"))
    {
        do_function (ch, &do_mfind, string);
        return;
    }

    if (!str_cmp (arg, "skill") || !str_cmp (arg, "spell"))
    {
        do_function (ch, &do_slookup, string);
        return;
    }
    /* do both */
    do_function (ch, &do_mfind, argument);
    do_function (ch, &do_ofind, argument);
}


void do_mfind (CHAR_DATA * ch, char *argument)
{
    extern int top_mob_index;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    MOB_INDEX_DATA *pMobIndex;
    int vnum;
    int nMatch;
    bool fAll;
    bool found;

    one_argument (argument, arg);
    if (arg[0] == '\0')
    {
        send_to_char ("Find whom?\n\r", ch);
        return;
    }

    fAll = FALSE;                /* !str_cmp( arg, "all" ); */
    found = FALSE;
    nMatch = 0;

    /*
     * Yeah, so iterating over all vnum's takes 10,000 loops.
     * Get_mob_index is fast, and I don't feel like threading another link.
     * Do you?
     * -- Furey
     */
    for (vnum = 0; nMatch < top_mob_index; vnum++)
    {
        if ((pMobIndex = get_mob_index (vnum)) != NULL)
        {
            nMatch++;
            if (fAll || is_name (argument, pMobIndex->player_name))
            {
                found = TRUE;
                sprintf (buf, "[%5d] %s\n\r",
                         pMobIndex->vnum, pMobIndex->short_descr);
                send_to_char (buf, ch);
            }
        }
    }

    if (!found)
        send_to_char ("No mobiles by that name.\n\r", ch);

    return;
}



void do_ofind (CHAR_DATA * ch, char *argument)
{
    extern int top_obj_index;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    OBJ_INDEX_DATA *pObjIndex;
    int vnum;
    int nMatch;
    bool fAll;
    bool found;

    one_argument (argument, arg);
    if (arg[0] == '\0')
    {
        send_to_char ("Find what?\n\r", ch);
        return;
    }

    fAll = FALSE;                /* !str_cmp( arg, "all" ); */
    found = FALSE;
    nMatch = 0;

    /*
     * Yeah, so iterating over all vnum's takes 10,000 loops.
     * Get_obj_index is fast, and I don't feel like threading another link.
     * Do you?
     * -- Furey
     */
    for (vnum = 0; nMatch < top_obj_index; vnum++)
    {
        if ((pObjIndex = get_obj_index (vnum)) != NULL)
        {
            nMatch++;
            if (fAll || is_name (argument, pObjIndex->name))
            {
                found = TRUE;
                sprintf (buf, "[%5d] %s\n\r",
                         pObjIndex->vnum, pObjIndex->short_descr);
                send_to_char (buf, ch);
            }
        }
    }

    if (!found)
        send_to_char ("No objects by that name.\n\r", ch);

    return;
}


void do_owhere (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_INPUT_LENGTH];
    BUFFER *buffer;
    OBJ_DATA *obj;
    OBJ_DATA *in_obj;
    bool found;
    int number = 0, max_found;

    found = FALSE;
    number = 0;
    max_found = 200;

    buffer = new_buf ();

    if (argument[0] == '\0')
    {
        send_to_char ("Find what?\n\r", ch);
        return;
    }

    for (obj = object_list; obj != NULL; obj = obj->next)
    {
        if (!can_see_obj (ch, obj) || !is_name (argument, obj->name)
            || ch->level < obj->level)
            continue;

        found = TRUE;
        number++;

        for (in_obj = obj; in_obj->in_obj != NULL; in_obj = in_obj->in_obj);

        if (in_obj->carried_by != NULL && can_see (ch, in_obj->carried_by)
            && in_obj->carried_by->in_room != NULL)
            sprintf (buf, "%3d) %s is carried by %s [Room %d]\n\r",
                     number, obj->short_descr, PERS (in_obj->carried_by, ch),
                     in_obj->carried_by->in_room->vnum);
        else if (in_obj->in_room != NULL
                 && can_see_room (ch, in_obj->in_room)) sprintf (buf,
                                                                 "%3d) %s is in %s [Room %d]\n\r",
                                                                 number,
                                                                 obj->short_descr,
                                                                 in_obj->in_room->name,
                                                                 in_obj->in_room->vnum);
        else
            sprintf (buf, "%3d) %s is somewhere\n\r", number,
                     obj->short_descr);

        buf[0] = UPPER (buf[0]);
        add_buf (buffer, buf);

        if (number >= max_found)
            break;
    }

    if (!found)
        send_to_char ("Nothing like that in heaven or earth.\n\r", ch);
    else
        page_to_char (buf_string (buffer), ch);

    free_buf (buffer);
}


void do_mwhere (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    BUFFER *buffer;
    CHAR_DATA *victim;
    bool found;
    int count = 0;

    if (argument[0] == '\0')
    {
        DESCRIPTOR_DATA *d;

        /* show characters logged */

        buffer = new_buf ();
        for (d = descriptor_list; d != NULL; d = d->next)
        {
            if (d->character != NULL && d->connected == CON_PLAYING
                && d->character->in_room != NULL && can_see (ch, d->character)
                && can_see_room (ch, d->character->in_room))
            {
                victim = d->character;
                count++;
                if (d->original != NULL)
                    sprintf (buf,
                             "%3d) %s (in the body of %s) is in %s [%d]\n\r",
                             count, d->original->name, victim->short_descr,
                             victim->in_room->name, victim->in_room->vnum);
                else
                    sprintf (buf, "%3d) %s is in %s [%d]\n\r", count,
                             victim->name, victim->in_room->name,
                             victim->in_room->vnum);
                add_buf (buffer, buf);
            }
        }

        page_to_char (buf_string (buffer), ch);
        free_buf (buffer);
        return;
    }

    found = FALSE;
    buffer = new_buf ();
    for (victim = char_list; victim != NULL; victim = victim->next)
    {
        if (victim->in_room != NULL && is_name (argument, victim->name))
        {
            found = TRUE;
            count++;
            sprintf (buf, "%3d) [%5d] %-28s [%5d] %s\n\r", count,
                     IS_NPC (victim) ? victim->pIndexData->vnum : 0,
                     IS_NPC (victim) ? victim->short_descr : victim->name,
                     victim->in_room->vnum, victim->in_room->name);
            add_buf (buffer, buf);
        }
    }

    if (!found)
        act ("You didn't find any $T.", ch, NULL, argument, TO_CHAR);
    else
        page_to_char (buf_string (buffer), ch);

    free_buf (buffer);

    return;
}



void do_reboo (CHAR_DATA * ch, char *argument)
{
    send_to_char ("If you want to REBOOT, spell it out.\n\r", ch);
    return;
}



void do_reboot (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    extern bool merc_down;
    DESCRIPTOR_DATA *d, *d_next;
    CHAR_DATA *vch;

    if (ch->invis_level < LEVEL_HERO)
    {
        sprintf (buf, "Reboot by %s.", ch->name);
        do_function (ch, &do_echo, buf);
    }

    merc_down = TRUE;
    for (d = descriptor_list; d != NULL; d = d_next)
    {
        d_next = d->next;
        vch = d->original ? d->original : d->character;
        if (vch != NULL)
            save_char_obj (vch);
        close_socket (d);
    }

    return;
}

void do_shutdow (CHAR_DATA * ch, char *argument)
{
    send_to_char ("If you want to SHUTDOWN, spell it out.\n\r", ch);
    return;
}

void do_shutdown (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    extern bool merc_down;
    DESCRIPTOR_DATA *d, *d_next;
    CHAR_DATA *vch;

    if (ch->invis_level < LEVEL_HERO)
        sprintf (buf, "Shutdown by %s.", ch->name);
    append_file (ch, SHUTDOWN_FILE, buf);
    strcat (buf, "\n\r");
    if (ch->invis_level < LEVEL_HERO)
    {
        do_function (ch, &do_echo, buf);
    }
    merc_down = TRUE;
    for (d = descriptor_list; d != NULL; d = d_next)
    {
        d_next = d->next;
        vch = d->original ? d->original : d->character;
        if (vch != NULL)
            save_char_obj (vch);
        close_socket (d);
    }
    return;
}

void do_protect (CHAR_DATA * ch, char *argument)
{
    CHAR_DATA *victim;

    if (argument[0] == '\0')
    {
        send_to_char ("Protect whom from snooping?\n\r", ch);
        return;
    }

    if ((victim = get_char_world (ch, argument)) == NULL)
    {
        send_to_char ("You can't find them.\n\r", ch);
        return;
    }

    if (IS_SET (victim->comm, COMM_SNOOP_PROOF))
    {
        act_new ("$N is no longer snoop-proof.", ch, NULL, victim, TO_CHAR,
                 POS_DEAD);
        send_to_char ("Your snoop-proofing was just removed.\n\r", victim);
        REMOVE_BIT (victim->comm, COMM_SNOOP_PROOF);
    }
    else
    {
        act_new ("$N is now snoop-proof.", ch, NULL, victim, TO_CHAR,
                 POS_DEAD);
        send_to_char ("You are now immune to snooping.\n\r", victim);
        SET_BIT (victim->comm, COMM_SNOOP_PROOF);
    }
}



void do_snoop (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    DESCRIPTOR_DATA *d;
    CHAR_DATA *victim;
    char buf[MAX_STRING_LENGTH];

    one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char ("Snoop whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_world (ch, arg)) == NULL)
    {
        send_to_char ("They aren't here.\n\r", ch);
        return;
    }

    if (victim->desc == NULL)
    {
        send_to_char ("No descriptor to snoop.\n\r", ch);
        return;
    }

    if (victim == ch)
    {
        send_to_char ("Cancelling all snoops.\n\r", ch);
        wiznet ("$N stops being such a snoop.",
                ch, NULL, WIZ_SNOOPS, WIZ_SECURE, get_trust (ch));
        for (d = descriptor_list; d != NULL; d = d->next)
        {
            if (d->snoop_by == ch->desc)
                d->snoop_by = NULL;
        }
        return;
    }

    if (victim->desc->snoop_by != NULL)
    {
        send_to_char ("Busy already.\n\r", ch);
        return;
    }

    if (!is_room_owner (ch, victim->in_room) && ch->in_room != victim->in_room
        && room_is_private (victim->in_room) && !IS_TRUSTED (ch, IMPLEMENTOR))
    {
        send_to_char ("That character is in a private room.\n\r", ch);
        return;
    }

    if (get_trust (victim) >= get_trust (ch)
        || IS_SET (victim->comm, COMM_SNOOP_PROOF))
    {
        send_to_char ("You failed.\n\r", ch);
        return;
    }

    if (ch->desc != NULL)
    {
        for (d = ch->desc->snoop_by; d != NULL; d = d->snoop_by)
        {
            if (d->character == victim || d->original == victim)
            {
                send_to_char ("No snoop loops.\n\r", ch);
                return;
            }
        }
    }

    victim->desc->snoop_by = ch->desc;
    sprintf (buf, "$N starts snooping on %s",
             (IS_NPC (ch) ? victim->short_descr : victim->name));
    wiznet (buf, ch, NULL, WIZ_SNOOPS, WIZ_SECURE, get_trust (ch));
    send_to_char ("Ok.\n\r", ch);
    return;
}



void do_switch (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char ("Switch into whom?\n\r", ch);
        return;
    }

    if (ch->desc == NULL)
        return;

    if (ch->desc->original != NULL)
    {
        send_to_char ("You are already switched.\n\r", ch);
        return;
    }

    if ((victim = get_char_world (ch, arg)) == NULL)
    {
        send_to_char ("They aren't here.\n\r", ch);
        return;
    }

    if (victim == ch)
    {
        send_to_char ("Ok.\n\r", ch);
        return;
    }

    if (!IS_NPC (victim))
    {
        send_to_char ("You can only switch into mobiles.\n\r", ch);
        return;
    }

    if (!is_room_owner (ch, victim->in_room) && ch->in_room != victim->in_room
        && room_is_private (victim->in_room) && !IS_TRUSTED (ch, IMPLEMENTOR))
    {
        send_to_char ("That character is in a private room.\n\r", ch);
        return;
    }

    if (victim->desc != NULL)
    {
        send_to_char ("Character in use.\n\r", ch);
        return;
    }

    sprintf (buf, "$N switches into %s", victim->short_descr);
    wiznet (buf, ch, NULL, WIZ_SWITCHES, WIZ_SECURE, get_trust (ch));

    ch->desc->character = victim;
    ch->desc->original = ch;
    victim->desc = ch->desc;
    ch->desc = NULL;
    /* change communications to match */
    if (ch->prompt != NULL)
        victim->prompt = str_dup (ch->prompt);
    victim->comm = ch->comm;
    victim->lines = ch->lines;
    send_to_char ("Ok.\n\r", victim);
    return;
}



void do_return (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];

    if (ch->desc == NULL)
        return;

    if (ch->desc->original == NULL)
    {
        send_to_char ("You aren't switched.\n\r", ch);
        return;
    }

    send_to_char
        ("You return to your original body. Type replay to see any missed tells.\n\r",
         ch);
    if (ch->prompt != NULL)
    {
        free_string (ch->prompt);
        ch->prompt = NULL;
    }

    sprintf (buf, "$N returns from %s.", ch->short_descr);
    wiznet (buf, ch->desc->original, 0, WIZ_SWITCHES, WIZ_SECURE,
            get_trust (ch));
    ch->desc->character = ch->desc->original;
    ch->desc->original = NULL;
    ch->desc->character->desc = ch->desc;
    ch->desc = NULL;
    return;
}

/* trust levels for load and clone */
bool obj_check (CHAR_DATA * ch, OBJ_DATA * obj)
{
    if (IS_TRUSTED (ch, GOD)
        || (IS_TRUSTED (ch, IMMORTAL) && obj->level <= 20
            && obj->cost <= 1000) || (IS_TRUSTED (ch, DEMI)
                                      && obj->level <= 10 && obj->cost <= 500)
        || (IS_TRUSTED (ch, ANGEL) && obj->level <= 5 && obj->cost <= 250)
        || (IS_TRUSTED (ch, AVATAR) && obj->level == 0 && obj->cost <= 100))
        return TRUE;
    else
        return FALSE;
}

/* for clone, to insure that cloning goes many levels deep */
void recursive_clone (CHAR_DATA * ch, OBJ_DATA * obj, OBJ_DATA * clone)
{
    OBJ_DATA *c_obj, *t_obj;


    for (c_obj = obj->contains; c_obj != NULL; c_obj = c_obj->next_content)
    {
        if (obj_check (ch, c_obj))
        {
            t_obj = create_object (c_obj->pIndexData, 0);
            clone_object (c_obj, t_obj);
            obj_to_obj (t_obj, clone);
            recursive_clone (ch, c_obj, t_obj);
        }
    }
}

/* command that is similar to load */
void do_clone (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char *rest;
    CHAR_DATA *mob;
    OBJ_DATA *obj;

    rest = one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char ("Clone what?\n\r", ch);
        return;
    }

    if (!str_prefix (arg, "object"))
    {
        mob = NULL;
        obj = get_obj_here (ch, rest);
        if (obj == NULL)
        {
            send_to_char ("You don't see that here.\n\r", ch);
            return;
        }
    }
    else if (!str_prefix (arg, "mobile") || !str_prefix (arg, "character"))
    {
        obj = NULL;
        mob = get_char_room (ch, rest);
        if (mob == NULL)
        {
            send_to_char ("You don't see that here.\n\r", ch);
            return;
        }
    }
    else
    {                            /* find both */

        mob = get_char_room (ch, argument);
        obj = get_obj_here (ch, argument);
        if (mob == NULL && obj == NULL)
        {
            send_to_char ("You don't see that here.\n\r", ch);
            return;
        }
    }

    /* clone an object */
    if (obj != NULL)
    {
        OBJ_DATA *clone;

        if (!obj_check (ch, obj))
        {
            send_to_char
                ("Your powers are not great enough for such a task.\n\r", ch);
            return;
        }

        clone = create_object (obj->pIndexData, 0);
        clone_object (obj, clone);
        if (obj->carried_by != NULL)
            obj_to_char (clone, ch);
        else
            obj_to_room (clone, ch->in_room);
        recursive_clone (ch, obj, clone);

        act ("$n has created $p.", ch, clone, NULL, TO_ROOM);
        act ("You clone $p.", ch, clone, NULL, TO_CHAR);
        wiznet ("$N clones $p.", ch, clone, WIZ_LOAD, WIZ_SECURE,
                get_trust (ch));
        return;
    }
    else if (mob != NULL)
    {
        CHAR_DATA *clone;
        OBJ_DATA *new_obj;
        char buf[MAX_STRING_LENGTH];

        if (!IS_NPC (mob))
        {
            send_to_char ("You can only clone mobiles.\n\r", ch);
            return;
        }

        if ((mob->level > 20 && !IS_TRUSTED (ch, GOD))
            || (mob->level > 10 && !IS_TRUSTED (ch, IMMORTAL))
            || (mob->level > 5 && !IS_TRUSTED (ch, DEMI))
            || (mob->level > 0 && !IS_TRUSTED (ch, ANGEL))
            || !IS_TRUSTED (ch, AVATAR))
        {
            send_to_char
                ("Your powers are not great enough for such a task.\n\r", ch);
            return;
        }

        clone = create_mobile (mob->pIndexData);
        clone_mobile (mob, clone);

        for (obj = mob->carrying; obj != NULL; obj = obj->next_content)
        {
            if (obj_check (ch, obj))
            {
                new_obj = create_object (obj->pIndexData, 0);
                clone_object (obj, new_obj);
                recursive_clone (ch, obj, new_obj);
                obj_to_char (new_obj, clone);
                new_obj->wear_loc = obj->wear_loc;
            }
        }
        char_to_room (clone, ch->in_room);
        act ("$n has created $N.", ch, NULL, clone, TO_ROOM);
        act ("You clone $N.", ch, NULL, clone, TO_CHAR);
        sprintf (buf, "$N clones %s.", clone->short_descr);
        wiznet (buf, ch, NULL, WIZ_LOAD, WIZ_SECURE, get_trust (ch));
        return;
    }
}

/* RT to replace the two load commands */

void do_load (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];

    argument = one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char ("Syntax:\n\r", ch);
        send_to_char ("  load mob <vnum>\n\r", ch);
        send_to_char ("  load obj <vnum> <level>\n\r", ch);
        return;
    }

    if (!str_cmp (arg, "mob") || !str_cmp (arg, "char"))
    {
        do_function (ch, &do_mload, argument);
        return;
    }

    if (!str_cmp (arg, "obj"))
    {
        do_function (ch, &do_oload, argument);
        return;
    }
    /* echo syntax */
    do_function (ch, &do_load, "");
}


void do_mload (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    MOB_INDEX_DATA *pMobIndex;
    CHAR_DATA *victim;
    char buf[MAX_STRING_LENGTH];

    one_argument (argument, arg);

    if (arg[0] == '\0' || !is_number (arg))
    {
        send_to_char ("Syntax: load mob <vnum>.\n\r", ch);
        return;
    }

    if ((pMobIndex = get_mob_index (atoi (arg))) == NULL)
    {
        send_to_char ("No mob has that vnum.\n\r", ch);
        return;
    }

    victim = create_mobile (pMobIndex);
    char_to_room (victim, ch->in_room);
    act ("$n has created $N!", ch, NULL, victim, TO_ROOM);
    sprintf (buf, "$N loads %s.", victim->short_descr);
    wiznet (buf, ch, NULL, WIZ_LOAD, WIZ_SECURE, get_trust (ch));
    send_to_char ("Ok.\n\r", ch);
    return;
}



void do_oload (CHAR_DATA * ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    OBJ_INDEX_DATA *pObjIndex;
    OBJ_DATA *obj;
    int level;

    argument = one_argument (argument, arg1);
    one_argument (argument, arg2);

    if (arg1[0] == '\0' || !is_number (arg1))
    {
        send_to_char ("Syntax: load obj <vnum> <level>.\n\r", ch);
        return;
    }

    level = get_trust (ch);        /* default */

    if (arg2[0] != '\0')
    {                            /* load with a level */
        if (!is_number (arg2))
        {
            send_to_char ("Syntax: oload <vnum> <level>.\n\r", ch);
            return;
        }
        level = atoi (arg2);
        if (level < 0 || level > get_trust (ch))
        {
            send_to_char ("Level must be be between 0 and your level.\n\r",
                          ch);
            return;
        }
    }

    if ((pObjIndex = get_obj_index (atoi (arg1))) == NULL)
    {
        send_to_char ("No object has that vnum.\n\r", ch);
        return;
    }

    obj = create_object (pObjIndex, level);
    if (CAN_WEAR (obj, ITEM_TAKE))
        obj_to_char (obj, ch);
    else
        obj_to_room (obj, ch->in_room);
    act ("$n has created $p!", ch, obj, NULL, TO_ROOM);
    wiznet ("$N loads $p.", ch, obj, WIZ_LOAD, WIZ_SECURE, get_trust (ch));
    send_to_char ("Ok.\n\r", ch);
    return;
}



void do_purge (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[100];
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    DESCRIPTOR_DATA *d;

    one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        /* 'purge' */
        CHAR_DATA *vnext;
        OBJ_DATA *obj_next;

        for (victim = ch->in_room->people; victim != NULL; victim = vnext)
        {
            vnext = victim->next_in_room;
            if (IS_NPC (victim) && !IS_SET (victim->act, ACT_NOPURGE)
                && victim != ch /* safety precaution */ )
                extract_char (victim, TRUE);
        }

        for (obj = ch->in_room->contents; obj != NULL; obj = obj_next)
        {
            obj_next = obj->next_content;
            if (!IS_OBJ_STAT (obj, ITEM_NOPURGE))
                extract_obj (obj);
        }

        act ("$n purges the room!", ch, NULL, NULL, TO_ROOM);
        send_to_char ("Ok.\n\r", ch);
        return;
    }

    if ((victim = get_char_world (ch, arg)) == NULL)
    {
        send_to_char ("They aren't here.\n\r", ch);
        return;
    }

    if (!IS_NPC (victim))
    {

        if (ch == victim)
        {
            send_to_char ("Ho ho ho.\n\r", ch);
            return;
        }

        if (get_trust (ch) <= get_trust (victim))
        {
            send_to_char ("Maybe that wasn't a good idea...\n\r", ch);
            sprintf (buf, "%s tried to purge you!\n\r", ch->name);
            send_to_char (buf, victim);
            return;
        }

        act ("$n disintegrates $N.", ch, 0, victim, TO_NOTVICT);

        if (victim->level > 1)
            save_char_obj (victim);
        d = victim->desc;
        extract_char (victim, TRUE);
        if (d != NULL)
            close_socket (d);

        return;
    }

    act ("$n purges $N.", ch, NULL, victim, TO_NOTVICT);
    extract_char (victim, TRUE);
    return;
}



void do_advance (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int level;
    int iLevel;

    argument = one_argument (argument, arg1);
    argument = one_argument (argument, arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0' || !is_number (arg2))
    {
        send_to_char ("Syntax: advance <char> <level>.\n\r", ch);
        return;
    }

    if ((victim = get_char_world (ch, arg1)) == NULL)
    {
        send_to_char ("That player is not here.\n\r", ch);
        return;
    }

    if (IS_NPC (victim))
    {
        send_to_char ("Not on NPC's.\n\r", ch);
        return;
    }

    if ((level = atoi (arg2)) < 1 || level > MAX_LEVEL)
    {
        sprintf (buf, "Level must be 1 to %d.\n\r", MAX_LEVEL);
        send_to_char (buf, ch);
        return;
    }

    if (level > get_trust (ch))
    {
        send_to_char ("Limited to your trust level.\n\r", ch);
        return;
    }

    /*
     * Lower level:
     *   Reset to level 1.
     *   Then raise again.
     *   Currently, an imp can lower another imp.
     *   -- Swiftest
     */
    if (level <= victim->level)
    {
        int temp_prac;

        send_to_char ("Lowering a player's level!\n\r", ch);
        send_to_char ("**** OOOOHHHHHHHHHH  NNNNOOOO ****\n\r", victim);
        temp_prac = victim->practice;
        victim->level = 1;
        victim->exp = exp_per_level (victim, victim->pcdata->points);
        victim->max_hit = 10;
        victim->max_mana = 100;
        victim->max_move = 100;
        victim->practice = 0;
        victim->hit = victim->max_hit;
        victim->mana = victim->max_mana;
        victim->move = victim->max_move;
        advance_level (victim, TRUE);
        victim->practice = temp_prac;
    }
    else
    {
        send_to_char ("Raising a player's level!\n\r", ch);
        send_to_char ("**** OOOOHHHHHHHHHH  YYYYEEEESSS ****\n\r", victim);
    }

    for (iLevel = victim->level; iLevel < level; iLevel++)
    {
        victim->level += 1;
        advance_level (victim, TRUE);
    }
    sprintf (buf, "You are now level %d.\n\r", victim->level);
    send_to_char (buf, victim);
    victim->exp = exp_per_level (victim, victim->pcdata->points)
        * UMAX (1, victim->level);
    victim->trust = 0;
    save_char_obj (victim);
    return;
}



void do_trust (CHAR_DATA * ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    int level;

    argument = one_argument (argument, arg1);
    argument = one_argument (argument, arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0' || !is_number (arg2))
    {
        send_to_char ("Syntax: trust <char> <level>.\n\r", ch);
        return;
    }

    if ((victim = get_char_world (ch, arg1)) == NULL)
    {
        send_to_char ("That player is not here.\n\r", ch);
        return;
    }

    if ((level = atoi (arg2)) < 0 || level > MAX_LEVEL)
    {
        sprintf (buf, "Level must be 0 (reset) or 1 to %d.\n\r", MAX_LEVEL);
        send_to_char (buf, ch);
        return;
    }

    if (level > get_trust (ch))
    {
        send_to_char ("Limited to your trust.\n\r", ch);
        return;
    }

    victim->trust = level;
    return;
}



void do_restore (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *vch;
    DESCRIPTOR_DATA *d;

    one_argument (argument, arg);
    if (arg[0] == '\0' || !str_cmp (arg, "room"))
    {
        /* cure room */

        for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
        {
            affect_strip (vch, gsn_plague);
            affect_strip (vch, gsn_poison);
            affect_strip (vch, gsn_blindness);
            affect_strip (vch, gsn_sleep);
            affect_strip (vch, gsn_curse);

            vch->hit = vch->max_hit;
            vch->mana = vch->max_mana;
            vch->move = vch->max_move;
            update_pos (vch);
            act ("$n has restored you.", ch, NULL, vch, TO_VICT);
        }

        sprintf (buf, "$N restored room %d.", ch->in_room->vnum);
        wiznet (buf, ch, NULL, WIZ_RESTORE, WIZ_SECURE, get_trust (ch));

        send_to_char ("Room restored.\n\r", ch);
        return;

    }

    if (get_trust (ch) >= MAX_LEVEL - 1 && !str_cmp (arg, "all"))
    {
        /* cure all */

        for (d = descriptor_list; d != NULL; d = d->next)
        {
            victim = d->character;

            if (victim == NULL || IS_NPC (victim))
                continue;

            affect_strip (victim, gsn_plague);
            affect_strip (victim, gsn_poison);
            affect_strip (victim, gsn_blindness);
            affect_strip (victim, gsn_sleep);
            affect_strip (victim, gsn_curse);

            victim->hit = victim->max_hit;
            victim->mana = victim->max_mana;
            victim->move = victim->max_move;
            update_pos (victim);
            if (victim->in_room != NULL)
                act ("$n has restored you.", ch, NULL, victim, TO_VICT);
        }
        send_to_char ("All active players restored.\n\r", ch);
        return;
    }

    if ((victim = get_char_world (ch, arg)) == NULL)
    {
        send_to_char ("They aren't here.\n\r", ch);
        return;
    }

    affect_strip (victim, gsn_plague);
    affect_strip (victim, gsn_poison);
    affect_strip (victim, gsn_blindness);
    affect_strip (victim, gsn_sleep);
    affect_strip (victim, gsn_curse);
    victim->hit = victim->max_hit;
    victim->mana = victim->max_mana;
    victim->move = victim->max_move;
    update_pos (victim);
    act ("$n has restored you.", ch, NULL, victim, TO_VICT);
    sprintf (buf, "$N restored %s",
             IS_NPC (victim) ? victim->short_descr : victim->name);
    wiznet (buf, ch, NULL, WIZ_RESTORE, WIZ_SECURE, get_trust (ch));
    send_to_char ("Ok.\n\r", ch);
    return;
}


void do_freeze (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char ("Freeze whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_world (ch, arg)) == NULL)
    {
        send_to_char ("They aren't here.\n\r", ch);
        return;
    }

    if (IS_NPC (victim))
    {
        send_to_char ("Not on NPC's.\n\r", ch);
        return;
    }

    if (get_trust (victim) >= get_trust (ch))
    {
        send_to_char ("You failed.\n\r", ch);
        return;
    }

    if (IS_SET (victim->act, PLR_FREEZE))
    {
        REMOVE_BIT (victim->act, PLR_FREEZE);
        send_to_char ("You can play again.\n\r", victim);
        send_to_char ("FREEZE removed.\n\r", ch);
        sprintf (buf, "$N thaws %s.", victim->name);
        wiznet (buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }
    else
    {
        SET_BIT (victim->act, PLR_FREEZE);
        send_to_char ("You can't do ANYthing!\n\r", victim);
        send_to_char ("FREEZE set.\n\r", ch);
        sprintf (buf, "$N puts %s in the deep freeze.", victim->name);
        wiznet (buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }

    save_char_obj (victim);

    return;
}



void do_log (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char ("Log whom?\n\r", ch);
        return;
    }

    if (!str_cmp (arg, "all"))
    {
        if (fLogAll)
        {
            fLogAll = FALSE;
            send_to_char ("Log ALL off.\n\r", ch);
        }
        else
        {
            fLogAll = TRUE;
            send_to_char ("Log ALL on.\n\r", ch);
        }
        return;
    }

    if ((victim = get_char_world (ch, arg)) == NULL)
    {
        send_to_char ("They aren't here.\n\r", ch);
        return;
    }

    if (IS_NPC (victim))
    {
        send_to_char ("Not on NPC's.\n\r", ch);
        return;
    }

    /*
     * No level check, gods can log anyone.
     */
    if (IS_SET (victim->act, PLR_LOG))
    {
        REMOVE_BIT (victim->act, PLR_LOG);
        send_to_char ("LOG removed.\n\r", ch);
    }
    else
    {
        SET_BIT (victim->act, PLR_LOG);
        send_to_char ("LOG set.\n\r", ch);
    }

    return;
}



void do_noemote (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char ("Noemote whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_world (ch, arg)) == NULL)
    {
        send_to_char ("They aren't here.\n\r", ch);
        return;
    }


    if (get_trust (victim) >= get_trust (ch))
    {
        send_to_char ("You failed.\n\r", ch);
        return;
    }

    if (IS_SET (victim->comm, COMM_NOEMOTE))
    {
        REMOVE_BIT (victim->comm, COMM_NOEMOTE);
        send_to_char ("You can emote again.\n\r", victim);
        send_to_char ("NOEMOTE removed.\n\r", ch);
        sprintf (buf, "$N restores emotes to %s.", victim->name);
        wiznet (buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }
    else
    {
        SET_BIT (victim->comm, COMM_NOEMOTE);
        send_to_char ("You can't emote!\n\r", victim);
        send_to_char ("NOEMOTE set.\n\r", ch);
        sprintf (buf, "$N revokes %s's emotes.", victim->name);
        wiznet (buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }

    return;
}



void do_noshout (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char ("Noshout whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_world (ch, arg)) == NULL)
    {
        send_to_char ("They aren't here.\n\r", ch);
        return;
    }

    if (IS_NPC (victim))
    {
        send_to_char ("Not on NPC's.\n\r", ch);
        return;
    }

    if (get_trust (victim) >= get_trust (ch))
    {
        send_to_char ("You failed.\n\r", ch);
        return;
    }

    if (IS_SET (victim->comm, COMM_NOSHOUT))
    {
        REMOVE_BIT (victim->comm, COMM_NOSHOUT);
        send_to_char ("You can shout again.\n\r", victim);
        send_to_char ("NOSHOUT removed.\n\r", ch);
        sprintf (buf, "$N restores shouts to %s.", victim->name);
        wiznet (buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }
    else
    {
        SET_BIT (victim->comm, COMM_NOSHOUT);
        send_to_char ("You can't shout!\n\r", victim);
        send_to_char ("NOSHOUT set.\n\r", ch);
        sprintf (buf, "$N revokes %s's shouts.", victim->name);
        wiznet (buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }

    return;
}



void do_notell (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char ("Notell whom?", ch);
        return;
    }

    if ((victim = get_char_world (ch, arg)) == NULL)
    {
        send_to_char ("They aren't here.\n\r", ch);
        return;
    }

    if (get_trust (victim) >= get_trust (ch))
    {
        send_to_char ("You failed.\n\r", ch);
        return;
    }

    if (IS_SET (victim->comm, COMM_NOTELL))
    {
        REMOVE_BIT (victim->comm, COMM_NOTELL);
        send_to_char ("You can tell again.\n\r", victim);
        send_to_char ("NOTELL removed.\n\r", ch);
        sprintf (buf, "$N restores tells to %s.", victim->name);
        wiznet (buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }
    else
    {
        SET_BIT (victim->comm, COMM_NOTELL);
        send_to_char ("You can't tell!\n\r", victim);
        send_to_char ("NOTELL set.\n\r", ch);
        sprintf (buf, "$N revokes %s's tells.", victim->name);
        wiznet (buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
    }

    return;
}



void do_peace (CHAR_DATA * ch, char *argument)
{
    CHAR_DATA *rch;

    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
    {
        if (rch->fighting != NULL)
            stop_fighting (rch, TRUE);
        if (IS_NPC (rch) && IS_SET (rch->act, ACT_AGGRESSIVE))
            REMOVE_BIT (rch->act, ACT_AGGRESSIVE);
    }

    send_to_char ("Ok.\n\r", ch);
    return;
}

void do_wizlock (CHAR_DATA * ch, char *argument)
{
    extern bool wizlock;
    wizlock = !wizlock;

    if (wizlock)
    {
        wiznet ("$N has wizlocked the game.", ch, NULL, 0, 0, 0);
        send_to_char ("Game wizlocked.\n\r", ch);
    }
    else
    {
        wiznet ("$N removes wizlock.", ch, NULL, 0, 0, 0);
        send_to_char ("Game un-wizlocked.\n\r", ch);
    }

    return;
}

/* RT anti-newbie code */

void do_newlock (CHAR_DATA * ch, char *argument)
{
    extern bool newlock;
    newlock = !newlock;

    if (newlock)
    {
        wiznet ("$N locks out new characters.", ch, NULL, 0, 0, 0);
        send_to_char ("New characters have been locked out.\n\r", ch);
    }
    else
    {
        wiznet ("$N allows new characters back in.", ch, NULL, 0, 0, 0);
        send_to_char ("Newlock removed.\n\r", ch);
    }

    return;
}


void do_slookup (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    int sn;

    one_argument (argument, arg);
    if (arg[0] == '\0')
    {
        send_to_char ("Lookup which skill or spell?\n\r", ch);
        return;
    }

    if (!str_cmp (arg, "all"))
    {
        for (sn = 0; sn < MAX_SKILL; sn++)
        {
            if (skill_table[sn].name == NULL)
                break;
            sprintf (buf, "Sn: %3d  Slot: %3d  Skill/spell: '%s'\n\r",
                     sn, skill_table[sn].slot, skill_table[sn].name);
            send_to_char (buf, ch);
        }
    }
    else
    {
        if ((sn = skill_lookup (arg)) < 0)
        {
            send_to_char ("No such skill or spell.\n\r", ch);
            return;
        }

        sprintf (buf, "Sn: %3d  Slot: %3d  Skill/spell: '%s'\n\r",
                 sn, skill_table[sn].slot, skill_table[sn].name);
        send_to_char (buf, ch);
    }

    return;
}

/* RT set replaces sset, mset, oset, and rset */

void do_set (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];

    argument = one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char ("Syntax:\n\r", ch);
        send_to_char ("  set mob       <name> <field> <value>\n\r", ch);
        send_to_char ("  set character <name> <field> <value>\n\r", ch);
        send_to_char ("  set obj       <name> <field> <value>\n\r", ch);
        send_to_char ("  set room      <room> <field> <value>\n\r", ch);
        send_to_char ("  set skill     <name> <spell or skill> <value>\n\r", ch);
        return;
    }

    if (!str_prefix (arg, "mobile") || !str_prefix (arg, "character"))
    {
        do_function (ch, &do_mset, argument);
        return;
    }

    if (!str_prefix (arg, "skill") || !str_prefix (arg, "spell"))
    {
        do_function (ch, &do_sset, argument);
        return;
    }

    if (!str_prefix (arg, "object"))
    {
        do_function (ch, &do_oset, argument);
        return;
    }

    if (!str_prefix (arg, "room"))
    {
        do_function (ch, &do_rset, argument);
        return;
    }
    /* echo syntax */
    do_function (ch, &do_set, "");
}


void do_sset (CHAR_DATA * ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int value;
    int sn;
    bool fAll;

    argument = one_argument (argument, arg1);
    argument = one_argument (argument, arg2);
    argument = one_argument (argument, arg3);

    if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0')
    {
        send_to_char ("Syntax:\n\r", ch);
        send_to_char ("  set skill <name> <spell or skill> <value>\n\r", ch);
        send_to_char ("  set skill <name> all <value>\n\r", ch);
        send_to_char ("   (use the name of the skill, not the number)\n\r",
                      ch);
        return;
    }

    if ((victim = get_char_world (ch, arg1)) == NULL)
    {
        send_to_char ("They aren't here.\n\r", ch);
        return;
    }

    if (IS_NPC (victim))
    {
        send_to_char ("Not on NPC's.\n\r", ch);
        return;
    }

    fAll = !str_cmp (arg2, "all");
    sn = 0;
    if (!fAll && (sn = skill_lookup (arg2)) < 0)
    {
        send_to_char ("No such skill or spell.\n\r", ch);
        return;
    }

    /*
     * Snarf the value.
     */
    if (!is_number (arg3))
    {
        send_to_char ("Value must be numeric.\n\r", ch);
        return;
    }

    value = atoi (arg3);
    if (value < 0 || value > 100)
    {
        send_to_char ("Value range is 0 to 100.\n\r", ch);
        return;
    }

    if (fAll)
    {
        for (sn = 0; sn < MAX_SKILL; sn++)
        {
            if (skill_table[sn].name != NULL)
                victim->pcdata->learned[sn] = value;
        }
    }
    else
    {
        victim->pcdata->learned[sn] = value;
    }

    return;
}


void do_mset (CHAR_DATA * ch, char *argument)
{
    char arg1[MIL];
    char arg2[MIL];
    char arg3[MIL];
    char buf[100];
    CHAR_DATA *victim;
    int value;

    smash_tilde (argument);
    argument = one_argument (argument, arg1);
    argument = one_argument (argument, arg2);
    strcpy (arg3, argument);

    if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0')
    {
        send_to_char ("Syntax:\n\r", ch);
        send_to_char ("  set char <name> <field> <value>\n\r", ch);
        send_to_char ("  Field being one of:\n\r", ch);
        send_to_char ("    str int wis dex con sex class level\n\r", ch);
        send_to_char ("    race group gold silver hp mana move prac\n\r", ch);
        send_to_char ("    align train thirst hunger drunk full\n\r", ch);
        send_to_char ("    security hours\n\r", ch);
        return;
    }

    if ((victim = get_char_world (ch, arg1)) == NULL)
    {
        send_to_char ("They aren't here.\n\r", ch);
        return;
    }

    /* clear zones for mobs */
    victim->zone = NULL;

    /*
     * Snarf the value (which need not be numeric).
     */
    value = is_number (arg3) ? atoi (arg3) : -1;

    /*
     * Set something.
     */
    if (!str_cmp (arg2, "str"))
    {
        if (value < 3 || value > get_max_train (victim, STAT_STR))
        {
            sprintf (buf,
                     "Strength range is 3 to %d\n\r.",
                     get_max_train (victim, STAT_STR));
            send_to_char (buf, ch);
            return;
        }

        victim->perm_stat[STAT_STR] = value;
        return;
    }

    if (!str_cmp (arg2, "security"))
    {                            /* OLC */
        if (IS_NPC (ch))
        {
            send_to_char ("NPC's can't set this value.\n\r", ch);
            return;
        }

        if (IS_NPC (victim))
        {
            send_to_char ("Not on NPC's.\n\r", ch);
            return;
        }

        if (value > ch->pcdata->security || value < 0)
        {
            if (ch->pcdata->security != 0)
            {
                sprintf (buf, "Valid security is 0-%d.\n\r",
                         ch->pcdata->security);
                send_to_char (buf, ch);
            }
            else
            {
                send_to_char ("Valid security is 0 only.\n\r", ch);
            }
            return;
        }
        victim->pcdata->security = value;
        return;
    }

    if (!str_cmp (arg2, "int"))
    {
        if (value < 3 || value > get_max_train (victim, STAT_INT))
        {
            sprintf (buf,
                     "Intelligence range is 3 to %d.\n\r",
                     get_max_train (victim, STAT_INT));
            send_to_char (buf, ch);
            return;
        }

        victim->perm_stat[STAT_INT] = value;
        return;
    }

    if (!str_cmp (arg2, "wis"))
    {
        if (value < 3 || value > get_max_train (victim, STAT_WIS))
        {
            sprintf (buf,
                     "Wisdom range is 3 to %d.\n\r", get_max_train (victim,
                                                                    STAT_WIS));
            send_to_char (buf, ch);
            return;
        }

        victim->perm_stat[STAT_WIS] = value;
        return;
    }

    if (!str_cmp (arg2, "dex"))
    {
        if (value < 3 || value > get_max_train (victim, STAT_DEX))
        {
            sprintf (buf,
                     "Dexterity range is 3 to %d.\n\r",
                     get_max_train (victim, STAT_DEX));
            send_to_char (buf, ch);
            return;
        }

        victim->perm_stat[STAT_DEX] = value;
        return;
    }

    if (!str_cmp (arg2, "con"))
    {
        if (value < 3 || value > get_max_train (victim, STAT_CON))
        {
            sprintf (buf,
                     "Constitution range is 3 to %d.\n\r",
                     get_max_train (victim, STAT_CON));
            send_to_char (buf, ch);
            return;
        }

        victim->perm_stat[STAT_CON] = value;
        return;
    }

    if (!str_prefix (arg2, "sex"))
    {
        if (value < 0 || value > 2)
        {
            send_to_char ("Sex range is 0 to 2.\n\r", ch);
            return;
        }
        victim->sex = value;
        if (!IS_NPC (victim))
            victim->pcdata->true_sex = value;
        return;
    }

    if (!str_prefix (arg2, "class"))
    {
        int class;

        if (IS_NPC (victim))
        {
            send_to_char ("Mobiles have no class.\n\r", ch);
            return;
        }

        class = class_lookup (arg3);
        if (class == -1)
        {
            char buf[MAX_STRING_LENGTH];

            strcpy (buf, "Possible classes are: ");
            for (class = 0; class < MAX_CLASS; class++)
            {
                if (class > 0)
                    strcat (buf, " ");
                strcat (buf, class_table[class].name);
            }
            strcat (buf, ".\n\r");

            send_to_char (buf, ch);
            return;
        }

        victim->class = class;
        return;
    }

    if (!str_prefix (arg2, "level"))
    {
        if (!IS_NPC (victim))
        {
            send_to_char ("Not on PC's.\n\r", ch);
            return;
        }

        if (value < 0 || value > MAX_LEVEL)
        {
            sprintf (buf, "Level range is 0 to %d.\n\r", MAX_LEVEL);
            send_to_char (buf, ch);
            return;
        }
        victim->level = value;
        return;
    }

    if (!str_prefix (arg2, "gold"))
    {
        victim->gold = value;
        return;
    }

    if (!str_prefix (arg2, "silver"))
    {
        victim->silver = value;
        return;
    }

    if (!str_prefix (arg2, "hp"))
    {
        if (value < -10 || value > 30000)
        {
            send_to_char ("Hp range is -10 to 30,000 hit points.\n\r", ch);
            return;
        }
        victim->max_hit = value;
        if (!IS_NPC (victim))
            victim->pcdata->perm_hit = value;
        return;
    }

    if (!str_prefix (arg2, "mana"))
    {
        if (value < 0 || value > 30000)
        {
            send_to_char ("Mana range is 0 to 30,000 mana points.\n\r", ch);
            return;
        }
        victim->max_mana = value;
        if (!IS_NPC (victim))
            victim->pcdata->perm_mana = value;
        return;
    }

    if (!str_prefix (arg2, "move"))
    {
        if (value < 0 || value > 30000)
        {
            send_to_char ("Move range is 0 to 30,000 move points.\n\r", ch);
            return;
        }
        victim->max_move = value;
        if (!IS_NPC (victim))
            victim->pcdata->perm_move = value;
        return;
    }

    if (!str_prefix (arg2, "practice"))
    {
        if (value < 0 || value > 250)
        {
            send_to_char ("Practice range is 0 to 250 sessions.\n\r", ch);
            return;
        }
        victim->practice = value;
        return;
    }

    if (!str_prefix (arg2, "train"))
    {
        if (value < 0 || value > 50)
        {
            send_to_char ("Training session range is 0 to 50 sessions.\n\r",
                          ch);
            return;
        }
        victim->train = value;
        return;
    }

    if (!str_prefix (arg2, "align"))
    {
        if (value < -1000 || value > 1000)
        {
            send_to_char ("Alignment range is -1000 to 1000.\n\r", ch);
            return;
        }
        victim->alignment = value;
        return;
    }

    if (!str_prefix (arg2, "thirst"))
    {
        if (IS_NPC (victim))
        {
            send_to_char ("Not on NPC's.\n\r", ch);
            return;
        }

        if (value < -1 || value > 100)
        {
            send_to_char ("Thirst range is -1 to 100.\n\r", ch);
            return;
        }

        victim->pcdata->condition[COND_THIRST] = value;
        return;
    }

    if (!str_prefix (arg2, "drunk"))
    {
        if (IS_NPC (victim))
        {
            send_to_char ("Not on NPC's.\n\r", ch);
            return;
        }

        if (value < -1 || value > 100)
        {
            send_to_char ("Drunk range is -1 to 100.\n\r", ch);
            return;
        }

        victim->pcdata->condition[COND_DRUNK] = value;
        return;
    }

    if (!str_prefix (arg2, "full"))
    {
        if (IS_NPC (victim))
        {
            send_to_char ("Not on NPC's.\n\r", ch);
            return;
        }

        if (value < -1 || value > 100)
        {
            send_to_char ("Full range is -1 to 100.\n\r", ch);
            return;
        }

        victim->pcdata->condition[COND_FULL] = value;
        return;
    }

    if (!str_prefix (arg2, "hunger"))
    {
        if (IS_NPC (victim))
        {
            send_to_char ("Not on NPC's.\n\r", ch);
            return;
        }

        if (value < -1 || value > 100)
        {
            send_to_char ("Full range is -1 to 100.\n\r", ch);
            return;
        }

        victim->pcdata->condition[COND_HUNGER] = value;
        return;
    }

    if (!str_prefix (arg2, "race"))
    {
        int race;

        race = race_lookup (arg3);

        if (race == 0)
        {
            send_to_char ("That is not a valid race.\n\r", ch);
            return;
        }

        if (!IS_NPC (victim) && !race_table[race].pc_race)
        {
            send_to_char ("That is not a valid player race.\n\r", ch);
            return;
        }

        victim->race = race;
        return;
    }

    if (!str_prefix (arg2, "group"))
    {
        if (!IS_NPC (victim))
        {
            send_to_char ("Only on NPCs.\n\r", ch);
            return;
        }
        victim->group = value;
        return;
    }

	if (!str_prefix (arg2, "hours"))
	{
		if (IS_NPC (victim))
		{
			send_to_char ("Not on NPC's.\n\r", ch);
			return;
		}

		if (!is_number (arg3))
		{
			send_to_char ("Value must be numeric.\n\r", ch);
			return;
		}

		value = atoi (arg3);

		if (value < 0 || value > 999)
		{
			send_to_char ("Value must be between 0 and 999.\n\r", ch);
			return;
		}

		victim->played = ( value * 3600 );
		printf_to_char(ch, "%s's hours set to %d.", victim->name, value);

		return;
	}

    /*
     * Generate usage message.
     */
    do_function (ch, &do_mset, "");
    return;
}

void do_string (CHAR_DATA * ch, char *argument)
{
    char type[MAX_INPUT_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;

    smash_tilde (argument);
    argument = one_argument (argument, type);
    argument = one_argument (argument, arg1);
    argument = one_argument (argument, arg2);
    strcpy (arg3, argument);

    if (type[0] == '\0' || arg1[0] == '\0' || arg2[0] == '\0'
        || arg3[0] == '\0')
    {
        send_to_char ("Syntax:\n\r", ch);
        send_to_char ("  string char <name> <field> <string>\n\r", ch);
        send_to_char ("    fields: name short long desc title spec\n\r", ch);
        send_to_char ("  string obj  <name> <field> <string>\n\r", ch);
        send_to_char ("    fields: name short long extended\n\r", ch);
        return;
    }

    if (!str_prefix (type, "character") || !str_prefix (type, "mobile"))
    {
        if ((victim = get_char_world (ch, arg1)) == NULL)
        {
            send_to_char ("They aren't here.\n\r", ch);
            return;
        }

        /* clear zone for mobs */
        victim->zone = NULL;

        /* string something */

        if (!str_prefix (arg2, "name"))
        {
            if (!IS_NPC (victim))
            {
                send_to_char ("Not on PC's.\n\r", ch);
                return;
            }
            free_string (victim->name);
            victim->name = str_dup (arg3);
            return;
        }

        if (!str_prefix (arg2, "description"))
        {
            free_string (victim->description);
            victim->description = str_dup (arg3);
            return;
        }

        if (!str_prefix (arg2, "short"))
        {
            free_string (victim->short_descr);
            victim->short_descr = str_dup (arg3);
            return;
        }

        if (!str_prefix (arg2, "long"))
        {
            free_string (victim->long_descr);
            strcat (arg3, "\n\r");
            victim->long_descr = str_dup (arg3);
            return;
        }

        if (!str_prefix (arg2, "title"))
        {
            if (IS_NPC (victim))
            {
                send_to_char ("Not on NPC's.\n\r", ch);
                return;
            }

            set_title (victim, arg3);
            return;
        }

        if (!str_prefix (arg2, "spec"))
        {
            if (!IS_NPC (victim))
            {
                send_to_char ("Not on PC's.\n\r", ch);
                return;
            }

            if ((victim->spec_fun = spec_lookup (arg3)) == 0)
            {
                send_to_char ("No such spec fun.\n\r", ch);
                return;
            }

            return;
        }
    }

    if (!str_prefix (type, "object"))
    {
        /* string an obj */

        if ((obj = get_obj_world (ch, arg1)) == NULL)
        {
            send_to_char ("Nothing like that in heaven or earth.\n\r", ch);
            return;
        }

        if (!str_prefix (arg2, "name"))
        {
            free_string (obj->name);
            obj->name = str_dup (arg3);
            return;
        }

        if (!str_prefix (arg2, "short"))
        {
            free_string (obj->short_descr);
            obj->short_descr = str_dup (arg3);
            return;
        }

        if (!str_prefix (arg2, "long"))
        {
            free_string (obj->description);
            obj->description = str_dup (arg3);
            return;
        }

        if (!str_prefix (arg2, "ed") || !str_prefix (arg2, "extended"))
        {
            EXTRA_DESCR_DATA *ed;

            argument = one_argument (argument, arg3);
            if (argument == NULL)
            {
                send_to_char
                    ("Syntax: oset <object> ed <keyword> <string>\n\r", ch);
                return;
            }

            strcat (argument, "\n\r");

            ed = new_extra_descr ();

            ed->keyword = str_dup (arg3);
            ed->description = str_dup (argument);
            ed->next = obj->extra_descr;
            obj->extra_descr = ed;
            return;
        }
    }


    /* echo bad use message */
    do_function (ch, &do_string, "");
}



void do_oset (CHAR_DATA * ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int value;

    smash_tilde (argument);
    argument = one_argument (argument, arg1);
    argument = one_argument (argument, arg2);
    strcpy (arg3, argument);

    if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0')
    {
        send_to_char ("Syntax:\n\r", ch);
        send_to_char ("  set obj <object> <field> <value>\n\r", ch);
        send_to_char ("  Field being one of:\n\r", ch);
        send_to_char ("    value0 value1 value2 value3 value4 (v1-v4)\n\r",
                      ch);
        send_to_char ("    extra wear level weight cost timer\n\r", ch);
        return;
    }

    if ((obj = get_obj_world (ch, arg1)) == NULL)
    {
        send_to_char ("Nothing like that in heaven or earth.\n\r", ch);
        return;
    }

    /*
     * Snarf the value (which need not be numeric).
     */
    value = atoi (arg3);

    /*
     * Set something.
     */
    if (!str_cmp (arg2, "value0") || !str_cmp (arg2, "v0"))
    {
        obj->value[0] = UMIN (50, value);
        return;
    }

    if (!str_cmp (arg2, "value1") || !str_cmp (arg2, "v1"))
    {
        obj->value[1] = value;
        return;
    }

    if (!str_cmp (arg2, "value2") || !str_cmp (arg2, "v2"))
    {
        obj->value[2] = value;
        return;
    }

    if (!str_cmp (arg2, "value3") || !str_cmp (arg2, "v3"))
    {
        obj->value[3] = value;
        return;
    }

    if (!str_cmp (arg2, "value4") || !str_cmp (arg2, "v4"))
    {
        obj->value[4] = value;
        return;
    }

    if (!str_prefix (arg2, "extra"))
    {
        obj->extra_flags = value;
        return;
    }

    if (!str_prefix (arg2, "wear"))
    {
        obj->wear_flags = value;
        return;
    }

    if (!str_prefix (arg2, "level"))
    {
        obj->level = value;
        return;
    }

    if (!str_prefix (arg2, "weight"))
    {
        obj->weight = value;
        return;
    }

    if (!str_prefix (arg2, "cost"))
    {
        obj->cost = value;
        return;
    }

    if (!str_prefix (arg2, "timer"))
    {
        obj->timer = value;
        return;
    }

    /*
     * Generate usage message.
     */
    do_function (ch, &do_oset, "");
    return;
}



void do_rset (CHAR_DATA * ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    int value;

    smash_tilde (argument);
    argument = one_argument (argument, arg1);
    argument = one_argument (argument, arg2);
    strcpy (arg3, argument);

    if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0')
    {
        send_to_char ("Syntax:\n\r", ch);
        send_to_char ("  set room <location> <field> <value>\n\r", ch);
        send_to_char ("  Field being one of:\n\r", ch);
        send_to_char ("    flags sector\n\r", ch);
        return;
    }

    if ((location = find_location (ch, arg1)) == NULL)
    {
        send_to_char ("No such location.\n\r", ch);
        return;
    }

    if (!is_room_owner (ch, location) && ch->in_room != location
        && room_is_private (location) && !IS_TRUSTED (ch, IMPLEMENTOR))
    {
        send_to_char ("That room is private right now.\n\r", ch);
        return;
    }

    /*
     * Snarf the value.
     */
    if (!is_number (arg3))
    {
        send_to_char ("Value must be numeric.\n\r", ch);
        return;
    }
    value = atoi (arg3);

    /*
     * Set something.
     */
    if (!str_prefix (arg2, "flags"))
    {
        location->room_flags = value;
        return;
    }

    if (!str_prefix (arg2, "sector"))
    {
        location->sector_type = value;
        return;
    }

    /*
     * Generate usage message.
     */
    do_function (ch, &do_rset, "");
    return;
}



void do_sockets (CHAR_DATA * ch, char *argument)
{
    char buf[2 * MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    DESCRIPTOR_DATA *d;
    int count;

    count = 0;
    buf[0] = '\0';

    one_argument (argument, arg);
    for (d = descriptor_list; d != NULL; d = d->next)
    {
        if (d->character != NULL && can_see (ch, d->character)
            && (arg[0] == '\0' || is_name (arg, d->character->name)
                || (d->original && is_name (arg, d->original->name))))
        {
            count++;
            sprintf (buf + strlen (buf), "[%3d %2d] %s@%s\n\r",
                     d->descriptor,
                     d->connected,
                     d->original ? d->original->name :
                     d->character ? d->character->name : "(none)", d->host);
        }
    }
    if (count == 0)
    {
        send_to_char ("No one by that name is connected.\n\r", ch);
        return;
    }

    sprintf (buf2, "%d user%s\n\r", count, count == 1 ? "" : "s");
    strcat (buf, buf2);
    page_to_char (buf, ch);
    return;
}



/*
 * Thanks to Grodyn for pointing out bugs in this function.
 */
void do_force (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    argument = one_argument (argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0')
    {
        send_to_char ("Force whom to do what?\n\r", ch);
        return;
    }

    one_argument (argument, arg2);

    if (!str_cmp (arg2, "delete") || !str_prefix (arg2, "mob"))
    {
        send_to_char ("That will NOT be done.\n\r", ch);
        return;
    }

    sprintf (buf, "$n forces you to '%s'.", argument);

	/* Replaced original block with code by Edwin to keep from
	 * corrupting pfiles in certain pet-infested situations.
	 * JR -- 10/15/00
	 */
	if ( !str_cmp( arg, "all" ) )
	{
    	DESCRIPTOR_DATA *desc,*desc_next;

    	if (get_trust(ch) < MAX_LEVEL - 3)
    	{
			send_to_char("Not at your level!\n\r",ch);
			return;
    	}

    	for ( desc = descriptor_list; desc != NULL; desc = desc_next )
    	{
			desc_next = desc->next;

			if (desc->connected==CON_PLAYING &&
	    		get_trust( desc->character ) < get_trust( ch ) )
	    	{
	    		act( buf, ch, NULL, desc->character, TO_VICT );
	    		interpret( desc->character, argument );
			}
    	}
	}
    else if (!str_cmp (arg, "players"))
    {
        CHAR_DATA *vch;
        CHAR_DATA *vch_next;

        if (get_trust (ch) < MAX_LEVEL - 2)
        {
            send_to_char ("Not at your level!\n\r", ch);
            return;
        }

        for (vch = char_list; vch != NULL; vch = vch_next)
        {
            vch_next = vch->next;

            if (!IS_NPC (vch) && get_trust (vch) < get_trust (ch)
                && vch->level < LEVEL_HERO)
            {
                act (buf, ch, NULL, vch, TO_VICT);
                interpret (vch, argument);
            }
        }
    }
    else if (!str_cmp (arg, "gods"))
    {
        CHAR_DATA *vch;
        CHAR_DATA *vch_next;

        if (get_trust (ch) < MAX_LEVEL - 2)
        {
            send_to_char ("Not at your level!\n\r", ch);
            return;
        }

        for (vch = char_list; vch != NULL; vch = vch_next)
        {
            vch_next = vch->next;

            if (!IS_NPC (vch) && get_trust (vch) < get_trust (ch)
                && vch->level >= LEVEL_HERO)
            {
                act (buf, ch, NULL, vch, TO_VICT);
                interpret (vch, argument);
            }
        }
    }
    else
    {
        CHAR_DATA *victim;

        if ((victim = get_char_world (ch, arg)) == NULL)
        {
            send_to_char ("They aren't here.\n\r", ch);
            return;
        }

        if (victim == ch)
        {
            send_to_char ("Aye aye, right away!\n\r", ch);
            return;
        }

        if (!is_room_owner (ch, victim->in_room)
            && ch->in_room != victim->in_room
            && room_is_private (victim->in_room)
            && !IS_TRUSTED (ch, IMPLEMENTOR))
        {
            send_to_char ("That character is in a private room.\n\r", ch);
            return;
        }

        if (get_trust (victim) >= get_trust (ch))
        {
            send_to_char ("Do it yourself!\n\r", ch);
            return;
        }

        if (!IS_NPC (victim) && get_trust (ch) < MAX_LEVEL - 3)
        {
            send_to_char ("Not at your level!\n\r", ch);
            return;
        }

        act (buf, ch, NULL, victim, TO_VICT);
        interpret (victim, argument);
    }

    send_to_char ("Ok.\n\r", ch);
    return;
}



/*
 * New routines by Dionysos.
 */
void do_invis (CHAR_DATA * ch, char *argument)
{
    int level;
    char arg[MAX_STRING_LENGTH];

    /* RT code for taking a level argument */
    one_argument (argument, arg);

    if (arg[0] == '\0')
        /* take the default path */

        if (ch->invis_level)
        {
            ch->invis_level = 0;
            act ("$n slowly fades into existence.", ch, NULL, NULL, TO_ROOM);
            send_to_char ("You slowly fade back into existence.\n\r", ch);
        }
        else
        {
            ch->invis_level = get_trust (ch);
            act ("$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM);
            send_to_char ("You slowly vanish into thin air.\n\r", ch);
        }
    else
        /* do the level thing */
    {
        level = atoi (arg);
        if (level < 2 || level > get_trust (ch))
        {
            send_to_char ("Invis level must be between 2 and your level.\n\r",
                          ch);
            return;
        }
        else
        {
            ch->reply = NULL;
            ch->invis_level = level;
            act ("$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM);
            send_to_char ("You slowly vanish into thin air.\n\r", ch);
        }
    }

    return;
}


void do_incognito (CHAR_DATA * ch, char *argument)
{
    int level;
    char arg[MAX_STRING_LENGTH];

    /* RT code for taking a level argument */
    one_argument (argument, arg);

    if (arg[0] == '\0')
        /* take the default path */

        if (ch->incog_level)
        {
            ch->incog_level = 0;
            act ("$n is no longer cloaked.", ch, NULL, NULL, TO_ROOM);
            send_to_char ("You are no longer cloaked.\n\r", ch);
        }
        else
        {
            ch->incog_level = get_trust (ch);
            act ("$n cloaks $s presence.", ch, NULL, NULL, TO_ROOM);
            send_to_char ("You cloak your presence.\n\r", ch);
        }
    else
        /* do the level thing */
    {
        level = atoi (arg);
        if (level < 2 || level > get_trust (ch))
        {
            send_to_char ("Incog level must be between 2 and your level.\n\r",
                          ch);
            return;
        }
        else
        {
            ch->reply = NULL;
            ch->incog_level = level;
            act ("$n cloaks $s presence.", ch, NULL, NULL, TO_ROOM);
            send_to_char ("You cloak your presence.\n\r", ch);
        }
    }

    return;
}



void do_holylight (CHAR_DATA * ch, char *argument)
{
    if (IS_NPC (ch))
        return;

    if (IS_SET (ch->act, PLR_HOLYLIGHT))
    {
        REMOVE_BIT (ch->act, PLR_HOLYLIGHT);
        send_to_char ("Holy light mode off.\n\r", ch);
    }
    else
    {
        SET_BIT (ch->act, PLR_HOLYLIGHT);
        send_to_char ("Holy light mode on.\n\r", ch);
    }

    return;
}

/* prefix command: it will put the string typed on each line typed */

void do_prefi (CHAR_DATA * ch, char *argument)
{
    send_to_char ("You cannot abbreviate the prefix command.\r\n", ch);
    return;
}

void do_prefix (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_INPUT_LENGTH];

    if (argument[0] == '\0')
    {
        if (ch->prefix[0] == '\0')
        {
            send_to_char ("You have no prefix to clear.\r\n", ch);
            return;
        }

        send_to_char ("Prefix removed.\r\n", ch);
        free_string (ch->prefix);
        ch->prefix = str_dup ("");
        return;
    }

    if (ch->prefix[0] != '\0')
    {
        sprintf (buf, "Prefix changed to %s.\r\n", argument);
        free_string (ch->prefix);
    }
    else
    {
        sprintf (buf, "Prefix set to %s.\r\n", argument);
    }

    ch->prefix = str_dup (argument);
}

/* commenting in order to fix gcc errors on fedora core 2
 * 9/21/04

#define CH(descriptor)  ((descriptor)->original ? \
(descriptor)->original : (descriptor)->character)
*/

/* This file holds the copyover data */
#define COPYOVER_FILE "copyover.data"

/* This is the executable file */
#define EXE_FILE      "../area/rom"

/*  Copyover - Original idea: Fusion of MUD++
 *  Adapted to Diku by Erwin S. Andreasen, <erwin@pip.dknet.dk>
 *  http://pip.dknet.dk/~pip1773
 *  Changed into a ROM patch after seeing the 100th request for it :)
 */
void do_copyover (CHAR_DATA * ch, char *argument)
{
    FILE *fp;
    DESCRIPTOR_DATA *d, *d_next;
    char buf[100], buf2[100], buf3[100];
    extern int port, control;    /* db.c */

    fp = fopen (COPYOVER_FILE, "w");

    if (!fp)
    {
        send_to_char ("Copyover file not writeable, aborted.\n\r", ch);
        log_f ("Could not write to copyover file: %s", COPYOVER_FILE);
        perror ("do_copyover:fopen");
        return;
    }

    /* Consider changing all saved areas here, if you use OLC */

    /* do_asave (NULL, ""); - autosave changed areas */


    sprintf (buf, "\n\r *** COPYOVER by %s - please remain seated!\n\r",
             ch->name);

    /* For each playing descriptor, save its state */
    for (d = descriptor_list; d; d = d_next)
    {
        CHAR_DATA *och = CH (d);
        d_next = d->next;        /* We delete from the list , so need to save this */

        if (!d->character || d->connected < CON_PLAYING)
        {                        /* drop those logging on */
            write_to_descriptor (d->descriptor,
                                 "\n\rSorry, we are rebooting. Come back in a few minutes.\n\r",
                                 0);
            close_socket (d);    /* throw'em out */
        }
        else
        {
            fprintf (fp, "%d %s %s\n", d->descriptor, och->name, d->host);

#if 0                            /* This is not necessary for ROM */
            if (och->level == 1)
            {
                write_to_descriptor (d->descriptor,
                                     "Since you are level one, and level one characters do not save, you gain a free level!\n\r",
                                     0);
                advance_level (och);
                och->level++;    /* Advance_level doesn't do that */
            }
#endif
            save_char_obj (och);

            write_to_descriptor (d->descriptor, buf, 0);
        }
    }

    fprintf (fp, "-1\n");
    fclose (fp);

    /* Close reserve and other always-open files and release other resources */

    fclose (fpReserve);

#ifdef IMC
   imc_hotboot();
#endif

    /* exec - descriptors are inherited */

    sprintf (buf, "%d", port);
    sprintf (buf2, "%d", control);
#ifdef IMC
   if( this_imcmud )
      snprintf( buf3, 100, "%d", this_imcmud->desc );
   else
      strncpy( buf3, "-1", 100 );
#else
   strncpy( buf3, "-1", 100 );
#endif
    execl (EXE_FILE, "rom", buf, "copyover", buf2, buf3, (char *) NULL);

    /* Failed - sucessful exec will not return */

    perror ("do_copyover: execl");
    send_to_char ("Copyover FAILED!\n\r", ch);

    /* Here you might want to reopen fpReserve */
    fpReserve = fopen (NULL_FILE, "r");
}

/* Recover from a copyover - load players */
void copyover_recover ()
{
    DESCRIPTOR_DATA *d;
    FILE *fp;
    char name[100];
    char host[MSL];
    int desc;
    bool fOld;

    log_f ("Copyover recovery initiated");

    fp = fopen (COPYOVER_FILE, "r");

    if (!fp)
    {                            /* there are some descriptors open which will hang forever then ? */
        perror ("copyover_recover:fopen");
        log_f ("Copyover file not found. Exitting.\n\r");
        exit (1);
    }

    unlink (COPYOVER_FILE);        /* In case something crashes - doesn't prevent reading  */

    for (;;)
    {
        int errorcheck = fscanf (fp, "%d %s %s\n", &desc, name, host);
        if ( errorcheck < 0 )
            break;
        if (desc == -1)
            break;

        /* Write something, and check if it goes error-free */
        if (!write_to_descriptor
            (desc, "\n\rRestoring from copyover...\n\r", 0))
        {
            close (desc);        /* nope */
            continue;
        }

        d = new_descriptor ();
        d->descriptor = desc;

        d->host = str_dup (host);
        d->next = descriptor_list;
        descriptor_list = d;
        d->connected = CON_COPYOVER_RECOVER;    /* -15, so close_socket frees the char */


        /* Now, find the pfile */

        fOld = load_char_obj (d, name);

        if (!fOld)
        {                        /* Player file not found?! */
            write_to_descriptor (desc,
                                 "\n\rSomehow, your character was lost in the copyover. Sorry.\n\r",
                                 0);
            close_socket (d);
        }
        else
        {                        /* ok! */

            write_to_descriptor (desc, "\n\rCopyover recovery complete.\n\r",
                                 0);

            /* Just In Case */
            if (!d->character->in_room)
                d->character->in_room = get_room_index (ROOM_VNUM_TEMPLE);

            /* Insert in the char_list */
            d->character->next = char_list;
            char_list = d->character;

            char_to_room (d->character, d->character->in_room);
            do_look (d->character, "auto");
            act ("$n materializes!", d->character, NULL, NULL, TO_ROOM);
            d->connected = CON_PLAYING;

            if (d->character->pet != NULL)
            {
                char_to_room (d->character->pet, d->character->in_room);
                act ("$n materializes!.", d->character->pet, NULL, NULL,
                     TO_ROOM);
            }
        }

    }
    fclose (fp);


}

/* This _should_ encompass all the QuickMUD config commands */
/* -- JR 11/24/00                                           */

void do_qmconfig (CHAR_DATA * ch, char * argument)
{
	extern int mud_ansiprompt;
	extern int mud_ansicolor;
	extern int mud_telnetga;
	extern char *mud_ipaddress;
	char arg1[MSL];
	char arg2[MSL];

	if (IS_NPC(ch))
		return;

	if (argument[0] == '\0')
	{
		printf_to_char(ch, "Valid qmconfig options are:\n\r");
		printf_to_char(ch, "    show       (shows current status of toggles)\n\r");
		printf_to_char(ch, "    ansiprompt [on|off]\n\r");
		printf_to_char(ch, "    ansicolor  [on|off]\n\r");
		printf_to_char(ch, "    telnetga   [on|off]\n\r");
		printf_to_char(ch, "    read\n\r");
		return;
	}

	argument = one_argument( argument, arg1 );
	argument = one_argument( argument, arg2 );

	if (!str_prefix(arg1, "read")) {
		qmconfig_read();
		return;
	}

	if (!str_prefix(arg1, "show"))
	{
		printf_to_char(ch, "ANSI prompt: %s", mud_ansiprompt ? "{GON{x\n\r" : "{ROFF{x\n\r");
		printf_to_char(ch, "ANSI color : %s", mud_ansicolor ? "{GON{x\n\r" : "{ROFF{x\n\r");
		printf_to_char(ch, "IP Address : %s\n\r", mud_ipaddress);
		printf_to_char(ch, "Telnet GA  : %s", mud_telnetga ? "{GON{x\n\r" : "{ROFF{x\n\r");
		return;
	}

	if (!str_prefix(arg1, "ansiprompt"))
	{
		if (!str_prefix(arg2, "on"))
		{
			mud_ansiprompt=TRUE;
			printf_to_char(ch, "New logins will now get an ANSI color prompt.\n\r");
			return;
		}

		else if(!str_prefix(arg2, "off"))
		{
			mud_ansiprompt=FALSE;
			printf_to_char(ch, "New logins will not get an ANSI color prompt.\n\r");
			return;
		}

		printf_to_char(ch, "Valid arguments are \"on\" and \"off\".\n\r");
		return;
	}

	if (!str_prefix(arg1, "ansicolor"))
	{
		if (!str_prefix(arg2, "on"))
		{
			mud_ansicolor=TRUE;
			printf_to_char(ch, "New players will have color enabled.\n\r");
			return;
		}

		else if (!str_prefix(arg2, "off"))
		{
			mud_ansicolor=FALSE;
			printf_to_char(ch, "New players will not have color enabled.\n\r");
			return;
		}

		printf_to_char(ch, "Valid arguments are \"on\" and \"off\".\n\r");
		return;
	}

	if (!str_prefix(arg1, "telnetga"))
	{
		if (!str_prefix(arg2, "on"))
		{
			mud_telnetga=TRUE;
			printf_to_char(ch, "Telnet GA will be enabled for new players.\n\r");
			return;
		}

		else if (!str_prefix(arg2, "off"))
		{
			mud_telnetga=FALSE;
			printf_to_char(ch, "Telnet GA will be disabled for new players.\n\r");
			return;
		}

		printf_to_char(ch, "Valid arguments are \"on\" and \"off\".\n\r");
		return;
	}

	printf_to_char(ch, "I have no clue what you are trying to do...\n\r");
	return;
}

void qmconfig_read (void) {
	FILE *fp;
	bool fMatch;
	char *word;
	extern int mud_ansiprompt, mud_ansicolor, mud_telnetga;

	log_f("Loading configuration settings from ../area/qmconfig.rc.");

	fp = fopen("../area/qmconfig.rc","r");
	if (!fp) {
		log_f("qmconfig.rc not found. Using compiled-in defaults.");
		return;
	}

	for(;;) {
		word = feof (fp) ? "END" : fread_word(fp);

		fMatch = FALSE;

		switch (UPPER(word[0])) {
			case '#':
				/* This is a comment line! */
				fMatch = TRUE;
				fread_to_eol (fp);
				break;
			case '*':
				fMatch = TRUE;
				fread_to_eol (fp);
				break;

			case 'A':
				KEY ("Ansicolor", mud_ansicolor, fread_number(fp));
				KEY ("Ansiprompt", mud_ansiprompt, fread_number(fp));
				break;
			case 'E':
				if (!str_cmp(word, "END"))
					return;
				break;
			case 'T':
				KEY ("Telnetga", mud_telnetga, fread_number(fp));
				break;
		}
		if (!fMatch) {
			log_f("qmconfig_read: no match for %s!", word);
			fread_to_eol(fp);
		}
	}
	log_f("Settings have been read from ../area/qmconfig.rc");
	exit(0);

}
