$command = "cd .. && " + "cd TextEditorInC && " + "make test && " + "./test_editor && " + "make clean"

wsl -d Ubuntu sh -c "$command"

if($LASTEXITCODE -ne 0){
    Write-Error "Build or tests failed inside WSL (exit code $LASTEXITCODE)"
    exit $LASTEXITCODE
}