# NOCTIS: lanza PPSSPP OCULTO (nunca aparece en la pantalla del usuario) para psp_eye.py.
# shot.ps1 lo dejaba ~7 s visible: PPSSPP ignora el WindowX fuera de pantalla al arrancar.
# Aqui arranca con SW_HIDE y ademas un vigia lo vuelve a ocultar si PPSSPP se muestra solo.
# Uso: powershell -ExecutionPolicy Bypass -File tools\emu\hidden.ps1 [-Build]
param([switch]$Build)
$ErrorActionPreference = 'SilentlyContinue'
Add-Type @"
using System; using System.Runtime.InteropServices;
public class WH {
  public delegate bool P(IntPtr h, IntPtr l);
  [DllImport("user32.dll")] public static extern bool EnumWindows(P cb, IntPtr l);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int n);
  public static int HideAll(uint pid) { int n = 0;
    EnumWindows((h,l) => { uint p; GetWindowThreadProcessId(h, out p);
      if (p==pid && IsWindowVisible(h)) { ShowWindow(h, 0); n++; } return true; }, IntPtr.Zero);
    return n; }
}
"@
$repo = "C:\Users\Usuario\Desktop\Proyectos_Claude\noctis-psp"
Get-Process PPSSPPWindows64 | Stop-Process -Force
Start-Sleep -Milliseconds 400
if ($Build) {
  $b = wsl -d Ubuntu-24.04 -u root -- bash -c 'cd /mnt/c/Users/Usuario/Desktop/Proyectos_Claude/noctis-psp && bash tools/build.sh'
  if (($b -join "`n") -notmatch 'Built target noctis') { Write-Output "BUILD FAIL:"; Write-Output (($b | Select-Object -Last 12) -join "`n"); exit }
  Write-Output "BUILD OK"
}
$proc = Start-Process -FilePath "$repo\ppsspp\PPSSPPWindows64.exe" -ArgumentList "`"$repo\build\EBOOT.PBP`"" -WindowStyle Hidden -PassThru
# vigia: 10 s ocultando cualquier ventana que PPSSPP muestre por su cuenta
$hid = 0; $t = [Diagnostics.Stopwatch]::StartNew()
while ($t.ElapsedMilliseconds -lt 10000) { $hid += [WH]::HideAll([uint32]$proc.Id); Start-Sleep -Milliseconds 15 }
$port = $null
foreach ($l in (netstat -ano | Select-String 'LISTENING')) { if ($l.Line.Trim().EndsWith(" $($proc.Id)")) { $port = ($l.Line -split '\s+')[2].Split(':')[-1]; break } }
if (-not (Get-Process -Id $proc.Id)) { Write-Output "PPSSPP se cerro (crash?)"; exit }
Write-Output ("HIDDEN ok pid={0} port={1} reocultadas={2}" -f $proc.Id, $port, $hid)
