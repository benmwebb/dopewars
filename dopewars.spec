Summary: Drug dealing game
Name: dopewars
Version: 1.4.8-devel
Release: 1
Distribution: Red Hat Linux
Vendor: Ben Webb
Copyright: GPL
Group: Games
Source0: dopewars-1.4.8-devel.tar.gz

%description
Based on John E. Dell's old Drug Wars game, dopewars is a simulation of an    
imaginary drug market.  dopewars is an All-American game which features       
buying, selling, and trying to get past the cops!                              
                                                                                
The first thing you need to do is pay off your debt to the Loan Shark. After   
that, your goal is to make as much money as possible (and stay alive)! You     
have one month of game time to make your fortune.                              

dopewars supports multiple players via. TCP/IP. Chatting to and fighting
with other players (computer or human) is supported; check the command line
switches (via dopewars -h) for further information. 

%prep
%setup
%build
./configure --bindir=/usr/bin --datadir=/var/lib/games
make

%install
make install-strip

%files
%doc ChangeLog LICENCE README
%doc aiplayer.html clientplay.html configfile.html credits.html
%doc developer.html example-cfg i18n.html index.html installation.html
%doc metaserver.html server.html servercommands.html windows.html

/usr/bin/dopewars
/var/lib/games/dopewars.sco
