/* 
 * *****************************************************************************************
Duplicate Removal
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
Use duplicate removal to search for duplicates under the provided PATH.
Duplicates are determined by using the MD5 Hash of the filecontent ignoring the time stamp.
Duplicates are removed by creating hard links to the first file found with the same hash.
Also orphaned hard links are considered: This way if one file with inode I_old is hardlinked 
to a new inode I_new all other files found with I_old will also be hardlinked to I_new without 
recalulating hash-sums.

Replacing same content files with hardlinks is risky in environments where files are getting changed, since this
will change all hardlinked files as well.
Thus, do only use dreplace on static file directories like backups, image databases.

Since hardlinks are being used this program only works on UNIX filesystems supporting hardlinks (f.e. ext4)
****************************************************************************************** 
*/

#include <QCoreApplication>
#include <QDir>
#include <iostream>
#include <string>
#include <QProcess>
#include <QDebug>
#include <QDirIterator>
#include <map>
#include <unistd.h>
#include <openssl/crypto.h>
#include <openssl/md5.h>
#include <unordered_set>
#include <sys/stat.h>

QDir WorkingPath;
bool DryRun = false;

using namespace std;


QByteArray GetHashSum (QString path)
{
    FILE* file = fopen(path.toStdString().c_str(), "rb");

    unsigned char hash[MD5_DIGEST_LENGTH];
    MD5_CTX md5;
    MD5_Init(&md5);
    const int bufSize = 32768;
    char* buffer = (char*) malloc(bufSize);
    int bytesRead = 0;
    while((bytesRead = fread(buffer, 1, bufSize, file)))
    {
        MD5_Update(&md5, buffer, bytesRead);
    }
    MD5_Final(hash, &md5);
    QByteArray HashArray;
    HashArray = QByteArray::fromRawData((char*) hash,sizeof (hash));
//     qDebug() << "MD5: " << HashArray.toHex();
    fclose(file);
    free(buffer);
    return HashArray.toHex();
}      

ino_t GetFileInode(QString Filename)
{
  struct stat state; 
  int ret = stat(Filename.toStdString().c_str(), &state);
  if (ret == -1)
  {
    qWarning() << "In File" << Filename;	
    qWarning() << "Failure getting inode";
    qWarning() << "Error code " << ret;
    return 0;    
  }
//   qDebug() << "INODE by function: " << state.st_ino;
  return state.st_ino;
}



bool ReplaceWithHardLink(QString SourceFile,QString TargetFile)
{
  unlink(SourceFile.toStdString().c_str());
  int ret = link(TargetFile.toStdString().c_str(),SourceFile.toStdString().c_str());
  if (ret != 0) 
  {
    qWarning() << "Doing link " << SourceFile << " --> " << TargetFile;
    qWarning() << "Error replacing file with hardlink";
    qWarning() << "Error code " << ret;
    return false;
  }
  return true;
}

int main(int argc, char** argv)
{  
  if (argc != 2) 
  {
    qCritical() << "Error: Wrong number of arguments, 1 expected";
    exit (1);
  }
  map <QByteArray, ino_t> HashToInode;
  map <ino_t,ino_t> InodeMap;
  map <ino_t,QString> InodeToFileName;
  uint NumDuplicates = 0;
  uint NumHardLinks = 0;
  uint NumOrphanedHardlinks = 0;
  unsigned long NumFiles = 0;

  WorkingPath.setPath(argv[1]);
  if (!WorkingPath.exists())
  {
    qCritical() << "Directory " << WorkingPath.absolutePath() << " does not exist, exiting ...";
    exit (2);
    
  }
  qDebug() << "Calling duplicate replace on diretory: " << WorkingPath.absolutePath(); 
  QDir::Filters filter = QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot;
  QDirIterator Iterator(WorkingPath.path(),QStringList(),filter,QDirIterator::Subdirectories);
  while (Iterator.hasNext())
  {
    NumFiles++;
    if (NumFiles % 100 == 0) 
    {
      qDebug() << "Processing file number: " << NumFiles << ". Found " << NumDuplicates << " duplicates, " << NumHardLinks << " hardlinks and " << NumOrphanedHardlinks << "orphaned hardlinks so far.";
    }
    QString _Filename = Iterator.next();     
    ino_t _Inode = GetFileInode(_Filename);
    if (_Inode == 0) continue;
    /// check if _Inode is in InodeMap
    /// if so there are to possibilities. 
    /// First: The Inode points to space, which we want to keep (first == second)
    /// Second: The File with this inode has been treated as a duplicate in previous files.
    /// In this case link this file to the same "base-inode"
    auto InodeMap_Itr = InodeMap.find(_Inode);
    if (InodeMap_Itr != InodeMap.end())
    {
      /// if Inode is the End Inode, this is a already hardlinked file
      if (InodeMap_Itr->first == InodeMap_Itr->second)
      {
	qDebug() << "Real hardlink: " << _Filename << " --> " << InodeToFileName[InodeMap_Itr->second]; 	
	NumHardLinks++;	
      }
      /// if not this means the inode this file points to has been de-duplicated previously (orphaned)
      else
      {
	QString TargetFileName = InodeToFileName[InodeMap_Itr->second];
	qDebug() << "Orphaned hardlink: " << _Filename <<  " --> " << TargetFileName;
	ReplaceWithHardLink(_Filename,TargetFileName);
	NumOrphanedHardlinks++;
      }      
    }    
    /// if unknown inode -> see if this file in a duplicate or a new file
    else
    {      
      QByteArray _MD5 = GetHashSum(_Filename); 
      auto TargetMD5_Inode = HashToInode.find(_MD5);
      /// Known hash but unknown Inode -> Point this file to the base inode.
      /// Also mark this inode as being a duplicate inode to the base inode.
      if (TargetMD5_Inode != HashToInode.end())
      {
	QString TargetFileName = InodeToFileName[TargetMD5_Inode->second];
	bool ok = ReplaceWithHardLink(_Filename,TargetFileName);
        if (!ok) continue;
	qDebug() << "Duplicate: " << _Filename << " --> " << TargetFileName;
	InodeMap[_Inode] = TargetMD5_Inode->second;
	NumDuplicates++;	
      } 
      /// Unknown Hash and unknown Inode --> add as new file
      else
      {
	HashToInode[_MD5] = _Inode;
	InodeToFileName[_Inode] = _Filename;
	InodeMap[_Inode] = _Inode;
      }
    }

  }   
  qDebug() << "Finished replacing duplicates";
  qDebug() << "Processed " << NumFiles << " files in total";
  qDebug() << "Found " << NumDuplicates << " real duplicates (no hardlinks)";  
  qDebug() << "Found " << NumHardLinks << " duplicates which were already hardlinks";
  qDebug() << "Found " << NumOrphanedHardlinks << " duplicates which were depracted hardlinks (orphans)";
  return 0;
}
