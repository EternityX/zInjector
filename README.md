# zInjector
A command-line utility for injecting dynamic link libraries into external processes.

## Usage
Grab the latest version from the releases page.

### Syntax
zInjector [dll path] [process name] [method number]

### Injection Methods
CreateRemoteThread - 1

#### Example
```
C:\zInjector.exe C:\Library.dll notepad.exe 1
```

## Todo
- Various methods of injection

# License
See LICENSE file
