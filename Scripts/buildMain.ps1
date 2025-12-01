$pwd = "cd .. && " + "cd TextEditorInC && " + "make test && " + "./test_editor && " + "make clean && " + "make main"

wsl -d Ubuntu sh -c "$pwd"

if($LASTEXITCODE -ne 0){
    Write-Error "Build or tests failed inside WSL (exit code $LASTEXITCODE)"
    exit $LASTEXITCODE
}