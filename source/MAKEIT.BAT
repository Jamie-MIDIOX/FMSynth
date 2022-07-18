call setddk
nmake %1
call setms
nmake %1 /f fmcfg.mak
nmake %1 /f fmtask.mak
