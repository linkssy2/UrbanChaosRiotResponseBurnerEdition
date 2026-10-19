using System;
using System.IO;
using System.Drawing;
using System.Windows.Forms;
using System.Diagnostics;
using System.Threading.Tasks;
using System.Web.Script.Serialization;
using System.Collections.Generic;

class BurnerLauncher : Form {
    static readonly string Root=AppDomain.CurrentDomain.BaseDirectory;
    static readonly string Data=Path.Combine(Root,"GameData");
    readonly Label status=new Label(); readonly ProgressBar progress=new ProgressBar();
    readonly Button play=new Button(), import=new Button(), cancel=new Button();
    bool busy, running; string cancelPath; Image cover;
    static string Quote(string s){return "\""+s.Replace("\"","\\\"")+"\"";}
    // Resolve beside the EXE so shortcuts and moving the folder keep the same portable data.
    static ProcessStartInfo Worker(string action,string extra="") {
        var p=new ProcessStartInfo(Path.Combine(Root,"tools","python","python.exe"),"-B "+Quote(Path.Combine(Root,"tools","import_game.py"))+" "+action+" --data "+Quote(Data)+extra);
        p.UseShellExecute=false;p.CreateNoWindow=true;p.RedirectStandardOutput=true;p.RedirectStandardError=true;p.WorkingDirectory=Root;return p;
    }
    static string ErrorMessage(string output,string error){
        string message="The game could not start.";
        foreach(string line in output.Split('\n')){try{var row=new JavaScriptSerializer().Deserialize<Dictionary<string,object>>(line);if(row.ContainsKey("message"))message=Convert.ToString(row["message"]);}catch{}}
        return String.IsNullOrWhiteSpace(error)?message:message+"\n"+error.Trim();
    }
    static async Task<bool> Ready(){using(var p=Process.Start(Worker("status"))){await Task.Run(()=>p.WaitForExit());return p.ExitCode==0;}}
    Button MakeButton(string text,EventHandler handler) {
        var b=new Button{Text=text,Dock=DockStyle.Fill,Height=56,FlatStyle=FlatStyle.Flat,BackColor=Color.FromArgb(37,38,36),ForeColor=Color.White,Font=new Font("Segoe UI",13,FontStyle.Bold),Margin=new Padding(0,6,0,6)};
        b.FlatAppearance.BorderColor=Color.FromArgb(115,112,96);b.Click+=handler;return b;
    }
    public BurnerLauncher() {
        Text="Urban Chaos: Riot Response - Burner Edition";ClientSize=new Size(1040,700);MinimumSize=new Size(880,620);StartPosition=FormStartPosition.CenterScreen;BackColor=Color.FromArgb(16,18,18);ForeColor=Color.White;AutoScaleMode=AutoScaleMode.Dpi;
        Icon=Icon.ExtractAssociatedIcon(Application.ExecutablePath);
        var layout=new TableLayoutPanel{Dock=DockStyle.Fill,ColumnCount=2,Padding=new Padding(20)};layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,45));layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,55));Controls.Add(layout);
        cover=Image.FromFile(Path.Combine(Root,"cover.png"));layout.Controls.Add(new PictureBox{Image=cover,SizeMode=PictureBoxSizeMode.Zoom,Dock=DockStyle.Fill,Margin=new Padding(0,0,24,0)},0,0);
        var panel=new TableLayoutPanel{Dock=DockStyle.Fill,ColumnCount=1,RowCount=10,Padding=new Padding(12,22,8,16)};
        int[] heights={40,66,68,70,30,66,66,58,54};foreach(int h in heights)panel.RowStyles.Add(new RowStyle(SizeType.Absolute,h));panel.RowStyles.Add(new RowStyle(SizeType.Percent,100));layout.Controls.Add(panel,1,0);
        panel.Controls.Add(new Label{Text="URBAN CHAOS  /  RIOT RESPONSE",Dock=DockStyle.Fill,ForeColor=Color.FromArgb(190,190,169),Font=new Font("Segoe UI",11,FontStyle.Bold)});
        panel.Controls.Add(new Label{Text="BURNER EDITION",Dock=DockStyle.Fill,ForeColor=Color.FromArgb(245,175,49),Font=new Font("Segoe UI",29,FontStyle.Bold)});
        panel.Controls.Add(new Label{Text="Your Xbox disc. Ready for PC.\nPortable game data and saves stay in GameData.",Dock=DockStyle.Fill,Font=new Font("Segoe UI",12),ForeColor=Color.FromArgb(206,207,199)});
        status.Text="Checking game data...";status.Dock=DockStyle.Fill;status.Font=new Font("Segoe UI",11);panel.Controls.Add(status);
        progress.Dock=DockStyle.Fill;progress.Margin=new Padding(0,0,0,10);panel.Controls.Add(progress);
        play=MakeButton("PLAY",async(s,e)=>await Play());play.Enabled=false;play.BackColor=Color.FromArgb(138,56,19);panel.Controls.Add(play);
        import=MakeButton("SELECT XBOX ISO",async(s,e)=>await Import());panel.Controls.Add(import);
        panel.Controls.Add(MakeButton("OPEN SAVES",(s,e)=>{string saves=Path.Combine(Data,"xbe-runtime","hdd");if(Directory.Exists(saves))Process.Start("explorer.exe",Quote(saves));else status.Text="No portable saves yet. Import your ISO and create a profile first.";}));
        cancel=MakeButton("QUIT",(s,e)=>{if(busy&&!running){File.WriteAllText(cancelPath,"");status.Text="Cancelling import...";cancel.Enabled=false;}else Close();});panel.Controls.Add(cancel);
        panel.Controls.Add(new Label{Text="v0.05  |  Windows x64 / Direct3D 12\nPortable edition. No disc files are included.",Dock=DockStyle.Fill,Font=new Font("Segoe UI",9),ForeColor=Color.FromArgb(138,143,138),TextAlign=ContentAlignment.BottomLeft});
        Shown+=async(s,e)=>await RefreshReady();FormClosing+=(s,e)=>{if(busy){e.Cancel=true;status.Text=running?"Close the game before exiting the launcher.":"Use Cancel Import to stop safely.";}};
    }
    async Task RefreshReady(){bool ready=await Ready();play.Enabled=ready;status.Text=ready?"READY TO PLAY\nYour imported game data is available.":"SELECT YOUR XBOX ISO\nChoose the supported Urban Chaos: Riot Response disc image.";}
    // Drain both output streams while the worker runs; progress is newline-delimited JSON.
    async Task<int> RunWorker(string action,string extra) {
        using(var p=new Process{StartInfo=Worker(action,extra)}) {
            p.OutputDataReceived+=(s,e)=>{if(e.Data==null)return;try{var v=new JavaScriptSerializer().Deserialize<Dictionary<string,object>>(e.Data);BeginInvoke((Action)(()=>{status.Text=Convert.ToString(v["message"]);progress.Value=Math.Max(0,Math.Min(100,Convert.ToInt32(v["percent"])));}));}catch{}};
            p.Start();p.BeginOutputReadLine();var errors=p.StandardError.ReadToEndAsync();await Task.Run(()=>p.WaitForExit());string error=await errors;
            if(p.ExitCode!=0&&!String.IsNullOrWhiteSpace(error))status.Text="Setup could not finish. "+error.Trim();return p.ExitCode;
        }
    }
    void Busy(bool value,bool game=false){busy=value;running=value&&game;import.Enabled=!value;play.Enabled=!value;cancel.Text=value&&!game?"CANCEL IMPORT":"QUIT";cancel.Enabled=true;}
    async Task Import(){using(var d=new OpenFileDialog{Title="Select your Urban Chaos Xbox ISO",Filter="Xbox disc image (*.iso)|*.iso",CheckFileExists=true}){
        if(d.ShowDialog(this)!=DialogResult.OK)return;cancelPath=Path.Combine(Data,"cancel-"+Guid.NewGuid().ToString("N"));Busy(true);progress.Value=0;
        try{Directory.CreateDirectory(Data);int code=await RunWorker("import"," --iso "+Quote(d.FileName)+" --cancel "+Quote(cancelPath));Busy(false);if(code==0)await RefreshReady();else play.Enabled=await Ready();}
        catch(Exception ex){Busy(false);status.Text=ex.Message;}finally{if(File.Exists(cancelPath))File.Delete(cancelPath);}
    }}
    async Task Play(){Busy(true,true);status.Text="Game running. Your settings and saves stay in this folder's GameData.";
        try{int code=await RunWorker("launch","");Busy(false);if(code==0)await RefreshReady();else play.Enabled=await Ready();}catch(Exception ex){Busy(false);status.Text=ex.Message;}}
    protected override void Dispose(bool disposing){if(disposing&&cover!=null)cover.Dispose();base.Dispose(disposing);}
    [STAThread] static int Main(string[] args) {
        Application.EnableVisualStyles();Application.SetCompatibleTextRenderingDefault(false);
        bool first;using(var mutex=new System.Threading.Mutex(true,"Local\\UrbanChaosBurnerEdition",out first)){
            if(!first){MessageBox.Show("Burner Edition is already open.","Burner Edition");return 1;}
            try{
#if DIRECT
                if(Ready().GetAwaiter().GetResult()) {using(var p=Process.Start(Worker("launch"))){var output=p.StandardOutput.ReadToEndAsync();var error=p.StandardError.ReadToEndAsync();p.WaitForExit();if(p.ExitCode!=0)MessageBox.Show(ErrorMessage(output.Result,error.Result),"Burner Edition",MessageBoxButtons.OK,MessageBoxIcon.Error);return p.ExitCode;}}
#endif
                using(var window=new BurnerLauncher()){
#if LAUNCHER_PREVIEW
                    if(args.Length==2&&args[0]=="--preview"){window.ShowInTaskbar=false;window.StartPosition=FormStartPosition.Manual;window.Location=new Point(-32000,-32000);window.Show();window.PerformLayout();Application.DoEvents();using(var bitmap=new Bitmap(window.Width,window.Height)){window.DrawToBitmap(bitmap,new Rectangle(Point.Empty,bitmap.Size));bitmap.Save(args[1]);}return 0;}
#endif
                    Application.Run(window);
                }
                return 0;
            }catch(Exception ex){MessageBox.Show(ex.Message,"Burner Edition",MessageBoxButtons.OK,MessageBoxIcon.Error);return 1;}
        }
    }
}
