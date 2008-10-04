#!/bin/bash
function do_checkinstall()
{
# $1 : Version
# $2 : PkgArch
# $3 : PkgRelease
# $4 : Required packages
	sudo checkinstall -RD --pkgname=mjs --pkgversion=$1 --pkgarch=$2 --pkgrelease=$3 --pkggroup=Sound --pkgsource=http://www.bolkhuis.nl --pkgaltsource=http://www.bolkhuis.nl/trac/mjs/ --maintainer=beheer@bolkhuis.nl --requires=$4
}
echo "********************************************************"
echo "** Dit is het magische MJS maak debian package script **"
echo "********************************************************"
echo -n "Enter Release: "
read VAR
sudo rm -Rf /tmp/mjs_build
sudo svn export http://serv/svn/mjs/trunk /tmp/mjs_build
cd /tmp/mjs_build 
if [[ "$1" == "i386" ]]
then 
	sudo ./configure --prefix=/usr --host=i386 --build=i386
	do_checkinstall 4.0 i386 $VAR libxine1,libncurses5
fi
if [[ "$1" == "amd64" ]]
then
	sudo ./configure --prefix=/usr --host=amd64 --build=amd64
	do_checkinstall 4.0 amd64 $VAR libxine1,libncurses5
fi
#scp *.deb root@serv:/srv/www/html/mjs/.
ssh root@serv "cd /srv/www/html/mjs && dpkg-scanpackages -m . /dev/null | gzip -9c > /srv/www/html/mjs/Packages.gz"
sudo rm -Rf /tmp/mjs_build
