if (Test-Path "F:\PSP\GAME\psptoy") {
    Copy-Item build/EBOOT.PBP F:\PSP\GAME\psptoy\EBOOT.PBP
} else {
    New-BurntToastNotification -Text "PSP no conectada", "No se copiarán los archivos"
}