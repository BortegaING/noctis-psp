# NOCTIS: manda teclas a PPSSPP (ventana FUERA de pantalla, invisible aunque este activa) y captura.
# Uso: play.ps1 -Keys "I:1500,J:800,T:80" -Out nombre   (tecla:milisegundos que se mantiene)
# Mapa PPSSPP (controls.ini): I/K/J/L = nub arriba/abajo/izq/der; flechas = D-pad;
# T=Triangulo X=Cruz S=Cuadrado C=Circulo  Q=L  W=R  espacio=Start
# Activa la ventana off-screen (nadie la ve) para que PPSSPP acepte el teclado, manda con
# SendInput (scan codes) y devuelve el foco a la ventana que lo tenia.
param([string]$Keys = "", [string]$Out = "play")
$ErrorActionPreference = 'SilentlyContinue'
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System; using System.Runtime.InteropServices;
public class WP {
  public delegate bool P(IntPtr h, IntPtr l);
  [DllImport("user32.dll")] public static extern bool EnumWindows(P cb, IntPtr l);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out R r);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint f);
  [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern uint MapVirtualKey(uint c, uint t);
  [DllImport("user32.dll")] public static extern bool MoveWindow(IntPtr h, int x, int y, int w, int ht, bool rp);
  [DllImport("user32.dll")] public static extern int GetSystemMetrics(int i);
  [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte sc, uint fl, UIntPtr ex);
  public struct R { public int L, T, Rr, B; }
  public static IntPtr Big(uint pid) { IntPtr best = IntPtr.Zero; int bw = -1;
    EnumWindows((h,l) => { uint p; GetWindowThreadProcessId(h, out p);
      if (p==pid) { R r; GetWindowRect(h, out r); int w=r.Rr-r.L; if (w>bw) { bw=w; best=h; } } return true; }, IntPtr.Zero);
    return best; }
  public static void Key(int vk, int ms) {
    byte sc = (byte)MapVirtualKey((uint)vk, 0);
    keybd_event((byte)vk, sc, 0, UIntPtr.Zero);
    System.Threading.Thread.Sleep(ms);
    keybd_event((byte)vk, sc, 2, UIntPtr.Zero);
  }
}
"@
$repo = "C:\Users\Usuario\Desktop\Proyectos_Claude\noctis-psp"
$p = Get-Process PPSSPPWindows64 -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $p) { Write-Output "NO_PPSSPP (lanza shot.ps1 primero)"; exit }
$h = [WP]::Big([uint32]$p.Id)
$vk = @{ I=0x49; K=0x4B; J=0x4A; L=0x4C; T=0x54; X=0x58; S=0x53; C=0x43; Q=0x51; W=0x57; SPACE=0x20; UP=0x26; DOWN=0x28; LEFT=0x25; RIGHT=0x27 }
$prev = [WP]::GetForegroundWindow()
if ($Keys) {
  $sw = [WP]::GetSystemMetrics(0); $sh = [WP]::GetSystemMetrics(1)
  [WP]::MoveWindow($h, $sw - 2, $sh - 2, 980, 600, $true) | Out-Null   # 1px asoma abajo-derecha: Windows acepta el foco, nadie lo ve
  Start-Sleep -Milliseconds 200
  [WP]::SetForegroundWindow($h) | Out-Null; Start-Sleep -Milliseconds 900
  foreach ($tok in ($Keys -split ',')) {
    if (-not $tok) { continue }
    $parts = $tok.Trim() -split ':'; $k = $parts[0].ToUpper(); $ms = 120; if ($parts.Count -gt 1) { $ms = [int]$parts[1] }
    if ($vk.ContainsKey($k)) { [WP]::Key($vk[$k], $ms); Start-Sleep -Milliseconds 100 }
  }
  Start-Sleep -Milliseconds 300
  if ($prev -ne [IntPtr]::Zero) { [WP]::SetForegroundWindow($prev) | Out-Null }
  [WP]::MoveWindow($h, -2600, 80, 980, 600, $true) | Out-Null   # de vuelta fuera de pantalla
  Start-Sleep -Milliseconds 300
}
Start-Sleep -Milliseconds 300
$r = New-Object WP+R; [WP]::GetWindowRect($h, [ref]$r) | Out-Null
$w = $r.Rr - $r.L; $ht = $r.B - $r.T
$bmp = New-Object System.Drawing.Bitmap($w, $ht); $g = [System.Drawing.Graphics]::FromImage($bmp)
$hdc = $g.GetHdc(); [WP]::PrintWindow($h, $hdc, 2) | Out-Null; $g.ReleaseHdc($hdc)
$bmp.Save("$repo\build\$Out.png", [System.Drawing.Imaging.ImageFormat]::Png); $g.Dispose(); $bmp.Dispose()
Write-Output ("PLAY ok -> build\{0}.png ({1}x{2})" -f $Out, $w, $ht)
