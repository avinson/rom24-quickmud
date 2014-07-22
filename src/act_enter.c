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
 ***************************************************************************/

/***************************************************************************
 *  ROM 2.4 is copyright 1993-1998 Russ Taylor                             *
 *  ROM has been brought to you by the ROM consortium                      *
 *      Russ Taylor (rtaylor@hypercube.org)                                *
 *      Gabrielle Taylor (gtaylor@efn.org)                                 *
 *      Brian Moore (zump@rom.org)                                         *
 *  By using this code, you have agreed to follow the terms of the         *
 *  ROM license, in the file Rom24/doc/rom.license                         *
 ***************************************************************************/

/*   QuickMUD - The Lazy Man's ROM - $Id: act_enter.c,v 1.2 2000/12/01 10:48:33 ring0 Exp $ */

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
#include "merc.h"
#include "interp.h"

/* random room generation procedure */
ROOM_INDEX_DATA *get_random_room (CHAR_DATA * ch)
{
    ROOM_INDEX_DATA *room;

    for (;;)
    {
        room = get_room_index (number_range (0, 65535));
        if (room != NULL)
            if (can_see_room (ch, room)
                && !room_is_private (room)
                && !IS_SET (room->room_flags, ROOM_PRIVATE)
                && !IS_SET (room->room_flags, ROOM_SOLITARY)
                && !IS_SET (room->room_flags, ROOM_SAFE)
                && (IS_NPC (ch) || IS_SET (ch->act, ACT_AGGRESSIVE)
                    || !IS_SET (room->room_flags, ROOM_LAW)))
                break;
    }

    return room;
}

/* RT Enter portals */
void do_enter (CHAR_DATA * ch, char *argument)
{
    ROOM_INDEX_DATA *location;

    if (ch->fighting != NULL)
        return;

    /* nifty portal stuff */
    if (argument[0] != '\0')
    {
        ROOM_INDEX_DATA *old_room;
        OBJ_DATA *portal;
        CHAR_DATA *fch, *fch_next;

        old_room = ch->in_room;

        portal = get_obj_list (ch, argument, ch->in_room->contents);

        if (portal == NULL)
        {
            send_to_char ("You don't see that here.\n\r", ch);
            return;
        }

        if (portal->item_type != ITEM_PORTAL
            || (IS_SET (portal->value[1], EX_CLOSED)
                && !IS_TRUSTED (ch, ANGEL)))
        {
            send_to_char ("You can't seem to find a way in.\n\r", ch);
            return;
        }

        if (!IS_TRUSTED (ch, ANGEL)
            && !IS_SET (portal->value[2], GATE_NOCURSE)
            && (IS_AFFECTED (ch, AFF_CURSE)
                || IS_SET (old_room->room_flags, ROOM_NO_RECALL)))
        {
            send_to_char ("Something prevents you from leaving...\n\r", ch);
            return;
        }

        if (IS_SET (portal->value[2], GATE_RANDOM) || portal->value[3] == -1)
        {
            location = get_random_room (ch);
            portal->value[3] = location->vnum;    /* for record keeping :) */
        }
        else if (IS_SET (portal->value[2], GATE_BUGGY)
                 && (number_percent () < 5)) location = get_random_room (ch);
        else
            location = get_room_index (portal->value[3]);

        if (location == NULL
            || location == old_room
            || !can_see_room (ch, location)
            || (room_is_private (location) && !IS_TRUSTED (ch, IMPLEMENTOR)))
        {
            act ("$p doesn't seem to go anywhere.", ch, portal, NULL,
                 TO_CHAR);
            return;
        }

        if (IS_NPC (ch) && IS_SET (ch->act, ACT_AGGRESSIVE)
            && IS_SET (location->room_flags, ROOM_LAW))
        {
            send_to_char ("Something prevents you from leaving...\n\r", ch);
            return;
        }

        act ("$n steps into $p.", ch, portal, NULL, TO_ROOM);

        if (IS_SET (portal->value[2], GATE_NORMAL_EXIT))
            act ("You enter $p.", ch, portal, NULL, TO_CHAR);
        else
            act ("You walk through $p and find yourself somewhere else...",
                 ch, portal, NULL, TO_CHAR);

        char_from_room (ch);
        char_to_room (ch, location);

        if (IS_SET (portal->value[2], GATE_GOWITH))
        {                        /* take the gate along */
            obj_from_room (portal);
            obj_to_room (portal, location);
        }

        if (IS_SET (portal->value[2], GATE_NORMAL_EXIT))
            act ("$n has arrived.", ch, portal, NULL, TO_ROOM);
        else
            act ("$n has arrived through $p.", ch, portal, NULL, TO_ROOM);

        do_function (ch, &do_look, "auto");

        /* charges */
        if (portal->value[0] > 0)
        {
            portal->value[0]--;
            if (portal->value[0] == 0)
                portal->value[0] = -1;
        }

        /* protect against circular follows */
        if (old_room == location)
            return;

        for (fch = old_room->people; fch != NULL; fch = fch_next)
        {
            fch_next = fch->next_in_room;

            if (portal == NULL || portal->value[0] == -1)
                /* no following through dead portals */
                continue;

            if (fch->master == ch && IS_AFFECTED (fch, AFF_CHARM)
                && fch->position < POS_STANDING)
                do_function (fch, &do_stand, "");

            if (fch->master == ch && fch->position == POS_STANDING)
            {

                if (IS_SET (ch->in_room->room_flags, ROOM_LAW)
                    && (IS_NPC (fch) && IS_SET (fch->act, ACT_AGGRESSIVE)))
                {
                    act ("You can't bring $N into the city.",
                         ch, NULL, fch, TO_CHAR);
                    act ("You aren't allowed in the city.",
                         fch, NULL, NULL, TO_CHAR);
                    continue;
                }

                act ("You follow $N.", fch, NULL, ch, TO_CHAR);
                do_function (fch, &do_enter, argument);
            }
        }

        if (portal != NULL && portal->value[0] == -1)
        {
            act ("$p fades out of existence.", ch, portal, NULL, TO_CHAR);
            if (ch->in_room == old_room)
                act ("$p fades out of existence.", ch, portal, NULL, TO_ROOM);
            else if (old_room->people != NULL)
            {
                act ("$p fades out of existence.",
                     old_room->people, portal, NULL, TO_CHAR);
                act ("$p fades out of existence.",
                     old_room->people, portal, NULL, TO_ROOM);
            }
            extract_obj (portal);
        }

        /* 
         * If someone is following the char, these triggers get activated
         * for the followers before the char, but it's safer this way...
         */
        if (IS_NPC (ch) && HAS_TRIGGER (ch, TRIG_ENTRY))
            mp_percent_trigger (ch, NULL, NULL, NULL, TRIG_ENTRY);
        if (!IS_NPC (ch))
            mp_greet_trigger (ch);

        return;
    }

    send_to_char ("Nope, can't do it.\n\r", ch);
    return;
}
