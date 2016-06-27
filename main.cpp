#include <QCoreApplication>
#include "cdromisoprocess.h"
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    CdromIsoProcess info;
    CdromIsoOSInfo *fileInfo;
	if(argc>=2){
		for(int i=1;i<argc;i++){
			//fileInfo=info.getCdromIsoInfo("/root/CentOS-6.5-x86_64-bin-DVD1.iso");
			fileInfo=info.getCdromIsoInfo(argv[i]);
			qDebug()<<"name:"<<fileInfo->name;
			qDebug()<<"bootable:"<<fileInfo->bootable;
			qDebug()<<"size:"<<fileInfo->size;
			qDebug()<<"voumelabel:"<<fileInfo->volumeLabel;
			qDebug()<<"os_type:"<<fileInfo->os_type;
			qDebug()<<"os_family:"<<fileInfo->os_family;
			qDebug()<<"os_version:"<<fileInfo->os_version;
			qDebug()<<"os_name:"<<fileInfo->os_name;
			qDebug()<<"os kernel:"<<fileInfo->kernel;
			qDebug()<<"os is new kernel:"<<fileInfo->is_new_kernel;
		}
	}
	return 0;//a.exec();
}
