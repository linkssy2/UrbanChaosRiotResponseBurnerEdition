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
inline std::string Override(std::string name){
    for(auto& c:name)if(c>='A'&&c<='Z')c+=32;
    for(const auto& file:Files())if(file==name)return (std::filesystem::path("translations/es-zarf")/file).string();
    return {};
}
}
