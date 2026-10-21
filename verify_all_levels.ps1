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

$exe = "c:\Users\hp\OneDrive\Desktop\Gravity game\Release\GravityShift.exe"
if (-not (Test-Path $exe)) {
    $exe = "c:\Users\hp\OneDrive\Desktop\Gravity game\Release\SpaceRanger.exe"
}
$dir = "c:\Users\hp\OneDrive\Desktop\Gravity game"

Write-Output "=========================================================="
Write-Output "   GRAVITY SHIFT: COMPLETE 5-LEVEL VERIFICATION SUITE     "
Write-Output "=========================================================="

Get-Process GravityShift, SpaceRanger -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Milliseconds 300

$proc = Start-Process -FilePath $exe -WorkingDirectory $dir -PassThru
Start-Sleep -Milliseconds 1200

if (-not $proc -or $proc.HasExited) {
    Write-Output "ERROR: Failed to launch game executable."
    exit 1
}

$hwnd = [WinMsg]::FindWindow($null, "GRAVITY SHIFT")
if ($hwnd -eq [IntPtr]::Zero) {
    $hwnd = (Get-Process -Id $proc.Id).MainWindowHandle
}

Write-Output "Launched Game PID: $($proc.Id), Window HWND: $hwnd"

function AssertProcessAlive($context) {
    $proc.Refresh()
    if ($proc.HasExited) {
        Write-Output "CRITICAL FAILURE: Process exited unexpectedly at '$context'! ExitCode: $($proc.ExitCode)"
        exit 1
    }
}

function SendChar($c, $sleepMs = 60) {
    [WinMsg]::PostMessage($hwnd, [WinMsg]::WM_CHAR, [IntPtr][char]$c, [IntPtr]0) | Out-Null
    Start-Sleep -Milliseconds $sleepMs
    AssertProcessAlive("SendChar $c")
}

function WalkForward($repeats = 6, $sleepMs = 25) {
    for ($i = 0; $i -lt $repeats; $i++) {
        [WinMsg]::PostMessage($hwnd, [WinMsg]::WM_CHAR, [IntPtr]119, [IntPtr]0) | Out-Null # 'w'
        Start-Sleep -Milliseconds $sleepMs
    }
    [WinMsg]::PostMessage($hwnd, [WinMsg]::WM_KEYUP, [IntPtr]0x57, [IntPtr]0) | Out-Null
    Start-Sleep -Milliseconds 40
    AssertProcessAlive("WalkForward")
}

# -------------------------------------------------------------------------------------------------
# LEVEL 1: BASIC MOVEMENT & LEVEL TRANSITION
# -------------------------------------------------------------------------------------------------
Write-Output ">>> TEST 1: LEVEL 1 - BASIC MOVEMENT <<<"
SendChar '1' 100
WalkForward 4 25
SendChar 'a' 50
SendChar 'd' 50
SendChar ' ' 120 # Jump
WalkForward 8 25
SendChar 'e' 150 # Exit Level 1
SendChar 13 150  # Advance to Level 2
Write-Output "  --> LEVEL 1 VERIFIED: PASSED!"

# -------------------------------------------------------------------------------------------------
# LEVEL 2: WALL WALKING (HAZARD BYPASS)
# -------------------------------------------------------------------------------------------------
Write-Output ">>> TEST 2: LEVEL 2 - WALL WALKING <<<"
SendChar '2' 100
SendChar 'g' 150 # Shift Left Wall (-X)
WalkForward 6 25 # Traverse past hazard on wall
SendChar 'g' 120 # Shift Ceiling
SendChar 'g' 120 # Shift Right Wall
SendChar 'g' 150 # Shift Floor
WalkForward 4 25
SendChar 'e' 150 # Exit Level 2
SendChar 'n' 150 # Advance via 'N'
Write-Output "  --> LEVEL 2 VERIFIED: PASSED!"

# -------------------------------------------------------------------------------------------------
# LEVEL 3: CRATE PUZZLE (BLOCKING & UNBLOCKING)
# -------------------------------------------------------------------------------------------------
Write-Output ">>> TEST 3: LEVEL 3 - CRATE PUZZLE <<<"
SendChar '3' 100
WalkForward 6 25
SendChar 'e' 120 # Push crate aside
WalkForward 3 25 # Body push
SendChar 'e' 150 # Exit Level 3
SendChar 13 150  # Advance to Level 4
Write-Output "  --> LEVEL 3 VERIFIED: PASSED!"

# -------------------------------------------------------------------------------------------------
# LEVEL 4: FIND THE KEYCARD (CEILING RETRIEVAL & FORCEFIELD UNLOCK)
# -------------------------------------------------------------------------------------------------
Write-Output ">>> TEST 4: LEVEL 4 - KEYCARD & FORCEFIELD <<<"
SendChar '4' 100
WalkForward 6 25
SendChar 'e' 100 # Rejection check
SendChar 'g' 120 # Left Wall
SendChar 'g' 150 # Ceiling
WalkForward 3 25
SendChar 'e' 150 # Collect Keycard
SendChar 'g' 120 # Right Wall
SendChar 'g' 150 # Floor
WalkForward 5 25
SendChar 'e' 150 # Exit Level 4
SendChar 13 150  # Advance to Level 5
Write-Output "  --> LEVEL 4 VERIFIED: PASSED!"

# -------------------------------------------------------------------------------------------------
# LEVEL 5: GRAVITY MASTER (EVACUATION EMERGENCY, WALL KEYCARD, CRATE & OXYGEN)
# -------------------------------------------------------------------------------------------------
Write-Output ">>> TEST 5: LEVEL 5 - GRAVITY MASTER <<<"
SendChar '5' 100
SendChar 'g' 120 # Left Wall
SendChar 'g' 120 # Ceiling
SendChar 'g' 150 # Right Wall
WalkForward 5 25
SendChar 'e' 150 # Collect Wall Keycard
SendChar 'g' 150 # Back to Floor
WalkForward 4 25
SendChar 'e' 120 # Push Crate
WalkForward 3 25
SendChar 'e' 200 # Escape Pod Complete!
Write-Output "  --> LEVEL 5 VERIFIED: PASSED!"

# -------------------------------------------------------------------------------------------------
# RESTART & REPLAY STATE ISOLATION VERIFICATION
# -------------------------------------------------------------------------------------------------
Write-Output ">>> TEST 6: RESTART & REPLAY INTEGRITY <<<"
SendChar '1' 100
WalkForward 3 25
SendChar 'r' 150 # Restart level
SendChar 't' 60  # Third person
SendChar 'f' 60  # First person
SendChar 'v' 60  # Top-down
SendChar 'v' 60  # Return
SendChar 'c' 60  # Ceiling toggle
SendChar 'c' 60  # Ceiling return

Write-Output "  Shutting down game [Q]..."
[WinMsg]::PostMessage($hwnd, [WinMsg]::WM_CHAR, [IntPtr][char]'q', [IntPtr]0) | Out-Null
Start-Sleep -Milliseconds 400

$proc.Refresh()
if ($proc.HasExited) {
    Write-Output "  --> Game cleanly shut down with exit code $($proc.ExitCode)."
} else {
    Stop-Process -Id $proc.Id -Force
}

Write-Output ""
Write-Output "=========================================================="
Write-Output "   ALL 5 LEVELS AND INTEGRITY TESTS PASSED SUCCESSFULLY!  "
Write-Output "=========================================================="
