Add-Type -AssemblyName System.Runtime.WindowsRuntime
$asTaskGeneric = ([System.WindowsRuntimeSystemExtensions].GetMethods() | Where-Object { $_.Name -eq 'AsTask' -and $_.GetParameters().Count -eq 1 -and $_.GetParameters()[0].ParameterType.Name -eq 'IAsyncOperation`1' })[0]

function AwaitTask($WinRtTask, $ResultType) {
    $asTask = $asTaskGeneric.MakeGenericMethod($ResultType)
    $netTask = $asTask.Invoke($null, @($WinRtTask))
    $netTask.Wait(-1) | Out-Null
    return $netTask.Result
}

[Windows.Media.Control.GlobalSystemMediaTransportControlsSessionManager, Windows.Media, ContentType = WindowsRuntime] | Out-Null
$manager = AwaitTask ([Windows.Media.Control.GlobalSystemMediaTransportControlsSessionManager]::RequestAsync()) ([Windows.Media.Control.GlobalSystemMediaTransportControlsSessionManager])

$sessions = $manager.GetSessions()
foreach ($session in $sessions) {
    $timeline = $session.GetTimelineProperties()
    $media = AwaitTask ($session.TryGetMediaPropertiesAsync()) ([Windows.Media.Control.GlobalSystemMediaTransportControlsSessionMediaProperties])
    $playback = $session.GetPlaybackInfo()
    [PSCustomObject]@{
        App        = $session.SourceAppUserModelId
        Title      = $media.Title
        Artist     = $media.Artist
        Status     = $playback.PlaybackStatus
        Position   = $timeline.Position.ToString()
        TotalTime  = $timeline.EndTime.ToString()
    } | Format-List
}
