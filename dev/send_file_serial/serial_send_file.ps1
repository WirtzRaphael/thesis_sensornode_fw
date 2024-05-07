$port = new-Object System.IO.Ports.SerialPort COM4, 9600, None, 8, one
$port.Open()
$file = Get-Content ".\testfile_text_4096bytes.txt"

foreach ($line in $file) {
    $port.WriteLine($line)
    Start-Sleep -Milliseconds 500 # Adjust sleep time as needed
}

$port.Close()
