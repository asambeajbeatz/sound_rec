# Search CMake
# Check if CMake is already available in the system PATH
if (Get-Command "cmake" -ErrorAction SilentlyContinue) {
    $cmakePath = "cmake"
	Write-Host "`nFound CMake in system PATH" -ForegroundColor Green
} 
else {
    # Define possible Visual Studio versions and editions to search for
    $vsVersions = @("2025", "2022", "2019", "2017")
    $vsEditions = @("Community", "Professional", "Enterprise")
    $cmakePath = $null

    # Iterate through paths to find the built-in CMake provided by Visual Studio
    foreach ($ver in $vsVersions) {
        foreach ($edit in $vsEditions) {
			
			# Base paths for 32-bit and 64-bit VS installations
            $basePaths = @("${env:ProgramFiles(x86)}", "${env:ProgramFiles}")
			
			foreach ($base in $basePaths){
				if ($base) { # Only run if $base is not empty
				$testPath = Join-Path $base "Microsoft Visual Studio\$ver\$edit\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" 
				if (Test-Path $testPath) { $cmakePath = $testPath; break } # Exit $basePaths loop
				}
			}
			
			if ($cmakePath) { break } # Exit $vsEditions loop
			
		}
		
		if ($cmakePath) { break } # Exit $vsVersions loop
		
	}
}

# Error handling if CMake is not found
if (-not $cmakePath) {
    Write-Error "CMake not found! Please install CMake or add it to your PATH."
    exit
}

$buildDir = "build"

# Remove build directory if it exists
if (Test-Path $buildDir) {
    Write-Host "`nFolder 'build' exists." -ForegroundColor Cyan

    $answer = Read-Host "`n Delete it? (y/n)"

    if ($answer -match '^[Yy]$') {
        Write-Host "`n Removing folder..." -ForegroundColor Gray 
        Remove-Item $buildDir -Recurse -Force 
	}
    else {
        Write-Host "`nAborted by user. Exiting.`n" -ForegroundColor Yellow
		Start-Sleep -Seconds 2
        exit
    }
}

# Create new build directory
New-Item -ItemType Directory -Path $buildDir | Out-Null

# Enter build directory
Set-Location $buildDir

Write-Host "`n`n=== Detecting Visual Studio and Generating Project ===`n" -ForegroundColor Green

# Start CMake
# -S .. (исходники на уровень выше)
# -B . (проект создавать в текущей папке build)
& $cmakePath -S .. -B . -A Win32 -T v141_xp

if ($LASTEXITCODE -ne 0) {
    Write-Host "`n [ERROR] Project generation failed." -ForegroundColor Red
    Write-Host "`n Make sure you have 'C++ Windows XP support for VS 2017 (v141_xp)' installed." -ForegroundColor Yellow
} else {
    Write-Host "`n [SUCCESS] Project generated in '$buildDir' folder." -ForegroundColor Green
}

Write-Host "`n`nPress Enter to exit..."
[void][System.Console]::ReadLine()
