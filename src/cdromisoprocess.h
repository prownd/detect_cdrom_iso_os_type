/***
 *author:hanjinpeng
 *email:545751287@qq.com
 */
#ifndef CDROMISOPROCESS_H
#define CDROMISOPROCESS_H
#include<QtCore>
#include"cdromisoinfo.h"


#define UNUSED(A) A

struct OSContent
{
    QString name;
    QString os_version;
    QString kernel;
    bool is_new_kernel;
};

struct CdromIsoOSInfo{
     QString name; //media or iso file  name
     QString volumeLabel; //volume label
     unsigned long long int size; // size
     OS_TYPE os_type;  //os type,linux or window or other
     QString os_family; //os distribution  ubuntu,microsoft ...
     QString os_version; // os distribution version number
     QString os_name;  //os name ,eg:ubuntu 10.04
     bool bootable;  // is boot or not
     QString kernel;//kernel version
     bool is_new_kernel;//new kernel ,after kernel 2.6.18, other else old kernel;
     QString description;//
     QString platform; //hardware platform x86 ppc etc...
};

class CdromIsoProcess
{
public:
    CdromIsoProcess();
    ~CdromIsoProcess();
    CdromIsoOSInfo* getCdromIsoInfo(QString path="/dev/cdrom");
    void detectOsType(QFileInfo fileinfo,CdromIsoInfo * info);
    void mountDetect(QString path,CdromIsoInfo* info);
    void processMatchInfo(CdromIsoInfo *info);
    bool readDir(const QString path,CdromIsoInfo * info);
    void callSystemCmd(QString cmd);
    QString& convertName(QString &filename);
    void initOSList();
    bool isNewKernel(QString version);
    void transferInfo(CdromIsoOSInfo *mediaInfo,CdromIsoInfo * info);
private:
    QString strTmp1;
    QString strTmp2;
    bool strTmpIsSet;
    QList<OSContent*> osContentList;
};

#endif // CDROMISOPROCESS_H
