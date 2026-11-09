<#
.SYNOPSIS
Builds the 13 targets of the matching build on Windows, inside WSL.

.DESCRIPTION
The matching build's compiler (a Windows program, run on Linux by wibo) needs
Linux: on Windows the build runs inside WSL 2. This script checks that WSL and a
distribution are there (it installs nothing), finds the repository in WSL's file
system or puts it there (the build is much faster there than on a Windows drive), and
runs tools/build.sh in it, which sets up the tools when anything is missing,
configures and builds. It ends with one line per target: OK for all 13.

Your copy of the game stays where it is on Windows (-OrigDir).

.PARAMETER OrigDir
The Windows folder with your copy of the game's disc (its files, or its .iso).
Default: $env:MH2_ORIG_DIR, else the repository's parent folder.

.PARAMETER Repo
The repository's folder in WSL, as a Linux path. Default: this repository, when
this script is in WSL's file system, else ~/mh2-decomp (made from this one).

.PARAMETER Distro
The WSL distribution. Default: the default distribution.

.PARAMETER Jobs
Parallel jobs for ninja (default: ninja's).

.PARAMETER Targets
Ninja targets in place of the default, for example progress or provenance.

.EXAMPLE
powershell -ExecutionPolicy Bypass -File tools\build.ps1 -OrigDir D:\games\mh2
#>
[CmdletBinding()]
param(
    [string]$OrigDir,
    [string]$Repo,
    [string]$Distro,
    [int]$Jobs = 0,
    [string[]]$Targets = @()
)
$ErrorActionPreference = 'Stop'

function Fail([string]$Message) {
    Write-Host "build.ps1: $Message" -ForegroundColor Red
    exit 1
}

# --- WSL and a distribution (read only: nothing is installed) --------------------
if (-not (Get-Command wsl.exe -ErrorAction SilentlyContinue)) {
    Fail ("WSL is not installed (wsl.exe was not found). Install it once, from an administrator PowerShell:`n" +
          "    wsl --install -d Ubuntu-24.04`n" +
          "then restart Windows and run this script again (https://learn.microsoft.com/windows/wsl/install).")
}

$oldEncoding = [Console]::OutputEncoding
[Console]::OutputEncoding = [System.Text.Encoding]::Unicode          # wsl -l writes UTF-16
try { $listed = @(& wsl.exe -l -q 2>$null) } finally { [Console]::OutputEncoding = $oldEncoding }
$names = @($listed | ForEach-Object { ($_ -replace "`0", '').Trim() } |
           Where-Object { $_ -and $_ -notmatch '^docker-desktop' -and $_ -notmatch '\s' })

# The repository may be in WSL already (this script run from a \\wsl.localhost\ path)
$repoWin = Split-Path -Parent $PSScriptRoot
$srcInWsl = $repoWin -match '^\\\\(?:wsl\.localhost|wsl\$)\\([^\\]+)(\\.*)?$'
if ($srcInWsl) {
    $srcDistro = $Matches[1]
    $srcLinux = ($Matches[2] -replace '\\', '/')
    if (-not $Distro) { $Distro = $srcDistro }
}
if (-not $Distro) {
    if ($names.Count -eq 0) {
        Fail ("WSL has no Linux distribution installed. Install one once, from an administrator PowerShell:`n" +
              "    wsl --install -d Ubuntu-24.04`n" +
              "then open it once to make your user, and run this script again.")
    }
    $Distro = $names[0]
} elseif ($names -notcontains $Distro) {
    Fail ("no WSL distribution named '$Distro'. Installed: " + ($names -join ', ') +
          ". To install one: wsl --install -d Ubuntu-24.04")
}

function Wsl {
    & wsl.exe -d $Distro -e @args
    if ($LASTEXITCODE -ne 0) { throw "wsl exit code $LASTEXITCODE" }
}
function WslOut {
    $o = & wsl.exe -d $Distro -e @args 2>$null
    if ($LASTEXITCODE -ne 0) { return $null }
    return (($o | Out-String).Trim())
}

$arch = WslOut uname -m
if ($arch -ne 'x86_64') {
    Fail "the distribution '$Distro' does not start, or is not x86-64 (uname -m: '$arch'). Check it with: wsl -d $Distro"
}
Write-Host "[..]   WSL distribution $Distro"

# --- the repository in WSL's file system -----------------------------------------
$home_ = WslOut printenv HOME
if (-not $home_) { Fail "cannot read the home folder in '$Distro'" }
if (-not $srcInWsl) { $srcLinux = WslOut wslpath -a -u $repoWin }
if (-not $Repo) { $Repo = if ($srcInWsl) { $srcLinux } else { "$home_/mh2-decomp" } }

if ($null -eq (WslOut test -f "$Repo/tools/build.sh" -a -f "$Repo/tools/setup.sh")) {
    foreach ($tool in 'git', 'tar', 'gzip') {
        if ($null -eq (WslOut which $tool)) {
            Fail ("'$tool' is not installed in '$Distro'. In an Ubuntu shell:`n" +
                  "    sudo apt-get install ca-certificates curl git tar gzip")
        }
    }
    Write-Host "[..]   the repository into WSL: $Repo (from $srcLinux)"
    if ($null -ne (WslOut test -e $Repo)) { Fail "$Repo exists in '$Distro' and is not a repository with tools/build.sh" }
    $safe = 'safe.directory=*'
    if ($null -ne (WslOut test -d "$srcLinux/.git")) {
        Wsl git -c $safe clone --quiet $srcLinux $Repo
        $url = WslOut git -c $safe -C $srcLinux remote get-url origin
        if ($url) { Wsl git -C $Repo remote set-url origin $url }
        Write-Host "       (what is committed in the checkout; uncommitted changes stay in it)"
    } else {
        Wsl mkdir -p $Repo
        Wsl cp -a "$srcLinux/." "$Repo/"
    }
} else {
    Write-Host "[ok]   the repository in WSL: $Repo"
}
if ($Repo -like '/mnt/*') {
    Write-Host "       warning: $Repo is on a Windows drive; the build is much faster in WSL's own file system (default: ~/mh2-decomp)" -ForegroundColor Yellow
}

# --- your copy of the game, left on Windows -------------------------------------
if (-not $OrigDir) { $OrigDir = $env:MH2_ORIG_DIR }
if (-not $OrigDir -and -not $srcInWsl) { $OrigDir = Split-Path -Parent $repoWin }
$origLinux = $null
if ($OrigDir) {
    if (-not (Test-Path -LiteralPath $OrigDir -PathType Container)) {
        Fail "the folder with your copy of the game does not exist: ${OrigDir} (give it with -OrigDir)"
    }
    $found = Get-ChildItem -LiteralPath $OrigDir -Recurse -Depth 1 -File -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -ieq 'SYSTEM.CNF' -or $_.Extension -ieq '.iso' } | Select-Object -First 1
    if (-not $found) {
        Fail "no SYSTEM.CNF and no .iso in ${OrigDir}: give the folder with your copy of the game with -OrigDir"
    }
    $origLinux = WslOut wslpath -a -u $OrigDir
    Write-Host "[ok]   your copy of the game: $OrigDir"
}

# --- the build ---------------------------------------------------------------------
$cmd = @('-d', $Distro, '-e', 'env')
if ($origLinux) { $cmd += "MH2_ORIG_DIR=$origLinux" }
$cmd += @('bash', "$Repo/tools/build.sh")
if ($Jobs -gt 0) { $cmd += @('-j', "$Jobs") }
$cmd += $Targets
Write-Host "[..]   tools/build.sh in WSL (the first run downloads the tools, about 400 MB)"
& wsl.exe @cmd
$code = $LASTEXITCODE
if ($code -ne 0) {
    Write-Host "build.ps1: the build failed (exit code $code); the messages above say where." -ForegroundColor Red
    exit $code
}
Write-Host "[done] the build. The repository is in WSL: $Repo (Windows path: \\wsl.localhost\$Distro$($Repo -replace '/', '\'))"
