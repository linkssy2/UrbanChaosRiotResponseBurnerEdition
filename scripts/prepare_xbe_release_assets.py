"""Release-only Setup directory and exact executable revision admission."""
from pathlib import Path


def function(source, signature):
    start=source.index(signature)
    return source[start:source.index('\n}',start)+2]


def once(source,old,new):
    assert source.count(old)==1,old
    return source.replace(old,new,1)


RELEASE_ASSETS=r'''fs::path FindAssets() {
    // Setup supplies the extracted folder even when the input was an ISO.
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
    game={assets,std::move(xbe)};
    kernel::MountDisc(nullptr);
    return true;
}'''


def prepare(sdk,area):
    sdk,area=Path(sdk),Path(area)
    source=(sdk/'runtime/app/game_files.cpp').read_text()
    # Keep the developer implementation verbatim; the release executable uses
    # the launcher/importer's extracted directory contract exclusively.
    for signature,replacement in [('fs::path FindAssets()',RELEASE_ASSETS),
                                  ('bool FindGame(GameFiles& game)',RELEASE_GAME)]:
        original=function(source,signature)
        wrapped='#ifdef UC_BURNER_RELEASE\n'+replacement+'\n#else\n'+original+'\n#endif'
        source=once(source,original,wrapped)
        assert '\n#else\n'+original+'\n#endif' in source
    target=area/'src/compat'
    target.mkdir(parents=True,exist_ok=True)
    (target/'game_files.cpp').write_text(source)
    # No XBE bytes or private paths are generated into this fixture or runtime.
    (target/'test_release_assets.cpp').write_text(
        (Path(__file__).parent/'test_xbe_release_assets.cpp').read_text())
