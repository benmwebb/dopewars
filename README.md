[![Build Status](https://github.com/benmwebb/dopewars/workflows/build/badge.svg?branch=develop)](https://github.com/benmwebb/dopewars/actions?query=workflow%3Abuild)
[![Download dopewars drug dealing game](https://img.shields.io/sourceforge/dt/dopewars.svg)](https://dopewars.sourceforge.io/download.html)

This is dopewars 1.6.1, a game simulating the life of a drug dealer in
New York. The aim of the game is to make lots and lots of money...
unfortunately, you start the game with a hefty debt, accumulating interest,
and the cops take a rather dim view of drug dealing...

These are brief instructions; see the HTML documentation for full information.

dopewars 1.6.1 servers should handle clients as old as version 1.4.3 with
hardly any visible problems (the reverse is also true). However, it is
recommended that both clients and servers are upgraded to 1.6.1!

## Installation

Either...

1. Get the relevant RPM from https://dopewars.sourceforge.io/
   
Or...

1. Get the tarball `dopewars-1.6.1.tar.gz` from the same URL
2. Extract it via `tar -xvzf dopewars-1.6.1.tar.gz`
3. Follow the instructions in the `INSTALL` file in the newly-created
   `dopewars-1.6.1` directory

Once you're done, you can safely delete the RPM, tarball and dopewars
directory. The dopewars binary is all you need!

dopewars stores its high score files by default in `/usr/share/dopewars.sco`
This will be created by make install or by RPM installation. A different high 
score file can be selected with the `-f` switch.

## Windows installation

dopewars now compiles as a console or regular application under Win32 (Windows 7
or later). Almost all functionality of the standard Unix binary is retained;
for example, all of the same command line switches are supported. However, for
convenience, the configuration file is the more Windows-friendly
"dopewars-config.txt".

The easiest way to install the Win32 version is to download the precompiled
binary. To build from source, see the `win32` directory.

## Usage

dopewars has built-in client-server support for multi-player games. For a
full list of options configurable on the command line, run dopewars with
the `-h` switch.

`dopewars -a`  
This is "antique" dopewars; it tries to keep to the original dopewars, based
on the "Drug Wars" game by John E. Dell, as closely as possible.

`dopewars`  
By default, dopewars supports multi-player games. On starting a game, the
program will attempt to connect to a dopewars server so that players can send
messages back and forth, and shoot each other if they really want to...

`dopewars -s`  
Starts a dopewars server. This sits in the background and handles multi-player
games. You probably want to use the `-l` command line option too to direct its
log output to somewhere sensible.

`dopewars -c`  
Create and run a computer dopewars player. This will attempt to connect
to a dopewars server, and if this succeeds, it will then participate in
multi-player dopewars games.

## Configuration

Most of the dopewars defaults (for example, the location of the high score file,
the port and server to connect to, the names of the drugs and guns, etc.) can be
configured by adding suitable entries to the dopewars configuration file. The
global file `/etc/dopewars` is read first, and can then be overridden by
the local settings in `~/.dopewars`. All of the settings here can also be
set on the command line of an interactive dopewars server when no players
are logged on. See the file "example-cfg" for an example configuration file,
and for a brief explanation of each option, type "help" in an interactive
server. A subset of the configuration options can also be tweaked via the
"Options" menu item in the GTK+/Win32 client.

## Playing

dopewars is supposed to be fairly self-explanatory. You should be able to 
pick the basics up fairly quickly, but still be discovering subtleties for 
_ages_ ;) If you're _really_ stuck, send me an email. I might even answer it!

Clue: buy drugs when they're cheap, sell them when they're expensive. The Bronx
and Ghetto are "special" locations. Anything more would spoil the fun. ;)

## Bugs

Well, there are bound to be lots. Let me know if you find any by
[opening an issue](https://github.com/benmwebb/dopewars/issues), and I'll see
if I can fix 'em... of course, a
[working patch](https://github.com/benmwebb/dopewars/pulls) would be even
nicer! ;)

## License

dopewars is released under the GNU General Public License; see the text file
LICENCE for further information. dopewars is copyright (C) Ben Webb 1998-2022.
The dopewars icons are copyright (C) Ocelot Mantis 2001.

## Support

dopewars is written and maintained by Ben Webb <benwebb@users.sf.net>  
Enquiries about dopewars may be sent to this address (keep them sensible 
please ;) Bug fixes and reports, improvements and patches are also welcomed.
