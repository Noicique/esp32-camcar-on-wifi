$idfPath = "L:\Program\Espressif\frameworks\esp-idf-v5.3.1"
$pythonEnvPath = "L:\Program\Espressif\python_env\idf5.3_py3.11_env"
$pythonExe = "$pythonEnvPath\Scripts\python.exe"
$idfPyScript = "$idfPath\tools\idf.py"

$env:IDF_PATH = $idfPath
$env:IDF_PYTHON_ENV_PATH = $pythonEnvPath
$env:IDF_TOOLS_PATH = $idfPath
$env:IDF_PYTHON_CHECK_CONSTRAINTS = "0"
$env:Path = "$pythonEnvPath\Scripts;$idfPath\tools;$env:Path"

& $pythonExe $idfPyScript @args
