# zInjector
A command-line utility for injecting dynamic link libraries into external processes.

# Building
Open 'zInjector.sln' in Visual Studio and build the solution in Release mode to create zInjector.exe.

# Usage

### Syntax
zInjector [dll path] [process name] [method number]

### Injection Methods
- CreateRemoteThread - 1
- NtCreateThreadEx - 2

#### Example
```
C:\zInjector.exe C:\Library.dll notepad.exe 1
```

# Todo
- Manual Mapping
- RtlCreateUserThread Injection
- Reflective Injection
- SetWindowsHookEx Injection
- QueueUserAPC Injection

# License
zInjector is licensed under the MIT License, see the LICENSE file for more information.
