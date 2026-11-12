using System;
using System.IO;
using System.Drawing;
using System.Diagnostics;
using System.Reflection;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Collections.Generic;
using System.Web.Script.Serialization;

class LauncherFixtureForm : BurnerLauncher {
    protected override bool ShowWithoutActivation { get { return true; } }
}
class LauncherChecks {
    static object Field(BurnerLauncher form,string name){return typeof(BurnerLauncher).GetField(name,BindingFlags.Instance|BindingFlags.NonPublic).GetValue(form);}
    static void Status(BurnerLauncher form,bool ready,string data,string mode="iso"){
        typeof(BurnerLauncher).GetMethod("ApplyStatus",BindingFlags.Instance|BindingFlags.NonPublic).Invoke(form,new object[]{new Dictionary<string,object>{{"ready",ready},{"data",data},{"source_mode",mode},{"message",ready?"Verified":"ISO moved or missing"}}});
    }
    static void Check(bool value,string message){if(!value)throw new Exception(message);}
    static void Until(Func<bool> condition){var timer=Stopwatch.StartNew();while(!condition()){Check(timer.ElapsedMilliseconds<10000,"UI worker timed out");Application.DoEvents();Thread.Sleep(5);}}
    static IEnumerable<Control> Descendants(Control parent){foreach(Control child in parent.Controls){yield return child;foreach(Control descendant in Descendants(child))yield return descendant;}}
    static Button ButtonNamed(Control parent,string text){
        foreach(Control control in Descendants(parent)){var button=control as Button;if(button!=null&&String.Equals(button.Text,text,StringComparison.OrdinalIgnoreCase))return button;}
        throw new Exception("Missing launcher action: "+text);
    }
    static void Show(BurnerLauncher form,string page){typeof(BurnerLauncher).GetMethod("ShowPage",BindingFlags.Instance|BindingFlags.NonPublic).Invoke(form,new[]{Field(form,page)});}
    static void Reachable(BurnerLauncher form){
        form.PerformLayout();Application.DoEvents();
        var actions=new List<Control>();
        foreach(Control control in Descendants(form))if(control.Visible&&(control is Button||control is ComboBox)){
            actions.Add(control);
            for(Control parent=control.Parent;parent!=null&&parent!=form;parent=parent.Parent){var scroll=parent as ScrollableControl;if(scroll!=null&&scroll.AutoScroll)scroll.ScrollControlIntoView(control);}
            Application.DoEvents();
            Check(control.Width>0&&control.Height>0,"Empty action bounds: "+control.Text);
            for(Control parent=control.Parent;parent!=null&&parent!=form;parent=parent.Parent){
                var bounds=parent.RectangleToClient(control.RectangleToScreen(control.ClientRectangle));
                var scroll=parent as ScrollableControl;
                Check(parent.ClientRectangle.Contains(bounds),"Action clipped after scrolling: "+control.Text+" bounds="+bounds+" in "+parent.GetType().Name+" name="+parent.Name+" parentBounds="+parent.Bounds+" client="+parent.ClientRectangle+" display="+parent.DisplayRectangle+" scroll="+(scroll==null?"none":scroll.AutoScrollPosition.ToString())+" form="+form.Size+" dpi="+form.CurrentAutoScaleDimensions);
            }
            if(control is Button)Check(TextRenderer.MeasureText(control.Text,control.Font,new Size(Math.Max(1,control.ClientSize.Width-control.Padding.Horizontal-8),Int32.MaxValue),TextFormatFlags.WordBreak).Height<=control.ClientSize.Height-control.Padding.Vertical,"Action label clipped: "+control.Text);
            if(control.Enabled)Check(control.TabStop&&control.CanSelect,"Action cannot receive keyboard focus: "+control.Text);
        }
        var tabbed=new HashSet<Control>();form.ActiveControl=null;
        for(int i=0;i<200;i++){if(!form.SelectNextControl(form.ActiveControl,true,true,true,true))break;if(!tabbed.Add(form.ActiveControl))break;}
        foreach(Control action in actions)if(action.Enabled)Check(tabbed.Contains(action),"Action missing from keyboard navigation: "+action.Text);
    }
    static void LayoutChecks(){
        // Explicit scaling exercises layout arithmetic; live monitor-DPI acceptance is separate.
        foreach(float scale in new[]{1f,1.25f,1.5f})using(var form=new LauncherFixtureForm()){
            form.ShowInTaskbar=false;form.StartPosition=FormStartPosition.Manual;form.Location=new Point(-32000,-32000);form.Show();
            if(scale!=1f)form.Scale(new SizeF(scale,scale));
            foreach(var size in new[]{new Size(880,680),new Size(1056,779)}){
                form.Size=new Size((int)Math.Round(size.Width*scale),(int)Math.Round(size.Height*scale));
                Status(form,true,AppDomain.CurrentDomain.BaseDirectory);
                Show(form,"home");Reachable(form);
                var homePlay=(Button)Field(form,"play");var homeManage=ButtonNamed((Control)Field(form,"home"),"MANAGE");
                Check(homePlay.Height==homeManage.Height&&Math.Abs(homePlay.Width-homeManage.Width)<=1,"Home actions differ in size: PLAY="+homePlay.Size+" MANAGE="+homeManage.Size+" form="+form.Size+" scale="+scale);
                homeManage.PerformClick();
                var manage=(Control)Field(form,"manage");Check(manage.Visible,"Manage navigation did not open");
                string[] names={"Settings","Game files","Add-ons","DLC / Prototypes","Updates","Support"};
                var sections=(TableLayoutPanel[])Field(form,"manageSections");Check(sections.Length==names.Length,"Manage navigation lost a section");
                string[][] grouped={new[]{"language","skipStartup"},new[]{"verify","folder","saves"},new[]{"textures","voices"},new string[0],new[]{"updates"},new[]{"diagnostics"}};
                for(int i=0;i<names.Length;i++){
                    ButtonNamed(manage,names[i]).PerformClick();
                    for(int j=0;j<sections.Length;j++)Check(sections[j].Visible==(i==j),"Manage section selection is inconsistent: "+names[i]);
                    foreach(string field in grouped[i])Check(sections[i].Contains((Control)Field(form,field)),"Action moved outside its group: "+field);
                    Reachable(form);
                    var expectedNavigationSize=ButtonNamed(manage,names[0]).Size;
                    foreach(string name in names){var navigationButton=ButtonNamed(manage,name);Check(navigationButton.Size==expectedNavigationSize,"Navigation button stretched: "+name+" size="+navigationButton.Size+" expected="+expectedNavigationSize+" form="+form.Size+" scale="+scale);}
                }
                int unavailable=0;
                foreach(Control control in Descendants(sections[3])){
                    Check(control is Label,"Prototype preview must not expose an import or play action");
                    if(control.Text=="Not yet playable in Burner Edition.")unavailable++;
                }
                Check(unavailable==2,"Prototype preview lost its unavailable status");
                Check(ButtonNamed(sections[5],"OPEN LOGS").Enabled,"Support lost the logs action");
                ButtonNamed(manage,"BACK").PerformClick();Check(((Control)Field(form,"home")).Visible,"Manage Back did not return Home");
                Status(form,false,AppDomain.CurrentDomain.BaseDirectory);((Button)Field(form,"select")).PerformClick();
                var source=(Control)Field(form,"source");Check(source.Visible,"Select Game did not open source choices");
                foreach(string action in new[]{"USE ISO DIRECTLY","IMPORT ONCE","USE EXISTING INSTALL"})Check(ButtonNamed(source,action).Enabled,"Source choice unavailable: "+action);
                Reachable(form);
                ButtonNamed(source,"BACK").PerformClick();Check(((Control)Field(form,"home")).Visible,"Source Back did not return Home");
                Show(form,"manage");ButtonNamed(manage,"Game files").PerformClick();ButtonNamed(manage,"SELECT GAME / REPAIR").PerformClick();Check(source.Visible,"Repair did not reach source choices");
            }
            form.Close();
        }
    }
    [STAThread] static int Main(string[] args){try{return Run(args);}catch(Exception error){Console.Error.WriteLine(error);return 1;}}
    static int Run(string[] args){
        Console.OutputEncoding=new UTF8Encoding(false);
        if(args.Length>2&&args[0]=="-B"){
            string root=Directory.GetParent(Path.GetDirectoryName(args[1])).FullName;
            string startup=Path.Combine(root,"startup-choice");
            if(args[2]=="verify"&&File.Exists(Path.Combine(root,"fail-verify"))){Console.WriteLine("{\"message\":\"Fixture verification failed; select your source again.\"}");return 3;}
            if(args[2]=="startup"){
                Check(args.Length>4&&args[3]=="--skip-startup","Startup option not forwarded");
                File.WriteAllText(startup,args[4]);
            }
            if(args[2]=="language"){
                File.WriteAllText(Path.Combine(root,"language-check-started"),"");
                var timer=Stopwatch.StartNew();while(!File.Exists(Path.Combine(root,"language-check-release"))){if(timer.ElapsedMilliseconds>10000)return 2;Thread.Sleep(5);}
            }
            Console.WriteLine(new JavaScriptSerializer().Serialize(new Dictionary<string,object>{{"ready",true},{"data",root},{"language",0},{"spanish_available",true},{"skip_startup",File.Exists(startup)&&File.ReadAllText(startup)=="on"},{"message","Verified"}}));return 0;
        }
        if(args.Length>0&&args[0]=="echo"){Console.WriteLine(new JavaScriptSerializer().Serialize(args));return 0;}
        Application.EnableVisualStyles();Application.SetCompatibleTextRenderingDefault(false);
        using(var form=new LauncherFixtureForm()){
            Exception failure=null;
            form.ShowInTaskbar=false;form.StartPosition=FormStartPosition.Manual;form.Location=new Point(-32000,-32000);
            form.Shown+=(s,e)=>form.BeginInvoke(new Action(()=>{
            try{
            var play=(Button)Field(form,"play");var select=(Button)Field(form,"select");
            Check(select.Visible&&!play.Visible&&!((Button)Field(form,"textures")).Enabled,"Unprepared source exposed Play or add-ons");
            Check(((Button)Field(form,"diagnostics")).Text.EndsWith("OFF"),"Diagnostics must default off");
            var skipStartup=(Button)Field(form,"skipStartup");
            Check(skipStartup.Text=="SKIP STARTUP: OFF","Startup skipping must default off");
            Check(((Button)Field(form,"updates")).Text=="CHECK FOR UPDATES"&&((Button)Field(form,"updates")).Enabled,"Updates must be available without a disc import");
            typeof(BurnerLauncher).GetMethod("ApplyStatus",BindingFlags.Instance|BindingFlags.NonPublic).Invoke(form,new object[]{new Dictionary<string,object>{{"ready",false},{"diagnostic_logging",true},{"skip_startup",true}}});
            var diagnostics=(Button)Field(form,"diagnostics");
            Check(diagnostics.Text.EndsWith("ON")&&diagnostics.Enabled&&diagnostics.AccessibleName==diagnostics.Text,"Saved diagnostics unavailable before setup or misreported to accessibility");
            Check(skipStartup.Text=="SKIP STARTUP: ON"&&skipStartup.Enabled&&skipStartup.AccessibleName==skipStartup.Text,"Saved startup choice unavailable before setup");
            var manage=(TableLayoutPanel)Field(form,"manage");
            LayoutChecks();
            Show(form,"home");
            select.PerformClick();Check(((Control)Field(form,"source")).Visible,"Select Game did not open source choices");
            Show(form,"home");
            Status(form,true,AppDomain.CurrentDomain.BaseDirectory);
            Check(play.Visible&&play.Enabled&&!select.Visible,"Verified source did not enable Play");
            Check(((Label)Field(form,"description")).Text.Contains("Keep it in place"),"Direct ISO retention guidance missing");
            Status(form,true,AppDomain.CurrentDomain.BaseDirectory,"extracted");
            Check(((Label)Field(form,"description")).Text.Contains("No ISO is needed"),"Import-once guidance missing");
            typeof(BurnerLauncher).GetField("busy",BindingFlags.Instance|BindingFlags.NonPublic).SetValue(form,true);
            typeof(BurnerLauncher).GetMethod("SetEnabled",BindingFlags.Instance|BindingFlags.NonPublic).Invoke(form,null);
            Check(!play.Enabled&&!((Control)Field(form,"pages")).Enabled,"Setup must prevent overlapping actions");
            typeof(BurnerLauncher).GetField("busy",BindingFlags.Instance|BindingFlags.NonPublic).SetValue(form,false);
            Status(form,false,AppDomain.CurrentDomain.BaseDirectory);
            Check(!play.Visible&&select.Visible&&((Label)Field(form,"status")).Text.Contains("missing"),"Missing ISO should offer recovery, not Play");
            // Stall the actual awaited language child to exercise the post-operation race.
            string root=AppDomain.CurrentDomain.BaseDirectory,tools=Path.Combine(root,"tools"),python=Path.Combine(tools,"python");
            Check(!Directory.Exists(tools),"Use a fresh fixture directory, not a real package");Directory.CreateDirectory(python);
            string started=Path.Combine(root,"language-check-started"),release=Path.Combine(root,"language-check-release");
            try{
                File.Copy(Application.ExecutablePath,Path.Combine(python,"python.exe"));
                foreach(string operation in new[]{"Operation","Play"}){
                    File.Delete(started);File.Delete(release);Status(form,true,root);
                    var method=typeof(BurnerLauncher).GetMethod(operation,BindingFlags.Instance|BindingFlags.NonPublic);
                    var task=(Task)method.Invoke(form,operation=="Operation"?new object[]{"verify",""}:null);
                    Until(()=>File.Exists(started));
                    Check(!play.Enabled&&!((Control)Field(form,"pages")).Enabled,"Controls reopened during awaited language refresh");
                    var cancel=(Button)Field(form,"cancel");
                    Check(operation=="Operation"?cancel.Enabled&&cancel.Text=="CANCEL":!cancel.Enabled,"Cancel/exit availability did not follow worker ownership");
                    File.WriteAllText(release,"");Until(()=>task.IsCompleted);task.GetAwaiter().GetResult();
                    Check(play.Enabled,"Controls did not recover after operation");
                }
                Show(form,"manage");ButtonNamed(manage,"Game files").PerformClick();
                File.WriteAllText(Path.Combine(root,"fail-verify"),"");
                var failed=(Task)typeof(BurnerLauncher).GetMethod("Operation",BindingFlags.Instance|BindingFlags.NonPublic).Invoke(form,new object[]{"verify",""});
                Until(()=>failed.IsCompleted);failed.GetAwaiter().GetResult();
                Check(((Label)Field(form,"status")).Text.Contains("Fixture verification failed"),"Status refresh hid the worker failure");
                Check(!(bool)Field(form,"busy")&&play.Enabled&&manage.Enabled&&((Control)Field(form,"manageFiles")).Visible&&Field(form,"cancelPath")==null,"Failed setup did not restore its recovery controls");
                File.Delete(Path.Combine(root,"fail-verify"));
                ButtonNamed(manage,"Settings").PerformClick();
                var language=(ComboBox)Field(form,"language");language.SelectedIndex=4;
                File.Delete(started);
                foreach(string choice in new[]{"on","off"}){
                    skipStartup.PerformClick();
                    Until(()=>!(bool)Field(form,"busy"));
                    Check(File.ReadAllText(Path.Combine(root,"startup-choice"))==choice,"Startup toggle did not forward saved choice");
                    Check(skipStartup.Text=="SKIP STARTUP: "+choice.ToUpperInvariant(),"Startup toggle did not refresh saved state");
                    Check(manage.Visible&&((Control)Field(form,"manageSettings")).Visible,"Startup toggle left its Settings section");
                    Check(language.SelectedIndex==4&&!File.Exists(started),"Startup toggle overwrote unsaved Spanish selection");
                }
            }finally{
                File.WriteAllText(release,"");File.Delete(Path.Combine(python,"python.exe"));Directory.Delete(python);Directory.Delete(tools);File.Delete(started);File.Delete(release);File.Delete(Path.Combine(root,"startup-choice"));File.Delete(Path.Combine(root,"fail-verify"));
            }
            }catch(Exception error){failure=error;Console.Error.WriteLine("Launcher check failed before cleanup: "+error);}
            finally{form.Close();Application.ExitThread();}
            }));
            Application.Run(form);
            if(failure!=null)throw failure;
        }
        string[] samples={"","E:\\Game Folder\\","C:\\", "quoted \"file\"", "trailing\\\\", "Español & %data%"};
        string command="echo";foreach(string value in samples)command+=" "+BurnerLauncher.Quote(value);
        var start=new ProcessStartInfo(Application.ExecutablePath,command){UseShellExecute=false,CreateNoWindow=true,RedirectStandardOutput=true,RedirectStandardError=true,StandardOutputEncoding=Encoding.UTF8};
        using(var child=Process.Start(start)){
            var actual=new JavaScriptSerializer().Deserialize<string[]>(child.StandardOutput.ReadToEnd());child.WaitForExit();
            Check(child.ExitCode==0&&actual.Length==samples.Length+1,"argv child failed");
            for(int i=0;i<samples.Length;i++)Check(actual[i+1]==samples[i],"Windows argv corrupted path: "+samples[i]);
        }
        Check(!BurnerLauncher.Worker("status").Arguments.Contains("--data"),"Launcher overrides selected installation");
        Console.WriteLine("Launcher grouped navigation, scaled layouts, keyboard reachability, worker recovery, saved startup/language behavior and real Windows argv round trips passed.");return 0;
    }
}
