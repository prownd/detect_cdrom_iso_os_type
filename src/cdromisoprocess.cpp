/***
 *author:hanjinpeng
 *email:545751287@qq.com
 */
#include "cdromisoprocess.h"
#include "mediadetglobal.h"
#include<QtCore/qmath.h>
extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/unistd.h>
#include <sys/statfs.h>
#include <sys/mount.h>
#include <magic.h>
#include <fcntl.h>
#include <dirent.h>
}
int detect_media(char *path,CdromIsoInfo *info);
CdromIsoProcess::CdromIsoProcess()
{
    strTmpIsSet=false;
    initOSList();
}

CdromIsoProcess::~CdromIsoProcess()
{
    for(int i=0;i<osContentList.count();i++)
    {
        delete osContentList[i];
    }
}

CdromIsoOSInfo* CdromIsoProcess::getCdromIsoInfo(QString path)
{
    CdromIsoInfo *info=new CdromIsoInfo;
    //convertName(path);
    QByteArray qba = path.toLocal8Bit();
    char * pathfile = qba.data();
    detect_media(pathfile,info);
    mountDetect(path,info);
    processMatchInfo(info);
    CdromIsoOSInfo * mediaInfo=new CdromIsoOSInfo;
    transferInfo(mediaInfo,info);
    delete info;
    info=NULL;
    return mediaInfo;
}


void CdromIsoProcess::detectOsType(QFileInfo fileinfo,CdromIsoInfo * info)
{
    if(fileinfo.suffix()=="rpm")
    {
        QString pattern_rpm_gpg_key("RPM-GPG-KEY-([a-zA-Z]+)-([0-9]+)");
        QString pattern_rpm_kernel("[a-zA-Z0-9]*kernel-([0-9]+\\.[0-9]+\\.[0-9]+)-([0-9]+)");
        QString pattern_rpm_kernel_suse("kernel-default-([0-9]+\\.[0-9]+\\.[0-9]+)");
        QString pattern_rpm_release("([a-zA-Z]+)-release-([0-9]+)[-\\.]([0-9]+)[-\\.]");
        QString pattern_rpm_release_redhat("([a-zA-Z]+)-release-server-[a-zA-Z0-9]+-([0-9]+\\.[0-9]+)\\.");
        QRegExp regexp_rpm_gpg_key(pattern_rpm_gpg_key);
        QRegExp regexp_rpm_kernel(pattern_rpm_kernel);
        QRegExp regexp_rpm_kernel_suse(pattern_rpm_kernel_suse);
        QRegExp regexp_rpm_release(pattern_rpm_release);
        QRegExp regexp_rpm_release_redhat(pattern_rpm_release_redhat);
        int pos=-1;

        pos= fileinfo.fileName().indexOf(regexp_rpm_gpg_key);
        if ( pos >= 0 )
        {
            strcpy(info->os_family,regexp_rpm_gpg_key.cap(1).toLocal8Bit().data());
            strcpy(info->os_version,regexp_rpm_gpg_key.cap(2).toLocal8Bit().data());
            sprintf(info->os_name,"%s %s",info->os_family,info->os_version);
            info->os_type=LINUX;
        }
        pos= fileinfo.fileName().indexOf(regexp_rpm_kernel);
        if ( pos >= 0 )
        {
            strcpy(info->kernel,regexp_rpm_kernel.cap(1).toLocal8Bit().data());
            info->is_new_kernel=isNewKernel(regexp_rpm_kernel.cap(1));
            info->os_type=LINUX;
        }
        pos= fileinfo.fileName().indexOf(regexp_rpm_kernel_suse);
        if ( pos >= 0 )
        {
            strcpy(info->kernel,regexp_rpm_kernel_suse.cap(1).toLocal8Bit().data());
            info->is_new_kernel=isNewKernel(regexp_rpm_kernel_suse.cap(1));
            info->os_type=LINUX;
        }
        pos= fileinfo.fileName().indexOf(regexp_rpm_release);
        if ( pos >= 0 )
        {
            strcpy(info->os_family,regexp_rpm_release.cap(1).toLocal8Bit().data());
            strcpy(info->os_version,(regexp_rpm_release.cap(2)+"."+regexp_rpm_release.cap(3)).toLocal8Bit().data());
            sprintf(info->os_name,"%s %s",info->os_family,info->os_version);
            info->os_type=LINUX;
        }
        pos= fileinfo.fileName().indexOf(regexp_rpm_release_redhat);
        if ( pos >= 0 )
        {
            strcpy(info->os_family,regexp_rpm_release_redhat.cap(1).toLocal8Bit().data());
            strcpy(info->os_version,regexp_rpm_release_redhat.cap(2).toLocal8Bit().data());
            sprintf(info->os_name,"%s %s",info->os_family,info->os_version);
            info->os_type=LINUX;
        }
    }

    if(fileinfo.fileName().endsWith("deb") or fileinfo.fileName().endsWith("DEB"))
    {
        QString pattern_deb_kernel("[a-zA-Z0-9]*linux-image-([0-9]+\\.[0-9]+\\.[0-9]+)-([0-9]+)");
        QRegExp regexp_deb_kernel(pattern_deb_kernel);
        int pos=-1;

        pos= fileinfo.fileName().indexOf(regexp_deb_kernel);
        if ( pos >= 0 )
        {
            strcpy(info->kernel,regexp_deb_kernel.cap(1).toLocal8Bit().data());
            info->is_new_kernel=isNewKernel(regexp_deb_kernel.cap(1));
            info->os_type=LINUX;
        }
    }

    //release  version
    if(fileinfo.fileName().toLower()=="release")
    {
        QFile file(fileinfo.filePath());
        if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            qDebug()<<"Can't open the file!"<<endl;
        }else{
            QTextStream in(&file);
            QString pattern_keyvalue("([a-zA-Z]*)\\s*:\\s*([a-zA-Z0-9\\.]*)");
            QRegExp regexp_keyvalue(pattern_keyvalue);
            int pos=-1;
            while( !in.atEnd())
            {
                QString line = in.readLine();
                pos= line.indexOf(regexp_keyvalue);
                if ( pos >= 0 )
                {
                    if(regexp_keyvalue.cap(1).toLower()=="label")
                    {
                        strcpy(info->os_family,regexp_keyvalue.cap(2).toLower().toLocal8Bit().data());
                        info->os_type=LINUX;
                        strTmp1=regexp_keyvalue.cap(2);
                    }
                    if(regexp_keyvalue.cap(1).toLower()=="version")
                    {
                        strcpy(info->os_version,regexp_keyvalue.cap(2).toLower().toLocal8Bit().data());
                        strTmp2=regexp_keyvalue.cap(2);
                    }
                    if(!strTmp1.isEmpty() && !strTmp2.isEmpty() && !strTmpIsSet)
                    {
                        strcpy(info->os_name,(strTmp1+" "+strTmp2).toLocal8Bit().data());
                        strTmpIsSet=true;
                    }
                }
            }
        }
    }

    //kernel
    if(fileinfo.fileName().toLower()=="filesystem.manifest")
    {
        QFile file(fileinfo.filePath());
        if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            qDebug()<<"Can't open the file!"<<endl;
        }else{
            QTextStream in(&file);
            QString pattern_manifesta("[a-zA-Z0-9]*linux-image-([0-9]+\\.[0-9]+\\.[0-9]+)-([0-9]+)");
            QRegExp regexp_manifesta(pattern_manifesta);
            int pos=-1;
            while( !in.atEnd())
            {
                QString line = in.readLine();
                pos= line.indexOf(regexp_manifesta);
                if ( pos >= 0 )
                {
                    strcpy(info->kernel,regexp_manifesta.cap(1).toLocal8Bit().data());
                    info->is_new_kernel=isNewKernel(regexp_manifesta.cap(1));
                    info->os_type=LINUX;
                }
            }
        }
    }

    //pkglist
    if(fileinfo.fileName().contains("pkglist."))
    {
        QFile file(fileinfo.filePath());
        if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            qDebug()<<"Can't open the file!"<<endl;
        }else{
            QTextStream in(&file);
            QString pattern_release("([a-zA-Z0-9]*)-release-[a-zA-Z]*([0-9]+\\.[0-9]+)");
            QRegExp regexp_release(pattern_release);

            QString pattern_kernel("[a-zA-Z0-9]*linux-([0-9]+\\.[0-9]+\\.[0-9]+)-([0-9]+)");
            QRegExp regexp_kernel(pattern_kernel);
            int pos=-1;
            while( !in.atEnd())
            {
                QString line = in.readLine();
                pos= line.indexOf(regexp_release);
                if ( pos >= 0 )
                {
                    strcpy(info->os_family,regexp_release.cap(1).toLocal8Bit().data());
                    strcpy(info->os_version,regexp_release.cap(2).toLocal8Bit().data());
                    sprintf(info->os_name,"%s %s",info->os_family,info->os_version);
                    info->os_type=LINUX;
                }

                pos= line.indexOf(regexp_kernel);
                if ( pos >= 0 )
                {
                    strcpy(info->kernel,regexp_kernel.cap(1).toLocal8Bit().data());
                    info->is_new_kernel=isNewKernel(regexp_kernel.cap(1));
                    info->os_type=LINUX;
                }
            }
        }
    }

    //gho window
    QString tmpfilename= fileinfo.fileName();
    if(info->bootable && info->os_type != LINUX &&( tmpfilename.toLower().endsWith(".exe") || tmpfilename.toLower().endsWith(".gho") ||tmpfilename.toLower().endsWith(".com")))
    {
        info->os_type=WINDOW;
    }
}

void CdromIsoProcess::mountDetect(QString path,CdromIsoInfo * info)
{
    QDir file;
    QString mountDir = QString("/mnt/ismbcloud_mount_detect_info%1").arg(qrand()%1000000, 6, 10, QChar('0'));
    file.mkdir(mountDir);
    QByteArray s = path.toLocal8Bit();
    const char * source = s.data();
    QByteArray s1 = mountDir.toLocal8Bit();
    const char * mountDir_c = s1.data();
    QString mountCmdLine=QString("mount -t iso9660  -o loop %1  %2").arg(source,mountDir);
    QString umountCmdLine=QString("umount -f %1").arg(mountDir);
    bool isIsoFile=true;
    if(path.endsWith(".iso") || path.endsWith(".ISO"))
        isIsoFile=true;
    else
        isIsoFile=false;
    if(isIsoFile)
    {
          callSystemCmd(mountCmdLine);
    }else{
        if(mount(source, mountDir_c, "iso9660", MS_RDONLY, "") == -1)
        {
            perror("mount failed:");
            file.rmdir(mountDir);
        }
    }
    readDir(mountDir,info);
    if(isIsoFile)
    {
        callSystemCmd(umountCmdLine);

    }else
    {
        if(umount2(mountDir_c, MNT_DETACH) == -1)
        {
             perror("umount:");
        }
    }
    file.rmdir(mountDir);
}

void CdromIsoProcess::processMatchInfo(CdromIsoInfo *info)
{
    if(strlen(info->os_family)!=0 && strlen(info->kernel)==0)
    {
        QString os_family_str=info->os_family;
        for(int i=0;i<osContentList.count();i++)
        {
            if(os_family_str.contains(osContentList[i]->name,Qt::CaseInsensitive))
            {
                strcpy(info->kernel,osContentList[i]->kernel.toLocal8Bit().data());
                info->is_new_kernel=isNewKernel(osContentList[i]->kernel);
            }
        }
    }
}

bool CdromIsoProcess::readDir(const QString  path,CdromIsoInfo * info)
{
    QDir dir(path);
    if(!dir.exists())
        return false;
    dir.setFilter(QDir::Dirs|QDir::Files);
    //dir.setSorting(QDir::DirsFirst);
    dir.setSorting(QDir::DirsLast);
    QFileInfoList list = dir.entryInfoList();
    int i=0;
    int nFiles=0;
    do{
        QFileInfo fileInfo = list.at(i);
        if(fileInfo.fileName()=="."|fileInfo.fileName()=="..")
        {
           i++;
           continue;
        }
//        bool bisSymLink=fileInfo.isSymLink();
//        if(bisSymLink)
//        {
//            i++;
//            continue;
//        }

        bool bisDir=fileInfo.isDir();
        if(bisDir)
        {
           nFiles++;
           readDir(fileInfo.filePath(),info);
        }
        else{
           nFiles++;
           detectOsType(fileInfo,info);
        }
        i++;
    }while(i<list.size());
    return true;
}

void CdromIsoProcess::callSystemCmd(QString cmd)
{
    char buf[256];
    strcpy(buf,cmd.simplified().toLatin1().data());
    system(buf);
}

QString& CdromIsoProcess::convertName(QString &filename)
{
    if(filename.contains("/dev/cdrom"))
        filename="/dev/sr0";
    return filename;
}

void CdromIsoProcess::initOSList()
{
    QString strlist[]={
                   "ubuntu,7.10,2.6.23","ubuntu,8.04,2.6.24","ubuntu,8.10,2.6.27","ubuntu,9.04,2.6.28"
                  ,"ubuntu,9.10,2.6.31","ubuntu,10.04,2.6.32","ubuntu,10.10,2.6.35","ubuntu,11.04,2.6.38"
                  ,"ubuntu,11.10,3.0.0","ubuntu,12.04,3.2.0","ubuntu,12.10,3.5.0","ubuntu,13.04,3.8.0"
                  ,"ubuntu,13.10,3.11.0","ubuntu,14.04,3.13.0","ubuntu,14.10,3.16.0"
                  ,"debian,4.0,2.6.18","debian,5.0,2.6.26","debian,6.0,2.6.32","debian,7.0,3.2.0"
                  //,"openSUSE,10.2,2.6.18","openSUSE,10.3,2.6.22"
                  //,"openSUSE,11.0,2.6.25","openSUSE,11.1,2.6.27","openSUSE,11.2,2.6.31","openSUSE,11.3,2.6.34","openSUSE,11.4,2.6.37"
                  //,"openSUSE,12.1,3.1.0","openSUSE,12.2,3.4.6","openSUSE,12.3,3.7.10"
                  //,"openSUSE,13.1,3.11.6","openSUSE,13.2,3.16.6"
                 };

    for(int i=0;i<sizeof(strlist)/sizeof(QString);i++)
    {
            OSContent * os=new OSContent;
            QStringList sl;
            sl=strlist[i].split(",");
            os->name=sl[0];
            os->os_version=sl[1];
            os->kernel=sl[2];
            os->is_new_kernel=isNewKernel(sl[2]);
        osContentList.append(os);
    }
}

bool CdromIsoProcess::isNewKernel(QString version)
{
    QStringList verList=version.split(".");
    int versionInt=0;
    for(int i=0;i<verList.count();i++)
        versionInt+=verList[i].toInt()*qPow(100,2-i);
    if(versionInt>=NEWKERNELBASE)
        return true;
    else
        return false;

}

void CdromIsoProcess::transferInfo(CdromIsoOSInfo *mediaInfo, CdromIsoInfo *info)
{
    if(mediaInfo!=NULL && info!=NULL)
    {
        mediaInfo->name=info->name;
        mediaInfo->volumeLabel=info->volumeLabel;
        mediaInfo->size=info->size;
        mediaInfo->os_type=info->os_type;
        mediaInfo->os_family=info->os_family;
        mediaInfo->os_version=info->os_version;
        mediaInfo->os_name=info->os_name;
        mediaInfo->bootable=info->bootable;
        mediaInfo->kernel=info->kernel;
        mediaInfo->is_new_kernel=info->is_new_kernel;
        mediaInfo->description=info->description;
        mediaInfo->platform=info->platform;
    }
}
