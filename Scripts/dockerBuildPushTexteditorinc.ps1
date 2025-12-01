& .\BuildAndRunTestEditor.ps1

Set-Location "C:\Users\vikto\source\git\BuildYourOwnX\TextEditorInC"

docker build -t viktorolausson/texteditorinc .

if($LASTEXITCODE -ne 0){
    Write-Error "Docker Build failed, stopping script"
}

docker push viktorolausson/texteditorinc

if($LASTEXITCODE -ne 0){
    Write-Error "Docker Push failed"
}

Write-Output "Docker build and push sucessfully"