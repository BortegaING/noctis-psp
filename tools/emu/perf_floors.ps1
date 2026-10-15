# NOCTIS: recorre los pisos 0..7 en el emulador oculto y saca, con SELECT apretado, el
# medidor de carga (vertices V y llamadas D del frame) + la RAM libre. Deja recortes en build\perf_N.png
param([int[]]$Floors = @(0,1,2,3,4,5,6,7))
$e = "C:\Users\Usuario\Desktop\Proyectos_Claude\noctis-psp\tools\emu"
$b = "C:\Users\Usuario\Desktop\Proyectos_Claude\noctis-psp\build"
Set-Location $e
foreach ($n in $Floors) {
  powershell -ExecutionPolicy Bypass -File (Join-Path $e "hidden.ps1") | Out-Null
  if ($n -gt 0) { python goto_floor.py $n | Out-Null }
  # el piso 1 se mide con los ojos ya en el cielo; en los jefes, con la pelea andando un poco
  $extra = @("wait:2000")
  if ($n -eq 1) { $extra = @("sy:1", "wait:2600", "sy:0", "wait:6500") }
  if ($n -ge 3) { $extra = @("sy:1", "wait:1500", "sy:0", "wait:4000") }
  python psp_eye.py @extra hold:select "wait:400" ("shot:perf_{0}" -f $n) free:select | Out-Null
  Write-Output ("piso {0} listo" -f $n)
}
