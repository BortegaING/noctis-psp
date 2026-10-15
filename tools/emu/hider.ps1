# NOCTIS: vigia que mantiene OCULTA toda ventana del PPSSPP de pruebas mientras ese proceso viva.
# (hidden.ps1 lo lanza oculto con el PID; antes el vigia duraba 10 s y PPSSPP a veces se mostraba
#  despues, en plena prueba, en la pantalla de Benjamin.)
param([int]$ProcId)
Add-Type @"
using System; using System.Runtime.InteropServices;
public class WHide {
  public delegate bool P(IntPtr h, IntPtr l);
  [DllImport("user32.dll")] public static extern bool EnumWindows(P cb, IntPtr l);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int n);
  public static void HideAll(uint pid) {
    EnumWindows((h,l) => { uint p; GetWindowThreadProcessId(h, out p);
      if (p==pid && IsWindowVisible(h)) ShowWindow(h, 0); return true; }, IntPtr.Zero);
  }
}
"@
while (Get-Process -Id $ProcId -ErrorAction SilentlyContinue) {
  [WHide]::HideAll([uint32]$ProcId)
  Start-Sleep -Milliseconds 30
}
