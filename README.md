QuickMUD is derived from ROM 2.4b6, Merc 2.1 and DikuMUD
==============

## Introduction

QuickMUD / ROM is a "[multi-user dungeon](https://en.wikipedia.org/wiki/MUD)", a text-based MMORPG. ROM is well-known for its fast-paced and exciting combat system. It also happens to be the initial codebase for [Carrion Fields](http://www.carrionfields.net/), the greatest MUD of all time.

## Docker Image

You can run the pre-built docker image with the following command:

```docker run -d -p 4000:4000 avinson/rom```

After that, connect like this:

```telnet localhost 4000```

If you have docker-compose available, a simple `docker-compose up` will start the
container and mount the player and log directories for easier saving/editing of
player files.

## First Immortal

A level 2 character named Shemp is included with a password of `psswrd`. You
can edit this file to create an immortal character or follow the instructions
below.

-----

To make your first immortal character, just start as a mortal
character, play at least as far as level 2, and then edit the
player file and change your level.  (After the first immortal,
you can advance the rest)

QuickMUD is a Rom24b6 codebase with the following major features added:

* OLC 1.81
* Lope's Color 2.0
* Erwin's Copyover
* Erwin's Noteboard
* Color Login

It is still basically a "stock" ROM server.  The  functionality  of the
code hasn't been modified much except for the addition of  OLC. Changes
are pretty much limited to cosmetic features, like color login.  If you
want to start your own ROM based server, this code can give you a quick
start with some standard 'extra features' already implemented. However,
for the sake of the mudding community at  large,  don't  just  download
this code, compile it, and advertise it on MUD websites  as  "a  highly
modified  ROM  codebase".  Spend  some  time  developing  it. The world
doesn't need another cookie-cutter MUD.

-----

This is the ROM 2.4 beta version of Merc 2.1 base code.
Please read the file in /Rom24/doc called rom.license before using
this program.

-----

Merc Diku Mud is a Diku Mud with many enhancements and contributions.  See our
'contrib.txt' and 'help merc' for acknowledgements.  Send us your contribution,
and you'll be in there too!

Enjoy our mud.  May your visit here be ... Mercenary.

This is the 2.1 production release of Merc.

-----

See other READMEs in the repo for full info and licenses.
