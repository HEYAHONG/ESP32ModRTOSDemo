#include "fstream"
#include "dirent.h"
#include "unistd.h"
#include "sys/stat.h"
#include  <string>
#include  <vector>
#include "RC_internal.h"
#include "stdlib.h"
#include "stdint.h"

std::vector<std::string> filelist;
void listdir(std::string dirname,std::string root)
{
#ifdef WIN32
#endif // WIN32
    DIR *dir=NULL;
    if(dirname.empty())
    {
        dir=opendir(root.c_str());
    }
    else
    {
        dir=opendir((root+dirname).c_str());
    }
    if(dir!=NULL)
    {
        struct dirent * dirnode=NULL;
        while((dirnode=readdir(dir)))
        {
            std::string name=dirname+std::string(dirnode->d_name);
            struct stat state= {0};
            if(0!=stat((root+name).c_str(),&state))
            {
                continue;
            }
            if(state.st_mode & S_IFREG)
            {
                if(name.empty())
                {
                    continue;
                }
                filelist.push_back(name);
                printf("file: %s\n",name.c_str());
            }
            if(state.st_mode & S_IFDIR)
            {
                if(std::string(dirnode->d_name)=="..")
                {
                    //不向上遍历
                    continue;
                }
                if(std::string(dirnode->d_name)==".")
                {
                    //不遍历本目录
                    continue;
                }
                listdir(name+"/",root);
            }
        }
        closedir(dir);
    }

}

std::vector<RC_Info_t> RC_Info_List;
void fsgen(std::string filename,std::string root)
{
    std::fstream RC_FS;
    RC_FS.open(filename.c_str(),std::ios_base::out );
    if(RC_FS.is_open())
    {
        RC_FS.clear();

        {
            //包含头文件
            RC_FS<<"#include \"RC_internal.h\"\n";
        }

        {
            size_t current_rc_data=0;
            size_t current_rc_name=0;
            RC_Info_t RC_Info_item= {0};
            {
                //RC_Data
                RC_FS << "\nconst unsigned char RC_Data[] =\n";
                RC_FS << "{\n";
            }
            for(auto it=filelist.begin(); it != filelist.end(); it++)
            {
                if(it==filelist.end())
                {
                    break;
                }
                std::string filename=(*it);
                std::fstream File;
                File.open((root+filename).c_str(),std::ios_base::in|std::ios_base::binary);
                if(File.is_open())
                {
                    RC_Info_item.data_offset=current_rc_data;
                    RC_Info_item.name_offset=current_rc_name;
                    {
                        RC_Info_item.name_size=(filename.length()+1);
                        current_rc_name+=RC_Info_item.name_size;
                    }
                    {
                        RC_FS << "//" << filename.c_str() << "\n";
                    }
                    while(!File.eof())
                    {
                        //写RC_Data
                        unsigned char buff[64]= {0};
                        size_t readbytes=File.readsome((char *)buff,sizeof(buff));
                        if(readbytes==0)
                        {
                            break;
                        }
                        {
                            for(size_t i=0; i<readbytes; i++)
                            {
                                current_rc_data++;
                                char temp[10]= {0};
                                sprintf(temp,"0x%02X,",(uint32_t)buff[i]);
                                RC_FS<< temp;
                            }
                        }
                        RC_FS << "\n";
                    }


                    {
                        RC_Info_item.data_size=current_rc_data-RC_Info_item.data_offset;
                        RC_Info_List.push_back(RC_Info_item);
                    }

                    {
                        current_rc_data++;
                        RC_FS << "0x00,\n";
                    }

                    File.close();
                }
            }
            {
                RC_FS << "0x00\n};\n";
            }

            {

                RC_FS << "\nconst unsigned char RC_Name[] =\n";
                RC_FS << "{\n";
            }
            for(auto it=filelist.begin(); it!=filelist.end(); it++)
            {
                if(it==filelist.end())
                {
                    break;
                }
                std::string filename=(*it);
                if(access((root+filename).c_str(),R_OK)==0)
                {
                    {
                        RC_FS << "//" <<  filename.c_str() << "\n";
                    }
                    unsigned char * str=(unsigned char *) filename.c_str();
                    for(size_t i=0; i< filename.length(); i++)
                    {

                        current_rc_data++;
                        char temp[10]= {0};
                        sprintf(temp,"0x%02X,",(int)str[i]);
                        RC_FS<< temp;

                    }
                    {
                        RC_FS << "0x00,\n";
                    }
                }
            }
            {
                RC_FS << "0x00\n};\n";
            }

            {
                RC_FS << "\nconst RC_Info_t RC_Info[] =\n";
                RC_FS << "{\n";
            }
            for(auto it=RC_Info_List.begin(); it!=RC_Info_List.end(); it++)
            {
                char temp[256]={0};
                sprintf(temp,"{%lu,%lu,%lu,%lu}\n",(*it).data_offset,(*it).data_size,(*it).name_offset,(*it).name_size);
                RC_FS<< temp;
                if((it+1)!=RC_Info_List.end())
                {
                    RC_FS << ",";
                }
                {
                    RC_FS << "\n";
                }
            }
            {
                RC_FS << "};\n";
            }

            {
                char temp[256]={0};
                sprintf(temp,"\nconst size_t    RC_Info_Size= %lu;\n",RC_Info_List.size());
                RC_FS << temp;
            }
        }

        RC_FS.close();
    }
}

int main(int argc,char *argv[])
{
    if(argc<3)
    {
        return -1;
    }

    setbuf(stdout,NULL);

    {
        printf("root:%s\nRC_fs.c:%s\n",argv[1],argv[2]);
    }

    listdir("",std::string(argv[1])+"/");

    fsgen(argv[2],std::string(argv[1])+"/");

    return 0;


}


