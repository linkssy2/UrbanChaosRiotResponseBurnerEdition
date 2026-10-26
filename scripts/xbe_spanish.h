#pragma once
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace trial_spanish {
inline std::vector<std::string> ReadFiles(const std::filesystem::path& root){
        std::vector<std::string> found;
        std::ifstream manifest(root/"manifest.txt");
        std::string name;uintmax_t size;
        while(manifest>>name){
            if(!(manifest>>size))return {};
            if(found.size()>=27||(name.size()!=8&&name.size()!=9)||name.substr(name.size()-7)!="-01.ent"||
               name[0]<'0'||name[0]>'9'||(name.size()==9&&(name[1]<'0'||name[1]>'9'))||size<200||size>64*1024*1024)return std::vector<std::string>{};
            std::error_code ec;
            const auto path=root/name;
            if(!std::filesystem::is_regular_file(std::filesystem::symlink_status(path,ec))||ec||
               std::filesystem::file_size(path,ec)!=size||ec)return std::vector<std::string>{};
            for(const auto& previous:found)if(previous==name)return std::vector<std::string>{};
            found.push_back(name);
        }
        return manifest.eof()&&found.size()==27?found:std::vector<std::string>{};
}
inline const std::vector<std::string>& Files(){
    static const auto files=ReadFiles("translations/es-zarf");
    return files;
}
inline bool Available(){return !Files().empty();}
inline bool VoiceName(const std::string& name){
    if(name.empty()||name.size()>80)return false;
    for(char c:name)if(!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='.'||c=='/'))return false;
    if(name.find("..")!=std::string::npos)return false;
    if(name.rfind("movies/",0)==0)return name.find('/',7)==std::string::npos&&name.size()>11&&name.substr(name.size()-4)==".xmv";
    if(name.find('/')!=std::string::npos)return false;
    const auto dot=name.find('.');
    if(dot==std::string::npos)return false;
    const bool archive=name.substr(dot)==".ent";
    const auto number=archive&&dot>=3&&name.substr(dot-3,3)=="-01"?name.substr(0,dot-3):name.substr(0,dot);
    if((!archive&&name.substr(dot)!=".saf")||number.empty()||number.size()>2)return false;
    for(char c:number)if(c<'0'||c>'9')return false;
    return !archive||dot==number.size()+3;
}
inline std::vector<std::string> ReadVoiceFiles(const std::filesystem::path& root){
    std::error_code ec;
    if(!std::filesystem::is_directory(std::filesystem::symlink_status(root,ec))||ec)return {};
    const auto index=root/"manifest.txt";
    if(!std::filesystem::is_regular_file(std::filesystem::symlink_status(index,ec))||ec||
       std::filesystem::file_size(index,ec)>16384||ec)return {};
    std::ifstream manifest(index);std::string magic;unsigned version=0,count=0;
    if(!(manifest>>magic>>version>>count)||magic!="UCSPANISHVOICES"||version!=1||count<28||count>128)return {};
    std::vector<std::string> found;unsigned archives=0,streams=0;
    for(unsigned i=0;i<count;++i){
        std::string name;uintmax_t size=0;
        if(!(manifest>>name>>size)||!VoiceName(name)||size<1||size>uintmax_t(2)*1024*1024*1024)return {};
        if(name.rfind("movies/",0)==0&&(!std::filesystem::is_directory(std::filesystem::symlink_status(root/"movies",ec))||ec))return {};
        const auto path=root/name;
        if(!std::filesystem::is_regular_file(std::filesystem::symlink_status(path,ec))||ec||std::filesystem::file_size(path,ec)!=size||ec)return {};
        for(const auto& previous:found)if(previous==name)return {};
        archives+=name.size()>4&&name.substr(name.size()-4)==".ent";
        streams+=name.size()>4&&name.substr(name.size()-4)==".saf";
        found.push_back(name);
    }
    std::string extra;
    if(manifest>>extra||!manifest.eof()||archives!=27||!streams)return {};
    return found;
}
inline const std::vector<std::string>& VoiceFiles(){
    static const auto files=ReadVoiceFiles("translations/es-voices");return files;
}
inline bool VoicesAvailable(){return !VoiceFiles().empty();}
inline std::string Override(std::string name){
    for(auto& c:name){if(c>='A'&&c<='Z')c+=32;else if(c=='\\')c='/';}
    for(const auto& file:VoiceFiles())if(file==name)return (std::filesystem::path("translations/es-voices")/file).string();
    for(const auto& file:Files())if(file==name)return (std::filesystem::path("translations/es-zarf")/file).string();
    return {};
}
}
