# NOCTIS: pantallazo de cada jefe (pisos 4..7 por defecto): entra al piso, deja pasar la intro y
# mantiene L para que la camara se fije en el jefe. Deja build\jefe_N_a.png y jefe_N_b.png
param([int[]]$Floors = @(4,5,6,7))
$e = "C:\Users\Usuario\Desktop\Proyectos_Claude\noctis-psp\tools\emu"
Set-Location $e
foreach ($n in $Floors) {
  powershell -ExecutionPolicy Bypass -File (Join-Path $e "hidden.ps1") | Out-Null
  python goto_floor.py $n | Out-Null
  python psp_eye.py "wait:3500" hold:l "wait:1600" ("shot:jefe_{0}_a" -f $n) "wait:2600" ("shot:jefe_{0}_b" -f $n) free:l | Out-Null
  Write-Output ("piso {0} listo" -f $n)
}
