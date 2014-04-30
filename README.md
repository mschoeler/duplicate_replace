duplicate_replace
=================

File Duplicate Replace

Duplicate Replace
Author: Markus Schoeler

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.


Usage: dreplace PATH
Use duplicate removal to search for duplicates under the provided PATH ignoring SymLinks.
Duplicates are determined by using the MD5 Hash of the filecontent ignoring the time stamp.
Duplicates are removed by creating hard links to the first file found with the same hash.
Also orphaned hard links are considered: This way if one file with inode I_old is hardlinked 
to a new inode I_new all other files found with I_old will also be hardlinked to I_new without 
recalulating hash-sums.

Replacing same content files with hardlinks is risky in environments where files are getting changed, since this
will change all hardlinked files as well.
Thus, do only use dreplace on static file directories like backups, image databases.

Since hardlinks are being used this program only works on UNIX filesystems supporting hardlinks (f.e. ext4)

Dependecies:
Qt4
OpenSSL