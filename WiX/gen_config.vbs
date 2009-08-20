function gen

Set oShell = CreateObject("WScript.Shell")
Set oEnv = oShell.Environment("SYSTEM")
nCores = oEnv("NUMBER_OF_PROCESSORS")

cfgfile = Session.Property("CustomActionData") & "\my_custom_settings.conf"

' Put some code here for writing stuff to the file

end function

