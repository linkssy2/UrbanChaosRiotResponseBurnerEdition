using System;
using System.IO;
using System.Text;
using System.Drawing;
using System.Windows.Forms;
using System.Diagnostics;
using System.Threading.Tasks;
using System.Web.Script.Serialization;
using System.Collections.Generic;

class BurnerLauncher : Form {
    static readonly string Root=AppDomain.CurrentDomain.BaseDirectory;
    readonly Label status=new Label(), heading=new Label(), description=new Label();
    readonly ProgressBar progress=new ProgressBar();
    readonly Panel pages=new Panel();
    readonly TableLayoutPanel home=Page(), source=Page(), manage=Page();
    readonly ComboBox language=new ComboBox();
    readonly Button play, select, textures, voices, verify, folder, saves, cancel, diagnostics, updates;
    string data=Path.Combine(Root,"GameData"), cancelPath;
    bool busy, running, ready, spanishAvailable, diagnosticLogging, updatePrepared;
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
    static TableLayoutPanel Page(){return new TableLayoutPanel{Dock=DockStyle.Fill,ColumnCount=1,RowCount=0,AutoScroll=true,Padding=new Padding(0,4,6,4)};}
    static void Add(TableLayoutPanel page,Control control,int height){int row=page.RowCount++;page.RowStyles.Add(new RowStyle(SizeType.Absolute,height));page.Controls.Add(control,0,row);}
    static Label TextLabel(string text,int size=11){return new Label{Text=text,Dock=DockStyle.Fill,ForeColor=Color.FromArgb(190,196,190),Font=new Font("Segoe UI",size),Padding=new Padding(0,4,0,0)};}
    Button MakeButton(string text,EventHandler handler,bool primary=false){
        var b=new Button{Text=text,AccessibleName=text,Dock=DockStyle.Fill,FlatStyle=FlatStyle.Flat,BackColor=primary?Color.FromArgb(138,56,19):Color.FromArgb(29,36,38),ForeColor=Color.White,Font=new Font("Segoe UI",12,FontStyle.Bold),Margin=new Padding(0,4,0,4),Cursor=Cursors.Hand};
        b.FlatAppearance.BorderColor=Color.FromArgb(105,119,115);b.Click+=handler;return b;
    }
    void ShowPage(TableLayoutPanel page){home.Visible=page==home;source.Visible=page==source;manage.Visible=page==manage;page.BringToFront();}

    public BurnerLauncher(){
        Text="Urban Chaos: Riot Response - Burner Edition";ClientSize=new Size(1040,740);MinimumSize=new Size(880,680);StartPosition=FormStartPosition.CenterScreen;BackColor=Color.FromArgb(12,18,20);ForeColor=Color.White;AutoScaleMode=AutoScaleMode.Dpi;KeyPreview=true;
        Icon=Icon.ExtractAssociatedIcon(Application.ExecutablePath);
        var layout=new TableLayoutPanel{Dock=DockStyle.Fill,ColumnCount=2,Padding=new Padding(20)};layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,43));layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,57));Controls.Add(layout);
        string coverPath=Path.Combine(Root,"cover.png");if(File.Exists(coverPath))cover=Image.FromFile(coverPath);
        layout.Controls.Add(new PictureBox{Image=cover,SizeMode=PictureBoxSizeMode.Zoom,Dock=DockStyle.Fill,Margin=new Padding(0,0,24,0)},0,0);
        var panel=new TableLayoutPanel{Dock=DockStyle.Fill,ColumnCount=1,RowCount=5,Padding=new Padding(12,12,8,0)};
        panel.RowStyles.Add(new RowStyle(SizeType.Absolute,30));panel.RowStyles.Add(new RowStyle(SizeType.Absolute,65));panel.RowStyles.Add(new RowStyle(SizeType.Percent,100));panel.RowStyles.Add(new RowStyle(SizeType.Absolute,98));panel.RowStyles.Add(new RowStyle(SizeType.Absolute,62));layout.Controls.Add(panel,1,0);
        panel.Controls.Add(TextLabel("URBAN CHAOS  /  RIOT RESPONSE",11),0,0);
        var title=TextLabel("BURNER EDITION",28);title.ForeColor=Color.FromArgb(245,175,49);title.Font=new Font("Segoe UI",28,FontStyle.Bold);panel.Controls.Add(title,0,1);
        pages.Dock=DockStyle.Fill;pages.Controls.AddRange(new Control[]{home,source,manage});panel.Controls.Add(pages,0,2);
        heading.Text="PREPARE YOUR GAME";heading.Dock=DockStyle.Fill;heading.Font=new Font("Segoe UI",18,FontStyle.Bold);Add(home,heading,54);
        description.Text="Choose your Xbox ISO or reuse an existing installation. You can also place one ISO/XISO in the assets folder for automatic setup.";description.Dock=DockStyle.Fill;description.Font=new Font("Segoe UI",12);description.ForeColor=Color.FromArgb(190,196,190);Add(home,description,116);
        play=MakeButton("PLAY",async(s,e)=>await Play(),true);select=MakeButton("SELECT GAME",(s,e)=>ShowPage(source),true);
        var primary=new Panel{Dock=DockStyle.Fill,Margin=new Padding(0,4,0,4)};primary.Controls.AddRange(new Control[]{play,select});Add(home,primary,64);
        Add(home,MakeButton("MANAGE",(s,e)=>ShowPage(manage)),64);
        Add(source,MakeButton("BACK",(s,e)=>ShowPage(home)),44);
        Add(source,MakeButton("USE ISO DIRECTLY",async(s,e)=>await SelectIso(true),true),54);
        Add(source,TextLabel("Play from your ISO. Keep it in place; only required preparation files are copied."),60);
        Add(source,MakeButton("IMPORT ONCE",async(s,e)=>await SelectIso(false)),54);
        Add(source,TextLabel("Copy all game files once. You can remove the ISO after setup."),50);
        Add(source,MakeButton("USE EXISTING INSTALL",async(s,e)=>await Existing()),54);
        Add(source,TextLabel("Reuse a previous launcher or GameData folder, including its saves and add-ons."),60);
        Add(manage,MakeButton("BACK",(s,e)=>ShowPage(home)),42);
        updates=MakeButton("CHECK FOR UPDATES",async(s,e)=>await UpdateRelease());Add(manage,updates,46);
        Add(manage,MakeButton("SELECT GAME / REPAIR FROM ISO",(s,e)=>ShowPage(source)),46);
        verify=MakeButton("VERIFY FILES",async(s,e)=>await Operation("verify",""));Add(manage,verify,46);
        var folders=new TableLayoutPanel{Dock=DockStyle.Fill,ColumnCount=2};folders.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,50));folders.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,50));
        folder=MakeButton("GAME FOLDER",(s,e)=>OpenFolder(data));saves=MakeButton("SAVES",(s,e)=>OpenFolder(Path.Combine(data,"xbe-runtime","hdd")));folders.Controls.Add(folder,0,0);folders.Controls.Add(saves,1,0);Add(manage,folders,46);
        textures=MakeButton("INSTALL HD TEXTURE PACK",async(s,e)=>await SelectAddon(false));Add(manage,textures,46);
        voices=MakeButton("IMPORT SPANISH VOICES (PS2 ISO)",async(s,e)=>await SelectAddon(true));voices.Font=new Font("Segoe UI",10,FontStyle.Bold);Add(manage,voices,46);
        Add(manage,TextLabel("LANGUAGE",10),28);
        language.DropDownStyle=ComboBoxStyle.DropDownList;language.Dock=DockStyle.Fill;language.AccessibleName="Game language";
        language.Items.AddRange(new object[]{"English","Français","Italiano","Deutsch","Español (ZARF beta)"});language.SelectedIndex=0;Add(manage,language,36);
        language.SelectionChangeCommitted+=(s,e)=>{if(language.SelectedIndex==4&&!spanishAvailable){status.Text="Set up your Xbox game first. Spanish text is included; optional voices come from your European PS2 ISO.";language.SelectedIndex=0;}else status.Text="Language will be saved when you press PLAY.";};
        Add(manage,TextLabel("Spanish text is included. HD game textures and PS2 voices are optional.",9),40);
        diagnostics=MakeButton("DIAGNOSTIC LOGGING: OFF",async(s,e)=>await Operation("diagnostics"," --logging "+(diagnosticLogging?"off":"on")));Add(manage,diagnostics,46);
        Add(manage,MakeButton("OPEN LOGS",(s,e)=>OpenFolder(Path.Combine(Root,"GameData","logs"))),46);
        Add(manage,TextLabel("Off by default. Enable before reproducing a problem, then share logs manually. Applies next game session.",9),48);
        int manageHeight=manage.Padding.Vertical;
        foreach(RowStyle row in manage.RowStyles)manageHeight+=(int)row.Height;
        manage.AutoScrollMinSize=new Size(0,manageHeight);
        foreach(var page in new[]{home,source,manage}){page.RowCount++;page.RowStyles.Add(new RowStyle(SizeType.Percent,100));}
        var feedback=new TableLayoutPanel{Dock=DockStyle.Fill,ColumnCount=1};feedback.RowStyles.Add(new RowStyle(SizeType.Percent,100));feedback.RowStyles.Add(new RowStyle(SizeType.Absolute,18));status.Text="Checking game data...";status.Dock=DockStyle.Fill;status.Font=new Font("Segoe UI",10);status.Padding=new Padding(0,8,0,0);feedback.Controls.Add(status,0,0);progress.Dock=DockStyle.Fill;progress.Visible=false;feedback.Controls.Add(progress,0,1);panel.Controls.Add(feedback,0,3);
        var footer=new TableLayoutPanel{Dock=DockStyle.Fill,ColumnCount=2};footer.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,70));footer.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,30));footer.Controls.Add(TextLabel("v0.11  |  Windows x64 / Direct3D 12\nNo disc files are included.",9),0,0);
        cancel=MakeButton("QUIT",(s,e)=>{if(busy&&!running&&cancelPath!=null){File.WriteAllText(cancelPath,"");status.Text="Cancelling safely...";cancel.Enabled=false;}else if(!busy)Close();});footer.Controls.Add(cancel,1,0);panel.Controls.Add(footer,0,4);
        ApplyStatus(new Dictionary<string,object>{{"ready",false}});ShowPage(home);
#if !LAUNCHER_PREVIEW
        Shown+=async(s,e)=>{busy=true;SetEnabled();try{await RefreshReady();busy=false;if(!ready)await Operation("auto-setup","");}catch(Exception ex){status.Text=ex.Message;}finally{busy=false;SetEnabled();}};
#endif
        KeyDown+=(s,e)=>{if(e.KeyCode==Keys.Escape&&!busy){ShowPage(home);e.Handled=true;}};
        FormClosing+=(s,e)=>{if(busy){e.Cancel=true;status.Text=running?"Close the game before exiting the launcher.":"Use Cancel to stop setup safely.";}};
    }
    void ApplyStatus(Dictionary<string,object> row){
        ready=Field(row,"ready")=="True";data=Field(row,"data",data);play.Visible=ready;select.Visible=!ready;
        diagnosticLogging=Field(row,"diagnostic_logging")=="True";diagnostics.Text="DIAGNOSTIC LOGGING: "+(diagnosticLogging?"ON":"OFF");
        heading.Text=ready?"READY TO PLAY":"PREPARE YOUR GAME";
        description.Text=ready?(Field(row,"source_mode")=="iso"?"Your Xbox ISO is connected. Keep it in place for play and updates.":"Your installed game is ready. No ISO is needed to play."):"Choose your Xbox ISO or reuse an existing installation. You can also place one ISO/XISO in the assets folder for automatic setup.";
        status.Text=Field(row,"message",ready?"Ready to play.":"Select Game to get started.");SetEnabled();
    }
    void SetEnabled(){pages.Enabled=!busy;play.Enabled=!busy&&ready;textures.Enabled=voices.Enabled=verify.Enabled=language.Enabled=!busy&&ready;folder.Enabled=saves.Enabled=!busy&&Directory.Exists(data);cancel.Text=busy&&!running?"CANCEL":"QUIT";cancel.Enabled=!busy||(!running&&cancelPath!=null);progress.Visible=busy&&!running;}
    async Task RefreshReady(){
        var result=await ReadWorker("status");if(result.Item1!=0&&result.Item1!=2)throw new InvalidOperationException(Field(Row(result.Item2),"message","Unable to check installation. "+result.Item3));
        ApplyStatus(Row(result.Item2));
        if(ready){var resultLanguage=await ReadWorker("language");if(resultLanguage.Item1==0){var row=Row(resultLanguage.Item2);int selected;if(Int32.TryParse(Field(row,"language"),out selected)&&selected>=0&&selected<language.Items.Count)language.SelectedIndex=selected;spanishAvailable=Field(row,"spanish_available")=="True";}}
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
        try{int code=await RunWorker(action,extra+" --cancel "+Quote(cancelPath));string message=status.Text;await RefreshReady();if(code!=0)status.Text=message;else{if(action=="voices")language.SelectedIndex=4;if(action=="verify")status.Text="File verification completed successfully.";else if(action=="diagnostics")status.Text=message;else ShowPage(home);}}
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
