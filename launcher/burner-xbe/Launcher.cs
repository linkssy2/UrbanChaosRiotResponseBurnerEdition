using System;
using System.IO;
using System.Text;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Windows.Forms;
using System.Diagnostics;
using System.Threading.Tasks;
using System.Web.Script.Serialization;
using System.Collections.Generic;

sealed class LauncherBanner : Panel {
    public LauncherBanner(){DoubleBuffered=true;}
    protected override void OnPaintBackground(PaintEventArgs e){
        if(Width<1||Height<1)return;
        using(var fill=new LinearGradientBrush(ClientRectangle,Color.FromArgb(24,48,50),Color.FromArgb(9,17,19),0f))e.Graphics.FillRectangle(fill,ClientRectangle);
        using(var scan=new Pen(Color.FromArgb(26,0,0,0)))for(int y=0;y<Height;y+=4)e.Graphics.DrawLine(scan,0,y,Width,y);
        using(var angle=new SolidBrush(Color.FromArgb(17,176,195,182)))e.Graphics.FillPolygon(angle,new[]{new Point(Width-170,0),new Point(Width,0),new Point(Width,Height),new Point(Width-240,Height)});
        using(var line=new Pen(Color.FromArgb(91,117,112)))e.Graphics.DrawLine(line,0,Height-1,Width,Height-1);
        using(var accent=new Pen(Color.FromArgb(168,164,111),3))e.Graphics.DrawLine(accent,0,Height-2,86,Height-2);
    }
}

class BurnerLauncher : Form {
    static readonly string Root=AppDomain.CurrentDomain.BaseDirectory;
    readonly Label status=new Label(), heading=new Label(), description=new Label();
    readonly ProgressBar progress=new ProgressBar();
    readonly Panel pages=new Panel();
    readonly TableLayoutPanel home=Page(), source=Page(), manage=Page();
    readonly TableLayoutPanel manageSettings=Page(), manageFiles=Page(), manageAddons=Page(), manageUpdates=Page(), manageSupport=Page();
    readonly TableLayoutPanel[] manageSections;
    readonly Button[] manageNavigation;
    readonly ComboBox language=new ComboBox();
    readonly Button play, select, textures, voices, verify, folder, saves, cancel, diagnostics, updates, skipStartup;
    string data=Path.Combine(Root,"GameData"), cancelPath;
    bool busy, running, ready, spanishAvailable, diagnosticLogging, updatePrepared, skipStartupEnabled;
    Image cover;

    // Windows argv quoting must also preserve a selected directory's final backslash.
    internal static string Quote(string value) {
        var result=new StringBuilder("\"");int slashes=0;
        foreach(char c in value){if(c=='\\'){slashes++;continue;}
            result.Append('\\',c=='"'?slashes*2+1:slashes);result.Append(c);slashes=0;}
        return result.Append('\\',slashes*2).Append('"').ToString();
    }
    internal static ProcessStartInfo Worker(string action,string extra="") {
        var p=new ProcessStartInfo(Path.Combine(Root,"tools","python","python.exe"),"-B "+Quote(Path.Combine(Root,"tools","import_game.py"))+" "+action+extra);
        p.UseShellExecute=false;p.CreateNoWindow=true;p.RedirectStandardOutput=true;p.RedirectStandardError=true;p.WorkingDirectory=Root;return p;
    }
    static Dictionary<string,object> Row(string text) {
        var result=new Dictionary<string,object>();
        foreach(string line in text.Split('\n')){try{result=new JavaScriptSerializer().Deserialize<Dictionary<string,object>>(line)??result;}catch{}}
        return result;
    }
    static string Field(Dictionary<string,object> row,string key,string fallback="") {
        object value;return row.TryGetValue(key,out value)?Convert.ToString(value):fallback;
    }
    static async Task<Tuple<int,string,string>> ReadWorker(string action) {
        using(var p=Process.Start(Worker(action))){var output=p.StandardOutput.ReadToEndAsync();var error=p.StandardError.ReadToEndAsync();
            await Task.Run(()=>p.WaitForExit());return Tuple.Create(p.ExitCode,await output,await error);}
    }
    static TableLayoutPanel Page(){var page=new TableLayoutPanel{Dock=DockStyle.Fill,ColumnCount=1,RowCount=0,AutoScroll=true,Margin=Padding.Empty,Padding=new Padding(0,4,6,4)};page.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,100));return page;}
    static void Add(TableLayoutPanel page,Control control,int height){int row=page.RowCount++;page.RowStyles.Add(new RowStyle(SizeType.Absolute,height));page.Controls.Add(control,0,row);}
    static Label TextLabel(string text,int size=10){return new Label{Text=text,Dock=DockStyle.Fill,BackColor=Color.Transparent,ForeColor=Color.FromArgb(187,201,193),Font=new Font("Segoe UI",size),Margin=Padding.Empty,Padding=new Padding(0,4,0,0)};}
    Button MakeButton(string text,EventHandler handler,bool primary=false){
        var b=new Button{Text=text,AccessibleName=text,Dock=DockStyle.Fill,FlatStyle=FlatStyle.Flat,BackColor=primary?Color.FromArgb(59,76,62):Color.FromArgb(22,39,42),ForeColor=Color.FromArgb(237,237,222),Font=new Font("Segoe UI",primary?14:10,primary?FontStyle.Bold|FontStyle.Italic:FontStyle.Bold),Margin=new Padding(0,3,0,3),Padding=new Padding(12,0,8,0),TextAlign=ContentAlignment.MiddleLeft,Cursor=Cursors.Hand};
        b.FlatAppearance.BorderColor=primary?Color.FromArgb(168,164,111):Color.FromArgb(65,91,88);b.FlatAppearance.MouseOverBackColor=Color.FromArgb(49,69,64);b.FlatAppearance.MouseDownBackColor=Color.FromArgb(71,86,65);
        b.Enter+=(s,e)=>b.FlatAppearance.BorderColor=Color.FromArgb(223,223,193);b.Leave+=(s,e)=>b.FlatAppearance.BorderColor=primary?Color.FromArgb(168,164,111):Color.FromArgb(65,91,88);
        b.Click+=handler;return b;
    }
    TableLayoutPanel PageHeading(string title,string backName){
        var row=new TableLayoutPanel{Dock=DockStyle.Fill,ColumnCount=2,RowCount=1,Margin=Padding.Empty};row.RowStyles.Add(new RowStyle(SizeType.Percent,100));row.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,100));row.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute,104));
        var label=TextLabel(title,18);label.Font=new Font("Segoe UI",18,FontStyle.Bold|FontStyle.Italic);label.ForeColor=Color.FromArgb(237,237,222);row.Controls.Add(label,0,0);
        var back=MakeButton("BACK",(s,e)=>ShowPage(home));back.Name=backName;row.Controls.Add(back,1,0);return row;
    }
    void ShowPage(TableLayoutPanel page){home.Visible=page==home;source.Visible=page==source;manage.Visible=page==manage;page.BringToFront();}
    void ShowManageSection(TableLayoutPanel section){
        for(int i=0;i<manageSections.Length;i++){bool selected=manageSections[i]==section;manageSections[i].Visible=selected;manageNavigation[i].BackColor=selected?Color.FromArgb(55,76,65):Color.FromArgb(17,31,34);manageNavigation[i].ForeColor=selected?Color.FromArgb(237,237,222):Color.FromArgb(175,194,185);}
        section.BringToFront();
    }

    public BurnerLauncher(){
        Text="Urban Chaos: Riot Response - Burner Edition";ClientSize=new Size(1040,740);MinimumSize=new Size(880,680);StartPosition=FormStartPosition.CenterScreen;BackColor=Color.FromArgb(8,16,18);ForeColor=Color.FromArgb(237,237,222);AutoScaleMode=AutoScaleMode.Dpi;AutoScaleDimensions=new SizeF(96,96);KeyPreview=true;
        Icon=Icon.ExtractAssociatedIcon(Application.ExecutablePath);
        var layout=new TableLayoutPanel{Dock=DockStyle.Fill,ColumnCount=1,RowCount=4,Padding=new Padding(24),Margin=Padding.Empty};
        layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,100));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute,96));layout.RowStyles.Add(new RowStyle(SizeType.Percent,100));layout.RowStyles.Add(new RowStyle(SizeType.Absolute,76));layout.RowStyles.Add(new RowStyle(SizeType.Absolute,48));Controls.Add(layout);
        var banner=new LauncherBanner{Dock=DockStyle.Fill,Margin=new Padding(0,0,0,16),Padding=new Padding(16,6,0,4)};
        var masthead=new TableLayoutPanel{Dock=DockStyle.Fill,ColumnCount=1,RowCount=2,BackColor=Color.Transparent,Margin=Padding.Empty};masthead.RowStyles.Add(new RowStyle(SizeType.Percent,100));masthead.RowStyles.Add(new RowStyle(SizeType.Absolute,25));
        masthead.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,100));
        var title=TextLabel("URBAN CHAOS",30);title.Font=new Font("Arial",30,FontStyle.Bold|FontStyle.Italic);title.ForeColor=Color.FromArgb(237,237,222);masthead.Controls.Add(title,0,0);
        var edition=TextLabel("RIOT RESPONSE  /  BURNER EDITION",10);edition.ForeColor=Color.FromArgb(172,183,144);masthead.Controls.Add(edition,0,1);banner.Controls.Add(masthead);layout.Controls.Add(banner,0,0);
        var content=new TableLayoutPanel{Dock=DockStyle.Fill,ColumnCount=2,RowCount=1,Margin=Padding.Empty};content.RowStyles.Add(new RowStyle(SizeType.Percent,100));content.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,30));content.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,70));layout.Controls.Add(content,0,1);
        string coverPath=Path.Combine(Root,"cover.png");if(File.Exists(coverPath))cover=Image.FromFile(coverPath);
        var artwork=new Panel{Dock=DockStyle.Fill,BackColor=BackColor,Margin=new Padding(0,0,22,0)};
        artwork.Resize+=(s,e)=>artwork.Invalidate();
        artwork.Paint+=(s,e)=>{
            if(artwork.Width<1||artwork.Height<1)return;
            if(cover!=null){float scale=Math.Min((float)artwork.Width/cover.Width,(float)artwork.Height/cover.Height);
                var frame=new Rectangle(0,0,Math.Max(1,(int)(cover.Width*scale)),Math.Max(1,(int)(cover.Height*scale)));
                e.Graphics.InterpolationMode=InterpolationMode.HighQualityBicubic;
                e.Graphics.DrawImage(cover,frame);
                using(var pen=new Pen(Color.FromArgb(65,91,88)))e.Graphics.DrawRectangle(pen,0,0,frame.Width-1,frame.Height-1);}
        };content.Controls.Add(artwork,0,0);
        pages.Dock=DockStyle.Fill;pages.Margin=Padding.Empty;pages.BackColor=Color.FromArgb(12,24,27);pages.Padding=new Padding(16,10,10,10);pages.Controls.AddRange(new Control[]{home,source,manage});content.Controls.Add(pages,1,0);
        heading.Text="PREPARE YOUR GAME";heading.Dock=DockStyle.Fill;heading.Font=new Font("Segoe UI",20,FontStyle.Bold|FontStyle.Italic);heading.Margin=Padding.Empty;Add(home,heading,48);
        description.Text="Choose your Xbox ISO or reuse an existing installation. You can also place one ISO/XISO in the assets folder for automatic setup.";description.Dock=DockStyle.Fill;description.Font=new Font("Segoe UI",11);description.ForeColor=Color.FromArgb(187,201,193);description.Margin=Padding.Empty;Add(home,description,92);
        play=MakeButton("PLAY",async(s,e)=>await Play(),true);select=MakeButton("SELECT GAME",(s,e)=>ShowPage(source),true);
        var primary=new Panel{Dock=DockStyle.Fill,Margin=new Padding(0,3,8,3)};primary.Controls.AddRange(new Control[]{play,select});
        var homeActions=new TableLayoutPanel{Dock=DockStyle.Fill,ColumnCount=2,RowCount=1,Margin=Padding.Empty};homeActions.RowStyles.Add(new RowStyle(SizeType.Percent,100));homeActions.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,50));homeActions.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,50));homeActions.Controls.Add(primary,0,0);
        var manageButton=MakeButton("MANAGE",(s,e)=>ShowPage(manage));manageButton.Name="manageButton";manageButton.Font=play.Font;manageButton.Margin=new Padding(8,3,0,3);homeActions.Controls.Add(manageButton,1,0);Add(home,homeActions,60);
        Add(home,TextLabel("Single-player and co-op use the same game.\nSettings, add-ons and support are in Manage."),60);
        Add(source,PageHeading("SELECT GAME","sourceBack"),42);
        Add(source,MakeButton("USE ISO DIRECTLY",async(s,e)=>await SelectIso(true),true),46);
        Add(source,TextLabel("Play from your ISO. Keep it in place; only preparation files are copied.",9),42);
        Add(source,MakeButton("IMPORT ONCE",async(s,e)=>await SelectIso(false)),46);
        Add(source,TextLabel("Copy all game files once. You can remove the ISO after setup.",9),42);
        Add(source,MakeButton("USE EXISTING INSTALL",async(s,e)=>await Existing()),46);
        Add(source,TextLabel("Reuse a previous launcher or GameData folder, including its saves and add-ons.",9),46);
        Add(manage,PageHeading("MANAGE","manageBack"),42);
        var manager=new TableLayoutPanel{Dock=DockStyle.Fill,ColumnCount=2,RowCount=1,Margin=Padding.Empty};manager.RowStyles.Add(new RowStyle(SizeType.Percent,100));manager.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute,138));manager.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,100));
        var navigation=Page();navigation.Padding=new Padding(0,6,12,0);manager.Controls.Add(navigation,0,0);
        var sections=new Panel{Dock=DockStyle.Fill,Margin=Padding.Empty,Padding=new Padding(10,0,0,0)};manager.Controls.Add(sections,1,0);
        manageSections=new[]{manageSettings,manageFiles,manageAddons,manageUpdates,manageSupport};manageNavigation=new Button[manageSections.Length];
        string[] sectionNames={"SETTINGS","GAME FILES","ADD-ONS","UPDATES","SUPPORT"};
        for(int i=0;i<manageSections.Length;i++){var section=manageSections[i];section.AccessibleName=sectionNames[i];sections.Controls.Add(section);manageNavigation[i]=MakeButton(sectionNames[i],(s,e)=>ShowManageSection(section));manageNavigation[i].Font=new Font("Segoe UI",9,FontStyle.Bold);Add(navigation,manageNavigation[i],48);}
        navigation.RowCount++;navigation.RowStyles.Add(new RowStyle(SizeType.Percent,100));
        manage.RowCount++;manage.RowStyles.Add(new RowStyle(SizeType.Percent,100));manage.Controls.Add(manager,0,manage.RowCount-1);
        Add(manageSettings,TextLabel("LANGUAGE",10),26);
        language.DropDownStyle=ComboBoxStyle.DropDownList;language.Dock=DockStyle.Fill;language.AccessibleName="Game language";language.Font=new Font("Segoe UI",11);language.BackColor=Color.FromArgb(24,42,43);language.ForeColor=ForeColor;language.FlatStyle=FlatStyle.Flat;
        language.Items.AddRange(new object[]{"English","Français","Italiano","Deutsch","Español (ZARF beta)"});language.SelectedIndex=0;Add(manageSettings,language,36);
        language.SelectionChangeCommitted+=(s,e)=>{if(language.SelectedIndex==4&&!spanishAvailable){status.Text="Set up your Xbox game first. Spanish text is included; optional voices come from your European PS2 ISO.";language.SelectedIndex=0;}else status.Text="Language will be saved when you press PLAY.";};
        Add(manageSettings,TextLabel("Saved when you press Play. Spanish text is included; voices are an optional add-on.",9),56);
        skipStartup=MakeButton("SKIP STARTUP: OFF",async(s,e)=>await Operation("startup"," --skip-startup "+(skipStartupEnabled?"off":"on")));Add(manageSettings,skipStartup,48);
        Add(manageSettings,TextLabel("Skip intro videos and Press Start to profile selection. Mission videos are unchanged. Applies next game session.",9),72);
        Add(manageFiles,TextLabel("GAME FILES",12),34);
        Add(manageFiles,TextLabel("Choose your game source, check its files, or open your game and save folders.",9),40);
        Add(manageFiles,MakeButton("SELECT GAME / REPAIR",(s,e)=>ShowPage(source)),48);
        verify=MakeButton("VERIFY FILES",async(s,e)=>await Operation("verify",""));Add(manageFiles,verify,48);
        folder=MakeButton("GAME FOLDER",(s,e)=>OpenFolder(data));saves=MakeButton("SAVES",(s,e)=>OpenFolder(Path.Combine(data,"xbe-runtime","hdd")));Add(manageFiles,folder,48);Add(manageFiles,saves,48);
        Add(manageAddons,TextLabel("OPTIONAL ADD-ONS",12),34);
        textures=MakeButton("INSTALL HD TEXTURE PACK",async(s,e)=>await SelectAddon(false));Add(manageAddons,textures,48);
        Add(manageAddons,TextLabel("Select your downloaded HD texture ZIP.",9),38);
        voices=MakeButton("IMPORT SPANISH VOICES",async(s,e)=>await SelectAddon(true));Add(manageAddons,voices,48);
        Add(manageAddons,TextLabel("Requires your European PS2 ISO (SLES-53991). Spanish text is already included.",9),62);
        Add(manageUpdates,TextLabel("LAUNCHER UPDATES",12),34);
        Add(manageUpdates,TextLabel("Check for a newer Burner Edition release. Saves, settings and installed add-ons are preserved.",9),68);
        updates=MakeButton("CHECK FOR UPDATES",async(s,e)=>await UpdateRelease());Add(manageUpdates,updates,48);
        Add(manageSupport,TextLabel("SUPPORT",12),34);
        Add(manageSupport,TextLabel("Enable logging before reproducing a problem, then share the relevant logs manually.",9),58);
        diagnostics=MakeButton("DIAGNOSTIC LOGGING: OFF",async(s,e)=>await Operation("diagnostics"," --logging "+(diagnosticLogging?"off":"on")));Add(manageSupport,diagnostics,48);
        Add(manageSupport,MakeButton("OPEN LOGS",(s,e)=>OpenFolder(Path.Combine(Root,"GameData","logs"))),48);
        Add(manageSupport,TextLabel("Off by default. Applies next game session. Logs stay on this computer.",9),58);
        foreach(var page in new[]{home,source,manageSettings,manageFiles,manageAddons,manageUpdates,manageSupport}){int height=page.Padding.Vertical;foreach(RowStyle row in page.RowStyles)height+=(int)row.Height;page.AutoScrollMinSize=new Size(0,height);page.RowCount++;page.RowStyles.Add(new RowStyle(SizeType.Percent,100));}
        ShowManageSection(manageSettings);
        var feedback=new TableLayoutPanel{Dock=DockStyle.Fill,ColumnCount=1,RowCount=3,Margin=new Padding(0,10,0,4)};feedback.RowStyles.Add(new RowStyle(SizeType.Absolute,20));feedback.RowStyles.Add(new RowStyle(SizeType.Percent,100));feedback.RowStyles.Add(new RowStyle(SizeType.Absolute,8));
        feedback.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,100));
        var statusTitle=TextLabel("STATUS",8);statusTitle.ForeColor=Color.FromArgb(172,183,144);feedback.Controls.Add(statusTitle,0,0);status.Text="Checking game data...";status.Dock=DockStyle.Fill;status.Font=new Font("Segoe UI",9);status.Margin=Padding.Empty;feedback.Controls.Add(status,0,1);progress.Dock=DockStyle.Fill;progress.Visible=false;progress.Margin=Padding.Empty;feedback.Controls.Add(progress,0,2);layout.Controls.Add(feedback,0,2);
        var footer=new TableLayoutPanel{Dock=DockStyle.Fill,ColumnCount=2,RowCount=1,Margin=Padding.Empty};footer.RowStyles.Add(new RowStyle(SizeType.Percent,100));footer.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,100));footer.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute,122));footer.Controls.Add(TextLabel("v0.17  /  WINDOWS + PROTON\nOriginal disc files are not included.",9),0,0);
        cancel=MakeButton("QUIT",(s,e)=>{if(busy&&!running&&cancelPath!=null){File.WriteAllText(cancelPath,"");status.Text="Cancelling safely...";cancel.Enabled=false;}else if(!busy)Close();});footer.Controls.Add(cancel,1,0);layout.Controls.Add(footer,0,3);
        ApplyStatus(new Dictionary<string,object>{{"ready",false}});ShowPage(home);
#if !LAUNCHER_PREVIEW
        Shown+=async(s,e)=>{busy=true;SetEnabled();try{await RefreshReady();busy=false;if(!ready)await Operation("auto-setup","");}catch(Exception ex){status.Text=ex.Message;}finally{busy=false;SetEnabled();}};
#endif
        KeyDown+=(s,e)=>{if(e.KeyCode==Keys.Escape&&!busy){ShowPage(home);e.Handled=true;}};
        FormClosing+=(s,e)=>{if(busy){e.Cancel=true;status.Text=running?"Close the game before exiting the launcher.":"Use Cancel to stop setup safely.";}};
    }
    void ApplyStatus(Dictionary<string,object> row){
        ready=Field(row,"ready")=="True";data=Field(row,"data",data);play.Visible=ready;select.Visible=!ready;
        diagnosticLogging=Field(row,"diagnostic_logging")=="True";diagnostics.Text=diagnostics.AccessibleName="DIAGNOSTIC LOGGING: "+(diagnosticLogging?"ON":"OFF");
        skipStartupEnabled=Field(row,"skip_startup")=="True";skipStartup.Text=skipStartup.AccessibleName="SKIP STARTUP: "+(skipStartupEnabled?"ON":"OFF");
        heading.Text=ready?"READY TO PLAY":"PREPARE YOUR GAME";
        description.Text=ready?(Field(row,"source_mode")=="iso"?"Your Xbox ISO is connected. Keep it in place for play and updates.":"Your installed game is ready. No ISO is needed to play."):"Choose your Xbox ISO or reuse an existing installation. You can also place one ISO/XISO in the assets folder for automatic setup.";
        status.Text=Field(row,"message",ready?"Ready to play.":"Select Game to get started.");SetEnabled();
    }
    void SetEnabled(){pages.Enabled=!busy;play.Enabled=!busy&&ready;textures.Enabled=voices.Enabled=verify.Enabled=language.Enabled=!busy&&ready;folder.Enabled=saves.Enabled=!busy&&Directory.Exists(data);cancel.Text=busy&&!running?"CANCEL":"QUIT";cancel.Enabled=!busy||(!running&&cancelPath!=null);progress.Visible=busy&&!running;}
    async Task RefreshReady(bool refreshLanguage=true){
        var result=await ReadWorker("status");if(result.Item1!=0&&result.Item1!=2)throw new InvalidOperationException(Field(Row(result.Item2),"message","Unable to check installation. "+result.Item3));
        ApplyStatus(Row(result.Item2));
        if(ready&&refreshLanguage){var resultLanguage=await ReadWorker("language");if(resultLanguage.Item1==0){var row=Row(resultLanguage.Item2);int selected;if(Int32.TryParse(Field(row,"language"),out selected)&&selected>=0&&selected<language.Items.Count)language.SelectedIndex=selected;spanishAvailable=Field(row,"spanish_available")=="True";}}
    }
    async Task<int> RunWorker(string action,string extra){
        using(var p=Process.Start(Worker(action,extra))){var errors=p.StandardError.ReadToEndAsync();string line;
            while((line=await p.StandardOutput.ReadLineAsync())!=null){var row=Row(line);if(action=="update"&&Field(row,"event")=="release_ready")updatePrepared=true;if(row.ContainsKey("message"))status.Text=Field(row,"message");int percent;if(Int32.TryParse(Field(row,"percent"),out percent))progress.Value=Math.Max(0,Math.Min(100,percent));}
            await Task.Run(()=>p.WaitForExit());string error=await errors;if(p.ExitCode!=0&&!String.IsNullOrWhiteSpace(error))status.Text="Setup could not finish. "+error.Trim();return p.ExitCode;}
    }
    async Task UpdateRelease(){
        if(busy)return;busy=true;running=false;updatePrepared=false;SetEnabled();status.Text="Checking GitHub for a newer release...";
        try{
            var result=await ReadWorker("check-updates");var row=Row(result.Item2);status.Text=Field(row,"message","Could not check for updates. You can keep playing.");
            if(result.Item1!=0||Field(row,"status")!="available")return;
            string version=Field(row,"latest_version");
            if(MessageBox.Show(this,"Install Burner Edition "+version+"?\n\nThe launcher will download and verify the official update, then restart. Saves, settings, your ISO and installed add-ons are kept.","Update Burner Edition",MessageBoxButtons.YesNo,MessageBoxIcon.Question,MessageBoxDefaultButton.Button2)!=DialogResult.Yes)return;
            cancelPath=Path.Combine(Root,".cancel-"+Guid.NewGuid().ToString("N"));progress.Value=0;SetEnabled();
            int code=await RunWorker("update"," --launcher-pid "+Process.GetCurrentProcess().Id+" --cancel "+Quote(cancelPath));
            if(code==0&&updatePrepared){busy=false;Close();}
            else if(code==0)status.Text="The update was not prepared. Your installed game was not changed.";
        }catch(Exception ex){status.Text=ex.Message;}
        finally{busy=false;if(!IsDisposed)SetEnabled();if(cancelPath!=null&&File.Exists(cancelPath))File.Delete(cancelPath);cancelPath=null;}
    }
    async Task Operation(string action,string extra){
        if(busy)return;cancelPath=Path.Combine(Root,".cancel-"+Guid.NewGuid().ToString("N"));busy=true;running=false;progress.Value=0;SetEnabled();status.Text="Checking and preparing game files...";
        try{int code=await RunWorker(action,extra+" --cancel "+Quote(cancelPath));string message=status.Text;await RefreshReady(action!="startup");if(code!=0)status.Text=message;else{if(action=="voices")language.SelectedIndex=4;if(action=="verify")status.Text="File verification completed successfully.";else if(action=="diagnostics"||action=="startup")status.Text=message;else ShowPage(home);}}
        catch(Exception ex){status.Text=ex.Message;}
        finally{busy=false;running=false;SetEnabled();if(File.Exists(cancelPath))File.Delete(cancelPath);cancelPath=null;}
    }
    async Task SelectIso(bool direct){using(var d=new OpenFileDialog{Title="Select your Urban Chaos Xbox ISO / XISO",Filter="Xbox disc images (*.iso;*.xiso)|*.iso;*.xiso",CheckFileExists=true}){if(d.ShowDialog(this)!=DialogResult.OK)return;await Operation(direct?"use-iso":"import"," --iso "+Quote(d.FileName));}}
    async Task Existing(){using(var d=new FolderBrowserDialog{Description="Select the previous Burner Edition folder or its GameData folder. Files and saves will stay there.",ShowNewFolderButton=false}){if(d.ShowDialog(this)!=DialogResult.OK)return;await Operation("use-installation"," --installation "+Quote(d.SelectedPath));}}
    async Task SelectAddon(bool voice){using(var d=new OpenFileDialog{Title=voice?"Select your European Urban Chaos PS2 ISO (SLES-53991)":"Select the Burner Edition HD texture ZIP",Filter=voice?"PS2 disc image (*.iso)|*.iso":"HD texture pack (*.zip)|*.zip",CheckFileExists=true}){if(d.ShowDialog(this)!=DialogResult.OK)return;await Operation(voice?"voices":"textures",(voice?" --iso ":" --pack ")+Quote(d.FileName));}}
    void OpenFolder(string path){if(Directory.Exists(path))Process.Start(new ProcessStartInfo("explorer.exe",Quote(path)){UseShellExecute=true});else status.Text="This folder has not been created yet.";}
    async Task Play(){if(busy||!ready)return;busy=true;running=true;SetEnabled();status.Text="Preparing game and bundled languages...";
        try{int code=await RunWorker("launch"," --language "+language.SelectedIndex);string message=status.Text;await RefreshReady();if(code!=0)status.Text=message;}
        catch(Exception ex){status.Text=ex.Message;}finally{busy=false;running=false;SetEnabled();}}
    protected override void Dispose(bool disposing){if(disposing&&cover!=null)cover.Dispose();base.Dispose(disposing);}
    [STAThread] static int Main(string[] args){
        Application.EnableVisualStyles();Application.SetCompatibleTextRenderingDefault(false);
        bool first;using(var mutex=new System.Threading.Mutex(true,"Local\\UrbanChaosBurnerEdition",out first)){
            if(!first){MessageBox.Show("Burner Edition is already open.","Burner Edition");return 1;}
            try{
#if !LAUNCHER_PREVIEW
                if(BurnerUpdates.SetupProcess.TryRecover(Root))return 0;
#endif
#if DIRECT
                var statusResult=ReadWorker("status").GetAwaiter().GetResult();
                if(statusResult.Item1==0){var result=ReadWorker("launch").GetAwaiter().GetResult();if(result.Item1!=0)MessageBox.Show(Field(Row(result.Item2),"message","The game could not start.")+"\n"+result.Item3,"Burner Edition",MessageBoxButtons.OK,MessageBoxIcon.Error);return result.Item1;}
#endif
                using(var window=new BurnerLauncher()){
#if LAUNCHER_PREVIEW
                    if(args.Length>=2&&args[0]=="--preview"){
                        if(args.Length>2&&args[2]=="ready")window.ApplyStatus(new Dictionary<string,object>{{"ready",true},{"source_mode","iso"}});
                        if(args.Length>2&&args[2]=="source")window.ShowPage(window.source);
                        if(args.Length>2&&args[2]=="manage"){window.ApplyStatus(new Dictionary<string,object>{{"ready",true}});window.ShowPage(window.manage);}
                        window.ShowInTaskbar=false;window.StartPosition=FormStartPosition.Manual;window.Location=new Point(-32000,-32000);window.Show();window.PerformLayout();Application.DoEvents();using(var bitmap=new Bitmap(window.Width,window.Height)){window.DrawToBitmap(bitmap,new Rectangle(Point.Empty,bitmap.Size));bitmap.Save(args[1]);}return 0;}
#endif
                    Application.Run(window);
                }return 0;
            }catch(Exception ex){MessageBox.Show(ex.Message,"Burner Edition",MessageBoxButtons.OK,MessageBoxIcon.Error);return 1;}
        }
    }
}
