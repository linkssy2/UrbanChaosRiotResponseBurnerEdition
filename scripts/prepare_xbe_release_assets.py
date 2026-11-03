"""Setup-selected extracted/direct-ISO sources and read-only prepared archives."""
from pathlib import Path


def function(source, signature):
    start=source.index(signature)
    return source[start:source.index('\n}',start)+2]


def once(source,old,new):
    assert source.count(old)==1,old
    return source.replace(old,new,1)


RELEASE_ASSETS=r'''fs::path FindAssets() {
    // Setup supplies the prepared directory for either extracted or direct ISO data.
    // Never search the executable's ancestors or the current working directory.
    const DWORD size=GetEnvironmentVariableW(L"UCRR_GAME_ASSETS",nullptr,0);
    if(!size||size>32768)return {};
    std::wstring value(size,L'\0');
    const DWORD copied=GetEnvironmentVariableW(L"UCRR_GAME_ASSETS",value.data(),size);
    if(!copied||copied>=size)return {};
    value.resize(copied);
    const fs::path directory(value);
    std::error_code ec;
    if(!directory.is_absolute()||!fs::is_directory(directory,ec)||ec)return {};
    return directory;
}'''

RELEASE_GAME=r'''bool FindGame(GameFiles& game) {
    const fs::path assets=FindAssets();
    if(assets.empty()) {
        MessageBoxW(nullptr,L"Run Setup to select and install your game files, then start the game from its launcher.",
                    kGame.short_title,MB_ICONERROR);
        return false;
    }
    std::vector<uint8_t> xbe;
    if(!ReadHostFile(assets / IMAGE_XBE,xbe)) {
        MessageBoxW(nullptr,L"The installed game files are missing the game executable. Run Setup to repair the installation.",
                    kGame.short_title,MB_ICONERROR);
        return false;
    }
    const std::string sha=Sha1(xbe);
    // The second accepted image differs only in its verified certificate. Both
    // complete-file hashes are intentional; no unknown revision can continue.
    const bool supported=sha=="fb9bbe0028b52f60a1b57b35b873093f334c2ce0"||
                         sha=="63805aa38ca20ac1db0712cba47dbcf38406ac58";
    LOGR("Game: %s supplied by Setup, sha1 %s%s",IMAGE_XBE,sha.c_str(),supported?"":", unsupported revision");
    if(!supported) {
        MessageBoxW(nullptr,L"These game files are not a supported revision, or the game executable has changed. Run Setup with a supported copy of Urban Chaos: Riot Response.",
                    kGame.short_title,MB_ICONERROR);
        return false;
    }
    std::unique_ptr<kernel::Disc> disc;
    const DWORD iso_size=GetEnvironmentVariableW(L"UCRR_GAME_ISO",nullptr,0);
    if(iso_size) {
        std::wstring value;
        DWORD copied=0;
        if(iso_size<=32768) {
            value.resize(iso_size);
            copied=GetEnvironmentVariableW(L"UCRR_GAME_ISO",value.data(),iso_size);
            if(copied&&copied<iso_size)value.resize(copied);
        }
        const fs::path iso(value);
        std::error_code ec;
        std::string error;
        std::vector<uint8_t> disc_xbe;
        disc=std::make_unique<kernel::Disc>();
        bool valid=copied&&copied<iso_size&&iso.is_absolute()&&fs::is_regular_file(iso,ec)&&!ec;
        valid=valid&&disc->Open(iso,error);
        const auto* entry=valid?disc->Find(IMAGE_XBE):nullptr;
        // Check the directory's size before allocating: a bad ISO must not turn
        // its untrusted file table into an arbitrary allocation or a fallback.
        valid=entry&&!entry->dir&&entry->size==xbe.size()&&
              disc->Load(IMAGE_XBE,disc_xbe)&&Sha1(disc_xbe)==sha;
        if(!valid) {
            MessageBoxW(nullptr,L"The selected game ISO is missing, unreadable, or does not match the installed game revision. Run Setup to select the correct ISO or import the game files.",
                        kGame.short_title,MB_ICONERROR);
            return false;
        }
        LOGR("Game: reading original game data directly from the ISO selected by Setup");
    }
    game={assets,std::move(xbe)};
    kernel::MountDisc(std::move(disc));
    return true;
}'''


PREPARED_FILES = ('coopcharacters.lst', 'coopinjection.lst',
                  'coopturrets03.lst', 'coopturrets08.lst')


def patch_files(source):
    """Preserve language priority; only four generated root files override an ISO.

    The original archives remain on the disc. Host consumers of ResolvePath and
    guest file APIs see the same prepared files, including directory metadata.
    Save partitions keep their original write behavior.
    """
    marker='// Setup-created co-op archives are the only local disc additions.'
    if marker in source:
        return source
    names=', '.join('"'+name+'"' for name in PREPARED_FILES)
    source=once(source,'struct File : Object {',
        marker+'\nconstexpr const char* kPreparedArchives[] = {'+names+'};\n\n'
        'bool IsPreparedArchive(const std::vector<std::string>& parts) {\n'
        '    if(parts.size()!=1)return false;\n'
        '    const auto name=Upper(parts[0]);\n'
        '    for(const auto* prepared:kPreparedArchives)if(name==Upper(prepared))return true;\n'
        '    return false;\n}\n\n'
        'struct File : Object {\n    bool read_only = false;')
    source=once(source,'    const Disc::Entry* entry = nullptr;  // there: the file or folder, null when missing\n',
        '    const Disc::Entry* entry = nullptr;  // there: the file or folder, null when missing\n'
        '    bool read_only = false;             // local data mounted as read-only DVD content\n')
    disc='    if (best->first == kCdRom)\n        if (const Disc* disc = MountedDisc()) return {{}, true, disc->Find(parts)};\n'
    language='''    if(best->first==kCdRom&&!parts.empty()&&parts.size()<=2&&trial_language::Active()==4){
        const auto translated=trial_spanish::Override(parts.size()==1?parts[0]:parts[0]+"/"+parts[1]);
        if(!translated.empty())return {translated};
    }
'''
    source=once(source,disc+language,
        language.replace('return {translated};','return {translated,false,nullptr,MountedDisc()!=nullptr};')+
        '''    if (best->first == kCdRom) {
        if (const Disc* disc = MountedDisc()) {
            if(IsPreparedArchive(parts))return {MatchCase(best->second,parts).string(),false,nullptr,true};
            return {{},true,disc->Find(parts)};
        }
    }
''')
    # Host-side prepared files must inherit the read-only DVD contract. Opening
    # them read-only is not enough: explicit metadata/delete APIs need guards.
    source=once(source,'    if (t.disc) return OpenOnDisc(t.entry, name, handle_out, iosb, disposition, options);',
        '''    if(t.read_only && ((access&0x500D0116u)||(options&0x1000u)||(disposition!=1&&disposition!=3))) {
        IoStatus(iosb,kAccessDenied,0);return kAccessDenied;
    }
    if (t.disc) return OpenOnDisc(t.entry, name, handle_out, iosb, disposition, options);''')
    source=once(source,'    bool is_dir = exists && fs::is_directory(host, ec);',
        '''    if(t.read_only&&!exists){IoStatus(iosb,kObjectNameNotFound,5);return kObjectNameNotFound;}
    bool is_dir = exists && fs::is_directory(host, ec);''')
    source=once(source,'    FILE* fp = std::fopen(host.c_str(), mode);',
        '    if(t.read_only)mode="rb";\n    FILE* fp = std::fopen(host.c_str(), mode);')
    source=once(source,'    if (t.disc) return OpenOnDisc(t.entry, name, handle_out, iosb, 1, options);',
        '''    if(t.read_only&&((a[1]&0x500D0116u)||(options&0x1000u))) {
        IoStatus(iosb,kAccessDenied,0);return kAccessDenied;
    }
    if (t.disc) return OpenOnDisc(t.entry, name, handle_out, iosb, 1, options);''')
    source=once(source,'        f->fp = std::fopen(host.c_str(), "r+b");',
        '        f->fp = std::fopen(host.c_str(), t.read_only?"rb":"r+b");')
    assert source.count('f->host = host;')==4  # three constructors and rename
    source=source.replace('f->host = host;\n', 'f->host = host;\n    f->read_only = t.read_only;\n',3)
    source=once(source,'    if (write && f->entry) return kAccessDenied;',
        '    if (write && (f->entry||f->read_only)) return kAccessDenied;')
    source=once(source,'    if (f->entry && cls != 14 && cls != 4) return kAccessDenied;',
        '    if ((f->entry||f->read_only) && cls != 14 && cls != 4) return kAccessDenied;')
    source=once(source,'            std::string host = ResolvePath(target, root);',
        '''            const auto destination=Resolve(target,root);
            if(destination.disc||destination.read_only)return kAccessDenied;
            std::string host = destination.host;''')
    source=once(source,'    std::string host = ResolvePath(ReadAnsiString(R32(a[0] + 4)), R32(a[0]));',
        '''    const auto target=Resolve(ReadAnsiString(R32(a[0]+4)),R32(a[0]));
    if(target.disc||target.read_only)return kAccessDenied;
    std::string host = target.host;''')
    old='''        if (f->entry) {
            for (const Disc::Entry& e : f->entry->children)
                if (WildMatch(pat, e.name)) f->disc_entries.push_back(&e);
        } else {'''
    new=r'''        if (f->entry) {
            for (const Disc::Entry& e : f->entry->children) {
                if(!WildMatch(pat,e.name))continue;
                const auto resolved=Resolve(f->xbox+"\\"+e.name,0);
                if(resolved.disc)f->disc_entries.push_back(&e);
                else {
                    std::error_code ec;
                    if(fs::is_regular_file(resolved.host,ec)&&!ec)f->entries.emplace_back(resolved.host);
                }
            }
            // Generated archives are absent from the original disc directory.
            if(f->entry==MountedDisc()->Find(std::vector<std::string>{})) {
                for(const auto* name:kPreparedArchives) {
                    if(!WildMatch(pat,name)||MountedDisc()->Find(name))continue;
                    const auto resolved=Resolve(f->xbox+"\\"+name,0);
                    std::error_code ec;
                    if(!resolved.host.empty()&&fs::is_regular_file(resolved.host,ec)&&!ec)
                        f->entries.emplace_back(resolved.host);
                }
            }
        } else {'''
    source=once(source,old,new)
    source=once(source,'    if (f->next >= (f->entry ? f->disc_entries.size() : f->entries.size())) {',
        '    if (f->next >= f->disc_entries.size()+f->entries.size()) {')
    source=once(source,'    std::string fname = f->entry ? f->disc_entries[i]->name : f->entries[i].path().filename().string();',
        '''    const bool disc_entry=i<f->disc_entries.size();
    const size_t local_index=disc_entry?0:i-f->disc_entries.size();
    std::string fname = disc_entry ? f->disc_entries[i]->name : f->entries[local_index].path().filename().string();''')
    source=once(source,'    if (f->entry) FillNetworkInfo(out + 8, *f->disc_entries[i]);\n    else FillNetworkInfo(out + 8, f->entries[i].path());',
        '    if (disc_entry) FillNetworkInfo(out + 8, *f->disc_entries[i]);\n    else FillNetworkInfo(out + 8, f->entries[local_index].path());')
    return source


def prepare(sdk,area):
    sdk,area=Path(sdk),Path(area)
    source=(sdk/'runtime/app/game_files.cpp').read_text()
    # Keep the developer implementation verbatim; the release executable uses
    # the launcher's explicit directory/optional ISO contract exclusively.
    for signature,replacement in [('fs::path FindAssets()',RELEASE_ASSETS),
                                  ('bool FindGame(GameFiles& game)',RELEASE_GAME)]:
        original=function(source,signature)
        wrapped='#ifdef UC_BURNER_RELEASE\n'+replacement+'\n#else\n'+original+'\n#endif'
        source=once(source,original,wrapped)
        assert '\n#else\n'+original+'\n#endif' in source
    target=area/'src/compat'
    target.mkdir(parents=True,exist_ok=True)
    (target/'game_files.cpp').write_text(source)
    disc=(sdk/'runtime/kernel/disc.cpp').read_text()
    disc=once(disc,'constexpr uint64_t kPartitions[] = {0, 0x18300000};',
        'constexpr uint64_t kPartitions[] = {0, 0xFD90, 0x30600, 0x0FD90000, 0x18300000};')
    (target/'disc.cpp').write_text(disc)
    # No XBE bytes or private paths are generated into this fixture or runtime.
    (target/'test_release_assets.cpp').write_text(
        (Path(__file__).parent/'test_xbe_release_assets.cpp').read_text())
    (target/'test_disc_files.cpp').write_text(
        (Path(__file__).parent/'test_xbe_disc_files.cpp').read_text())
    files=target/'files.cpp'
    files.write_text(patch_files(files.read_text()))
