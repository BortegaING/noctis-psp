# NOCTIS -- sintetiza cada linea de un archivo de texto (UTF-8) con una voz de Windows (WinRT/OneCore)
# y la guarda como raw_00.wav, raw_01.wav ... en la carpeta de salida. Lo llama tools/voice/make_voice.py.
#   powershell -ExecutionPolicy Bypass -File tools\voice\tts_winrt.ps1 <lineas.txt> <carpeta> [voz] [velocidad] [tono]
# Cada linea va dentro de un <prosody>: puede llevar SSML (<break time='300ms'/>).
# La voz por defecto es "Microsoft Raul" (es-MX, hombre): con la lectura castellana el latin suena
# eclesiastico. Las voces de SAPI clasico (System.Speech) en este PC son solo de mujer.
$ErrorActionPreference = 'Stop'
$file = $args[0]; $outDir = $args[1]
$voiceName = 'Microsoft Raul'; if ($args.Count -gt 2) { $voiceName = $args[2] }
$rate = '-10%'; if ($args.Count -gt 3) { $rate = $args[3] }
$pitch = '-10%'; if ($args.Count -gt 4) { $pitch = $args[4] }

Add-Type -AssemblyName System.Runtime.WindowsRuntime
$null = [Windows.Media.SpeechSynthesis.SpeechSynthesizer, Windows.Media.SpeechSynthesis, ContentType = WindowsRuntime]
$null = [Windows.Storage.Streams.DataReader, Windows.Storage.Streams, ContentType = WindowsRuntime]
$asTask = ([System.WindowsRuntimeSystemExtensions].GetMethods() | Where-Object {
        $_.Name -eq 'AsTask' -and $_.GetParameters().Count -eq 1 -and $_.GetParameters()[0].ParameterType.Name -eq 'IAsyncOperation`1' })[0]
function Await($op, [Type]$t) {
    $task = $asTask.MakeGenericMethod($t).Invoke($null, @($op))
    $task.Wait(-1) | Out-Null
    $task.Result
}

$synth = New-Object Windows.Media.SpeechSynthesis.SpeechSynthesizer
$v = [Windows.Media.SpeechSynthesis.SpeechSynthesizer]::AllVoices | Where-Object { $_.DisplayName -eq $voiceName } | Select-Object -First 1
if (-not $v) { throw "no esta la voz '$voiceName'" }
$synth.Voice = $v

$k = 0
foreach ($ln in (Get-Content -Encoding UTF8 $file)) {
    if (-not $ln.Trim()) { continue }
    $ssml = "<speak version='1.0' xmlns='http://www.w3.org/2001/10/synthesis' xml:lang='es-MX'>" +
            "<prosody rate='$rate' pitch='$pitch'>$ln</prosody></speak>"
    $st = Await ($synth.SynthesizeSsmlToStreamAsync($ssml)) ([Windows.Media.SpeechSynthesis.SpeechSynthesisStream])
    $rd = New-Object Windows.Storage.Streams.DataReader($st.GetInputStreamAt(0))
    $n = Await ($rd.LoadAsync([uint32]$st.Size)) ([uint32])
    $buf = New-Object byte[] $n
    $rd.ReadBytes($buf)
    [IO.File]::WriteAllBytes((Join-Path $outDir ('raw_{0:D2}.wav' -f $k)), $buf)
    $k++
}
Write-Output "TTS ok: $k frases ($voiceName)"
