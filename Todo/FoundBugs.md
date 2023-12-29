# Bugs

## ResourceServer

**[10/09/2023] #1 Slient Failure**  
Failure of shader compilation is slient, no error messages produces.
ResourceSystem will keep wait raw resources to be produced and the entire engine will not shutdown correctly.

**[24/09/2023] #2 Log Stack Corrupted**  
In Base/Log/Log_Win32.cpp, TraceMessage.  
When user feed in a very long log message, stack of messageBuffer will overflow and cause corruption.  
**Temporary fixed**: increase the size of messageBuffer to 1024.

**[12/29/2023] #3 Visual Studio repeat linkage**  
When complile Esoterica.Base, it will trigger Base/RHI/RHIResourceBinding.cpp to build and produce a object file.
Then the linker will link it and cause multiple linkage warnings and some other extern linkaga error suck as unresolved symbol.
Temporary solution: change the name of RHIResourceBinding.cpp to RHIResourceBindings.cpp to avoid collision.
The prove evidence is inside the build log of visual studio. In D:\coding\mine\Cpp\Esoterica\Build\_Temp\x64_Debug\Esoterica.Base\Esoterica.Base.tlog\link.command.1.tlog,
you could find D:\CODING\MINE\CPP\ESOTERICA\BUILD\_TEMP\X64_DEBUG\ESOTERICA.BASE\RHIRESOURCEBINDING.OBJ but RHIResourceBinding.cpp already vanished!  