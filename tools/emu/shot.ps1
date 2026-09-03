# NOCTIS: compila (opcional), lanza PPSSPP fuera de pantalla y captura robusto.
param([switch]$Build)
$ErrorActionPreference = 'SilentlyContinue'
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System; using System.Runtime.InteropServices;
public class W3 {
  public delegate bool P(IntPtr h, IntPtr l);
  [DllImport("user32.dll")] public static extern bool EnumWindows(P cb, IntPtr l);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out R r);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int n);
  [DllImport("user32.dll")] public static extern bool MoveWindow(IntPtr h, int x, int y, int w, int ht, bool rp);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint f);
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
$out   = "$repo\build\noctis.png"

Get-Process PPSSPPWindows64 | Stop-Process -Force
Start-Sleep -Milliseconds 400

if ($Build) {
  $b = wsl -d Ubuntu-24.04 -u root -- bash -c 'cd /mnt/c/Users/Usuario/Desktop/Proyectos_Claude/noctis-psp && bash tools/build.sh'
  if (($b -join "`n") -notmatch 'Built target noctis') {
    Write-Output "BUILD FAIL:"; Write-Output (($b | Select-Object -Last 12) -join "`n"); exit
  }
  Write-Output "BUILD OK"
}

$proc = Start-Process -FilePath $exe -ArgumentList "`"$eboot`"" -PassThru
Start-Sleep -Seconds 7
$p = Get-Process -Id $proc.Id -ErrorAction SilentlyContinue
if (-not $p) { Write-Output "PPSSPP se cerro (crash?)"; exit }

# encuentra la ventana del juego (la mas ancha del proceso), restaura y fija tamano
$h = [W3]::Big([uint32]$p.Id)
[W3]::ShowWindow($h, 9) | Out-Null
Start-Sleep -Milliseconds 300
[W3]::MoveWindow($h, -2600, 80, 980, 600, $true) | Out-Null
Start-Sleep -Milliseconds 1500
# re-busca por si el handle valido cambio tras el resize
$h = [W3]::Big([uint32]$p.Id)
$r = New-Object W3+R; [W3]::GetWindowRect($h, [ref]$r) | Out-Null
$w = $r.Rr - $r.L; $ht = $r.B - $r.T
if ($w -lt 400) { [W3]::MoveWindow($h, -2600, 80, 980, 600, $true) | Out-Null; Start-Sleep -Seconds 1; $r = New-Object W3+R; [W3]::GetWindowRect($h, [ref]$r) | Out-Null; $w = $r.Rr - $r.L; $ht = $r.B - $r.T }
$cx = [int]($w/2); $cy = [int]($ht/2)
$saved = $false
for ($k = 0; $k -lt 12 -and -not $saved; $k++) {
  Start-Sleep -Milliseconds 500
  $bmp = New-Object System.Drawing.Bitmap($w, $ht)
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $hdc = $g.GetHdc(); [W3]::PrintWindow($h, $hdc, 2) | Out-Null; $g.ReleaseHdc($hdc)
  # con software renderer la captura es fiable; rechaza solo el frame BLANCO de
  # carga. Toma un par de frames de calentamiento primero.
  $px = $bmp.GetPixel($cx, $cy)
  $white = ($px.R -gt 235 -and $px.G -gt 235 -and $px.B -gt 235)
  if (-not $white -and $k -ge 3) { $bmp.Save($out, [System.Drawing.Imaging.ImageFormat]::Png); $saved = $true }
  $g.Dispose(); $bmp.Dispose()
}
Write-Output ("SHOT {0}x{1} saved={2}" -f $w, $ht, $saved)
