# NOCTIS: lanza PPSSPP EN PANTALLA (al frente) y lo deja corriendo para jugar en el PC.
param([switch]$Build)
$ErrorActionPreference = 'SilentlyContinue'
Add-Type @"
using System; using System.Runtime.InteropServices;
public class WShow {
  public delegate bool P(IntPtr h, IntPtr l);
  [DllImport("user32.dll")] public static extern bool EnumWindows(P cb, IntPtr l);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out R r);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int n);
  [DllImport("user32.dll")] public static extern bool MoveWindow(IntPtr h, int x, int y, int w, int ht, bool rp);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool BringWindowToTop(IntPtr h);
  public struct R { public int L, T, Rr, B; }
  public static IntPtr Big(uint pid) {
    IntPtr best = IntPtr.Zero; int bw = -1;
    EnumWindows((h,l) => { uint p; GetWindowThreadProcessId(h, out p);
      if (p==pid) { R r; GetWindowRect(h, out r); int w=r.Rr-r.L; if (w>bw) { bw=w; best=h; } }
      return true; }, IntPtr.Zero);
    return best;
  }
}
"@
$repo  = "C:\Users\Usuario\Desktop\Proyectos_Claude\noctis-psp"
$eboot = "$repo\build\EBOOT.PBP"
$exe   = "$repo\ppsspp\PPSSPPWindows64.exe"

Get-Process PPSSPPWindows64 | Stop-Process -Force
Start-Sleep -Milliseconds 400

if ($Build) {
  $b = wsl -d Ubuntu-24.04 -u root -- bash -c 'cd /mnt/c/Users/Usuario/Desktop/Proyectos_Claude/noctis-psp && bash tools/build.sh'
  if (($b -join "`n") -notmatch 'Built target noctis') { Write-Output "BUILD FAIL"; exit }
  Write-Output "BUILD OK"
}

$proc = Start-Process -FilePath $exe -ArgumentList "`"$eboot`"" -PassThru
Start-Sleep -Seconds 6
$p = Get-Process -Id $proc.Id -ErrorAction SilentlyContinue
if (-not $p) { Write-Output "PPSSPP se cerro (crash?)"; exit }

# trae la ventana a pantalla (60,60), restaura y al frente
$h = [WShow]::Big([uint32]$p.Id)
[WShow]::ShowWindow($h, 9) | Out-Null          # SW_RESTORE
Start-Sleep -Milliseconds 300
[WShow]::MoveWindow($h, 60, 60, 1225, 750, $true) | Out-Null
[WShow]::BringWindowToTop($h) | Out-Null
[WShow]::SetForegroundWindow($h) | Out-Null
$r = New-Object WShow+R; [WShow]::GetWindowRect($h, [ref]$r) | Out-Null
Write-Output ("EN PANTALLA en ({0},{1}) tam {2}x{3} - PID {4}" -f $r.L, $r.T, ($r.Rr-$r.L), ($r.B-$r.T), $p.Id)
