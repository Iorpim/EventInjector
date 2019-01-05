# EventInjector
Tool for injecting events into Windows Event log


-------------------------------------------------------------------
## Usage
EventInjector is a command line tool, for usage details use `EventInjector -h`.

"-" and "/" are interchangeable.

## Limitations
Windows allows for any user, even unprivileged, to write in Application log as any other user, System and Security, however, require permissions. For System log, administrator permissions are necessary, while Security log requires _generate auditing events_ permission, with running as _Local System_ being the simplest way to achieve all requirements with the least of registry/GPO modifications.

Application logs are indiscernible from legitimate events, but System events show minor differences as it appears to use a different API to register events.
