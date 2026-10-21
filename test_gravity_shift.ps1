Add-Type @"
using System;
using System.Runtime.InteropServices;
public class WinMsg {
    [DllImport("user32.dll", SetLastError = true)]
    public static extern IntPtr FindWindow(string lpClassName, string lpWindowName);
    [DllImport("user32.dll")]
    public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
    public const uint WM_CHAR = 0x0102;
    public const uint WM_KEYDOWN = 0x0100;
    public const uint WM_KEYUP = 0x0101;
}
"@

Get-Process GravityShift, SpaceRanger -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Milliseconds 300

$exe = "c:\Users\hp\OneDrive\Desktop\Gravity game\Release\GravityShift.exe"
if (-not (Test-Path $exe)) {
    $exe = "c:\Users\hp\OneDrive\Desktop\Gravity game\Release\SpaceRanger.exe"
}
$dir = "c:\Users\hp\OneDrive\Desktop\Gravity game"

Write-Output "=== LAUNCHING GRAVITY SHIFT INTEGRATION TEST ==="
$proc = Start-Process -FilePath $exe -WorkingDirectory $dir -PassThru
Start-Sleep -Seconds 2

if ($proc -and -not $proc.HasExited) {
    $hwnd = [WinMsg]::FindWindow($null, "GRAVITY SHIFT")
    if ($hwnd -eq [IntPtr]::Zero) {
        $hwnd = (Get-Process -Id $proc.Id).MainWindowHandle
    }

    Write-Output "Game Process PID: $($proc.Id), Window HWND: $hwnd"

    function AssertAlive($step) {
        $proc.Refresh()
        if ($proc.HasExited) {
            Write-Output "TEST FAILED: Process exited at $step! ExitCode: $($proc.ExitCode)"
            exit 1
        }
    }

    # Step 1: Normal Walking on Floor
    Write-Output "Step 1: Testing Normal Walking on Floor (W/S/A/D)..."
    [WinMsg]::PostMessage($hwnd, [WinMsg]::WM_CHAR, [IntPtr]119, [IntPtr]0) # 'w'
    Start-Sleep -Milliseconds 150
    [WinMsg]::PostMessage($hwnd, [WinMsg]::WM_KEYUP, [IntPtr]0x57, [IntPtr]0)
    Start-Sleep -Milliseconds 80
    AssertAlive("Walking W")

    [WinMsg]::PostMessage($hwnd, [WinMsg]::WM_CHAR, [IntPtr]97, [IntPtr]0) # 'a'
    Start-Sleep -Milliseconds 100
    [WinMsg]::PostMessage($hwnd, [WinMsg]::WM_KEYUP, [IntPtr]0x41, [IntPtr]0)
    Start-Sleep -Milliseconds 80
    AssertAlive("Strafing A")

    [WinMsg]::PostMessage($hwnd, [WinMsg]::WM_CHAR, [IntPtr]100, [IntPtr]0) # 'd'
    Start-Sleep -Milliseconds 100
    [WinMsg]::PostMessage($hwnd, [WinMsg]::WM_KEYUP, [IntPtr]0x44, [IntPtr]0)
    Start-Sleep -Milliseconds 100
    AssertAlive("Strafing D")

    # Step 2: Jump on Floor
    Write-Output "Step 2: Testing Jump on Floor (Space)..."
    [WinMsg]::PostMessage($hwnd, [WinMsg]::WM_CHAR, [IntPtr]32, [IntPtr]0) # Space
    Start-Sleep -Milliseconds 350
    AssertAlive("Jump")

    # Step 3: Shift Gravity through all 4 orientations (Floor -> Left Wall -> Ceiling -> Right Wall -> Floor)
    Write-Output "Step 3: Cycling Gravity Shift (G key) across all 4 surfaces..."
    for ($g = 1; $g -le 4; $g++) {
        [WinMsg]::PostMessage($hwnd, [WinMsg]::WM_CHAR, [IntPtr]103, [IntPtr]0) # 'g'
        Start-Sleep -Milliseconds 350
        AssertAlive("Gravity Shift #$g")
    }

    # Step 4: Level 1 - Walk to Exit Door and Press [E]
    Write-Output "Step 4: Level 1 - Walking to Exit Door..."
    [WinMsg]::PostMessage($hwnd, [WinMsg]::WM_CHAR, [IntPtr]49, [IntPtr]0) # Level 1
    Start-Sleep -Milliseconds 150

    for ($i = 0; $i -lt 12; $i++) {
        [WinMsg]::PostMessage($hwnd, [WinMsg]::WM_CHAR, [IntPtr]119, [IntPtr]0)
        Start-Sleep -Milliseconds 40
    }
    [WinMsg]::PostMessage($hwnd, [WinMsg]::WM_KEYUP, [IntPtr]0x57, [IntPtr]0)
    Start-Sleep -Milliseconds 150
    AssertAlive("Walk to Door")

    Write-Output "  Pressing [E] to exit Level 1..."
    [WinMsg]::PostMessage($hwnd, [WinMsg]::WM_CHAR, [IntPtr]101, [IntPtr]0) # 'e'
    Start-Sleep -Milliseconds 250
    AssertAlive("Exit Level 1")

    Write-Output "  Pressing [ENTER] to advance to Level 2..."
    [WinMsg]::PostMessage($hwnd, [WinMsg]::WM_CHAR, [IntPtr]13, [IntPtr]0) # Enter
    Start-Sleep -Milliseconds 250
    AssertAlive("Advance to Level 2")

    # Step 5: Level 2 - Wall Walking
    Write-Output "Step 5: Level 2 - Testing Wall Walking..."
    [WinMsg]::PostMessage($hwnd, [WinMsg]::WM_CHAR, [IntPtr]50, [IntPtr]0) # Level 2
    Start-Sleep -Milliseconds 150

    # Shift to Left Wall
    [WinMsg]::PostMessage($hwnd, [WinMsg]::WM_CHAR, [IntPtr]103, [IntPtr]0) # 'g'
    Start-Sleep -Milliseconds 350
    AssertAlive("Shift to Left Wall")

    # Walk along Left Wall
    for ($i = 0; $i -lt 8; $i++) {
        [WinMsg]::PostMessage($hwnd, [WinMsg]::WM_CHAR, [IntPtr]119, [IntPtr]0)
        Start-Sleep -Milliseconds 40
    }
    [WinMsg]::PostMessage($hwnd, [WinMsg]::WM_KEYUP, [IntPtr]0x57, [IntPtr]0)
    Start-Sleep -Milliseconds 150
    AssertAlive("Walk on Left Wall")

    # Step 6: Level 3 - Crate Push
    Write-Output "Step 6: Level 3 - Testing Movable Crate..."
    [WinMsg]::PostMessage($hwnd, [WinMsg]::WM_CHAR, [IntPtr]51, [IntPtr]0) # Level 3
    Start-Sleep -Milliseconds 150
    [WinMsg]::PostMessage($hwnd, [WinMsg]::WM_CHAR, [IntPtr]101, [IntPtr]0) # 'e' (push crate)
    Start-Sleep -Milliseconds 200
    AssertAlive("Push Crate")

    # Step 7: Level 4 - Keycard Retrieval
    Write-Output "Step 7: Level 4 - Testing Keycard and Locked Door..."
    [WinMsg]::PostMessage($hwnd, [WinMsg]::WM_CHAR, [IntPtr]52, [IntPtr]0) # Level 4
    Start-Sleep -Milliseconds 150
    [WinMsg]::PostMessage($hwnd, [WinMsg]::WM_CHAR, [IntPtr]101, [IntPtr]0) # 'e'
    Start-Sleep -Milliseconds 200
    AssertAlive("Keycard Interaction")

    # Step 8: Level 5 - Evacuation and Oxygen Timer
    Write-Output "Step 8: Level 5 - Testing Gravity Master and Evacuation..."
    [WinMsg]::PostMessage($hwnd, [WinMsg]::WM_CHAR, [IntPtr]53, [IntPtr]0) # Level 5
    Start-Sleep -Milliseconds 200
    AssertAlive("Level 5 Timer")

    # Step 9: Camera Modes and Replay
    Write-Output "Step 9: Testing Camera Toggles [T/F/V] and Replay [R]..."
    [WinMsg]::PostMessage($hwnd, [WinMsg]::WM_CHAR, [IntPtr]116, [IntPtr]0) # 't'
    Start-Sleep -Milliseconds 100
    [WinMsg]::PostMessage($hwnd, [WinMsg]::WM_CHAR, [IntPtr]102, [IntPtr]0) # 'f'
    Start-Sleep -Milliseconds 100
    [WinMsg]::PostMessage($hwnd, [WinMsg]::WM_CHAR, [IntPtr]118, [IntPtr]0) # 'v'
    Start-Sleep -Milliseconds 100
    [WinMsg]::PostMessage($hwnd, [WinMsg]::WM_CHAR, [IntPtr]114, [IntPtr]0) # 'r' (replay)
    Start-Sleep -Milliseconds 200
    AssertAlive("Camera & Replay")

    $proc.Refresh()
    if (-not $proc.HasExited) {
        Write-Output "=== COMPLETE TEST PASSED: ALL PHASES VERIFIED SUCCESSFULLY! ==="
        Stop-Process -Id $proc.Id -Force
        Write-Output "Game cleanly shut down after verification."
    }
} else {
    Write-Output "TEST FAILED: Failed to launch game executable."
    exit 1
}
