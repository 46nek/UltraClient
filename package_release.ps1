$ErrorActionPreference = "Stop"

$projectName = "KrunkerUltraClient"
$version = "v1.0.0"
$releaseDir = "build\Release"
$outputDirName = "${projectName}_${version}"
$outputZip = "${outputDirName}.zip"

Write-Host "Creating Release Package: $outputZip" -ForegroundColor Cyan

# 1. Rebuild the project to ensure everything is up-to-date
Write-Host "Building project..." -ForegroundColor Yellow
cmd.exe /c "build.bat"

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed! Aborting release packaging." -ForegroundColor Red
    exit 1
}

# 2. Create temporary release folder
if (Test-Path $outputDirName) {
    Remove-Item -Path $outputDirName -Recurse -Force
}
New-Item -Path $outputDirName -ItemType Directory | Out-Null

# 3. Copy necessary files
Write-Host "Copying files to $outputDirName..." -ForegroundColor Yellow
Copy-Item -Path "$releaseDir\KrunkerUltraClient.exe" -Destination $outputDirName

# Copy folders
Copy-Item -Path "$releaseDir\ui" -Destination $outputDirName -Recurse
Copy-Item -Path "$releaseDir\swapper" -Destination $outputDirName -Recurse
Copy-Item -Path "$releaseDir\scripts" -Destination $outputDirName -Recurse

# 4. Cleanup user-specific or unnecessary files inside the package
Write-Host "Cleaning up package contents..." -ForegroundColor Yellow
$swapperDir = "$outputDirName\swapper"
if (Test-Path $swapperDir) {
    # Remove everything in swapper folder, we just want it to exist empty
    Get-ChildItem -Path $swapperDir | Remove-Item -Recurse -Force
}

$scriptsDir = "$outputDirName\scripts"
if (Test-Path $scriptsDir) {
    # Remove everything in scripts EXCEPT auto_memory_cleaner.js
    Get-ChildItem -Path $scriptsDir | Where-Object { $_.Name -ne "auto_memory_cleaner.js" } | Remove-Item -Recurse -Force
}

# 5. Zip the folder
Write-Host "Zipping package..." -ForegroundColor Yellow
if (Test-Path $outputZip) {
    Remove-Item -Path $outputZip -Force
}
Compress-Archive -Path $outputDirName -DestinationPath $outputZip

# 6. Cleanup temp folder
Remove-Item -Path $outputDirName -Recurse -Force

Write-Host "Release package created successfully: $(Resolve-Path $outputZip)" -ForegroundColor Green
