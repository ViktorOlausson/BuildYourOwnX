#win project root path
$projectSubDirName = "TextEditorInC"
$scriptFolder = Split-Path -Parent $MyInvocation.MyCommand.Path

$mainFolder = Split-Path -Parent $scriptFolder

$projectRoot = Join-Path $mainFolder $projectSubDirName

$projectWslPath = wsl wslpath -a "$projectRoot"

$pwd = "cd .. && " + "cd TextEditorInC && " + "make test && " + "./test_editor && " + "make clean && " + "make main"


wsl -d Ubuntu sh -c "$pwd"

if($LASTEXITCODE -ne 0){
    Write-Error "Build or tests failed inside WSL (exit code $LASTEXITCODE)"
    exit $LASTEXITCODE
}