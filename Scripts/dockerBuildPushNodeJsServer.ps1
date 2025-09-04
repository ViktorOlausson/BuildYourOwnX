Set-Location "C:\Users\vikto\source\git\BuildYourOwnX\NodeJsWebServer"

docker build -t viktorolausson/nodejswebserver:latest .

if($LASTEXITCODE -ne 0){
    Write-Error "Docker Build failed, stopping script"
}

docker push viktorolausson/nodejswebserver:latest

if($LASTEXITCODE -ne 0){
    Write-Error "Docker Push failed"
}

Write-Host "Docker build and push sucessfully" -ForegroundColor Green