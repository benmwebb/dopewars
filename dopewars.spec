Summary:   Drug dealing game
Name:      dopewars
Version:   cvs
Release:   1
Vendor:    Ben Webb
License:   GPL
Group:     Amusements/Games
Source0:   dopewars-cvs.tar.gz

BuildRoot: /tmp/dopewars-rpm

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
./configure --prefix=/usr
make DESTDIR=${RPM_BUILD_ROOT}

%install
make DESTDIR=${RPM_BUILD_ROOT} install-strip
${RPM_BUILD_ROOT}/usr/bin/dopewars -C ${RPM_BUILD_ROOT}/usr/share/dopewars.sco

%clean
rm -rf ${RPM_BUILD_ROOT}

%files
%doc ChangeLog LICENCE README
%doc doc/aiplayer.html doc/clientplay.html doc/configfile.html doc/credits.html
%doc doc/developer.html doc/example-cfg doc/i18n.html doc/index.html
%doc doc/installation.html doc/metaserver.html doc/server.html 
%doc doc/servercommands.html doc/windows.html
/usr/bin/dopewars
%config /usr/share/dopewars.sco
/usr/man/man6/dopewars.6.gz
/usr/share/gnome/apps/Games/dopewars.desktop
/usr/share/pixmaps/dopewars-pill.png
/usr/share/pixmaps/dopewars-weed.png
/usr/share/pixmaps/dopewars-shot.png
/usr/share/locale/de/LC_MESSAGES/dopewars.mo
/usr/share/locale/pl/LC_MESSAGES/dopewars.mo
/usr/share/locale/pt_BR/LC_MESSAGES/dopewars.mo
/usr/share/locale/fr/LC_MESSAGES/dopewars.mo

%changelog
* Wed Sep 26 2001 Ben Webb <ben@bellatrix.pcl.ox.ac.uk>
   - Added support for a buildroot
