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

class LauncherChecks {
    static object Field(BurnerLauncher form,string name){return typeof(BurnerLauncher).GetField(name,BindingFlags.Instance|BindingFlags.NonPublic).GetValue(form);}
    static void Status(BurnerLauncher form,bool ready,string data,string mode="iso"){
        typeof(BurnerLauncher).GetMethod("ApplyStatus",BindingFlags.Instance|BindingFlags.NonPublic).Invoke(form,new object[]{new Dictionary<string,object>{{"ready",ready},{"data",data},{"source_mode",mode},{"message",ready?"Verified":"ISO moved or missing"}}});
    }
    static void Check(bool value,string message){if(!value)throw new Exception(message);}
    static void Until(Func<bool> condition){var timer=Stopwatch.StartNew();while(!condition()){Check(timer.ElapsedMilliseconds<10000,"UI worker timed out");Application.DoEvents();Thread.Sleep(5);}}
    [STAThread] static int Main(string[] args){try{return Run(args);}catch(Exception error){Console.Error.WriteLine(error.Message);return 1;}}
    static int Run(string[] args){
        Console.OutputEncoding=new UTF8Encoding(false);
        if(args.Length>2&&args[0]=="-B"){
            string root=Directory.GetParent(Path.GetDirectoryName(args[1])).FullName;
            if(args[2]=="language"){
                File.WriteAllText(Path.Combine(root,"language-check-started"),"");
                var timer=Stopwatch.StartNew();while(!File.Exists(Path.Combine(root,"language-check-release"))){if(timer.ElapsedMilliseconds>10000)return 2;Thread.Sleep(5);}
            }
            Console.WriteLine(new JavaScriptSerializer().Serialize(new Dictionary<string,object>{{"ready",true},{"data",root},{"language",0},{"spanish_available",true},{"message","Verified"}}));return 0;
        }
        if(args.Length>0&&args[0]=="echo"){Console.WriteLine(new JavaScriptSerializer().Serialize(args));return 0;}
        Application.EnableVisualStyles();Application.SetCompatibleTextRenderingDefault(false);
        using(var form=new BurnerLauncher()){
            form.ShowInTaskbar=false;form.StartPosition=FormStartPosition.Manual;form.Location=new Point(-32000,-32000);form.Show();Application.DoEvents();
            var play=(Button)Field(form,"play");var select=(Button)Field(form,"select");
            Check(select.Visible&&!play.Visible&&!((Button)Field(form,"textures")).Enabled,"Unprepared source exposed Play or add-ons");
            Check(((Button)Field(form,"diagnostics")).Text.EndsWith("OFF"),"Diagnostics must default off");
            Check(((Button)Field(form,"updates")).Text=="CHECK FOR UPDATES"&&((Button)Field(form,"updates")).Enabled,"Updates must be available without a disc import");
            typeof(BurnerLauncher).GetMethod("ApplyStatus",BindingFlags.Instance|BindingFlags.NonPublic).Invoke(form,new object[]{new Dictionary<string,object>{{"ready",false},{"diagnostic_logging",true}}});
            Check(((Button)Field(form,"diagnostics")).Text.EndsWith("ON")&&((Button)Field(form,"diagnostics")).Enabled,"Saved diagnostics unavailable before setup");
            var manage=(TableLayoutPanel)Field(form,"manage");
            foreach(var size in new[]{new Size(880,680),new Size(1056,779)}){
                form.Size=size;
                typeof(BurnerLauncher).GetMethod("ShowPage",BindingFlags.Instance|BindingFlags.NonPublic).Invoke(form,new object[]{manage});
                form.PerformLayout();Application.DoEvents();manage.AutoScrollPosition=new Point(0,10000);Application.DoEvents();
                Check(manage.Controls[manage.Controls.Count-1].Bottom<=manage.ClientSize.Height,"Diagnostic guidance cannot be reached by scrolling");
            }
            typeof(BurnerLauncher).GetMethod("ShowPage",BindingFlags.Instance|BindingFlags.NonPublic).Invoke(form,new[]{Field(form,"home")});
            select.PerformClick();Check(((Control)Field(form,"source")).Visible,"Select Game did not open source choices");
            typeof(BurnerLauncher).GetMethod("ShowPage",BindingFlags.Instance|BindingFlags.NonPublic).Invoke(form,new[]{Field(form,"home")});
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
                    File.WriteAllText(release,"");Until(()=>task.IsCompleted);task.GetAwaiter().GetResult();
                    Check(play.Enabled,"Controls did not recover after operation");
                }
            }finally{
                File.WriteAllText(release,"");File.Delete(Path.Combine(python,"python.exe"));Directory.Delete(python);Directory.Delete(tools);File.Delete(started);File.Delete(release);
            }
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
        Console.WriteLine("Launcher source states, overlap guard, recovery and real Windows argv round trips passed.");return 0;
    }
}
