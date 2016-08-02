/***
 *author:hanjinpeng
 *email:545751287@qq.com
 */
#ifndef CDROMISOINFO_H
#define CDROMISOINFO_H
//new kernel 2.6.18 later
#define NEWKERNELBASE  (2*10000+6*100+18)
enum OS_TYPE { LINUX=1, WINDOW, OTHER};
typedef struct CdromIsoInfo{
     char name[512]; //media or iso file  name
     char volumeLabel[512]; //volume label
     unsigned long long int size; // size
     OS_TYPE os_type;  //os type,linux or window or other
     char os_family[512]; //os distribution  ubuntu,microsoft ...
     char os_version[512]; // os distribution version number
     char os_name[512];  //os name
     bool bootable;  // is boot or not
     char kernel[512];//kernel
     bool is_new_kernel;//new kernel ,after kernel 2.6.18, other else old kernel;
     char description[512];//
     char platform[512]; //hardware platform x86 ppc etc...
}CdromIsoInfo;

#endif // CDROMISOINFO_H
