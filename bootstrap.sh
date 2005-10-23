# $Id: bootstrap.sh 89 2005-08-24 22:57:12Z lennart $

# This file is part of libdaemon.
#
# libdaemon is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# libdaemon is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with libdaemon; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.

FLAGS="--sysconfdir=/etc"

case `uname -s` in
    Darwin)
    export LIBTOOLIZE=/opt/local/bin/glibtoolize
    export PKG_CONFIG_PATH="/opt/local/lib/pkgconfig"
    FLAGS="$FLAGS --prefix=/opt/local --disable-lynx"
    ;;
    FreeBSD)
    cp /usr/local/share/aclocal/libtool15.m4 common
    export LIBTOOLIZE=/usr/local/bin/libtoolize15
    export PKG_CONFIG_PATH="/usr/local/lib/pkgconfig"
    ;;
    Linux)
    ;;
esac

CFLAGS="$CFLAGS -g -O0" exec ./autogen.sh $FLAGS "$@"
