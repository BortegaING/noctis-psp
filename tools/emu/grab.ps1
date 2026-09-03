$ErrorActionPreference='SilentlyContinue'
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;using System.Runtime.InteropServices;
public class WG{
 public delegate bool P(IntPtr h,IntPtr l);
 [DllImport("user32.dll")] public static extern bool EnumWindows(P cb,IntPtr l);
 [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h,out uint pid);
 [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h,out R r);
 public struct R{public int L,T,Rr,B;}
 public static IntPtr Big(uint pid){IntPtr b=IntPtr.Zero;int bw=-1;
  EnumWindows((h,l)=>{uint p;GetWindowThreadProcessId(h,out p);
   if(p==pid){R r;GetWindowRect(h,out r);int w=r.Rr-r.L;if(w>bw){bw=w;b=h;}}return true;},IntPtr.Zero);
  return b;}
}
"@
$p=Get-Process PPSSPPWindows64 -ErrorAction SilentlyContinue | Select-Object -First 1
if(-not $p){"NO_PPSSPP";exit}
$h=[WG]::Big([uint32]$p.Id)
$r=New-Object WG+R;[WG]::GetWindowRect($h,[ref]$r)|Out-Null
$w=$r.Rr-$r.L;$ht=$r.B-$r.T
if($w -lt 100){"WIN_TOO_SMALL $w x $ht";exit}
$bmp=New-Object System.Drawing.Bitmap($w,$ht)
$g=[System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($r.L,$r.T,0,0,(New-Object System.Drawing.Size($w,$ht)))
$out="C:\Users\Usuario\Desktop\Proyectos_Claude\noctis-psp\build\screen.png"
$bmp.Save($out,[System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose();$bmp.Dispose()
"GRABBED ${w}x${ht} at ($($r.L),$($r.T))"
