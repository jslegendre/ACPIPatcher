# ACPIPatcher
An EFI application to add SSDTs and/or patch in your own DSDT

I made this tool because I wanted a way to use [RehabMans ACPI Debug tool](https://github.com/RehabMan/OS-X-ACPI-Debug) on my MacBook Pro without using Clover.  Although I made this with macOS in mind it will work with any OS along with any bootloader (provided your bootloader does not do its own ACPI patching).

## How to use:
Using this tool is fairly straighforward. Just place a folder titled `ACPI` in the same directory as `ACPIPatcher.efi` and it will search the folder for any `.aml` files.  Any file not named `DSDT.aml` will be added to the XSDT table as SSDTs while `DSDT.aml` will completely replace the OEM DSDT. Once you have everything in place just call `ACPIPatcher` from an EFI shell.

Your directory listing should look something like this:
```
FS0:> ls
<DIR> ACPI
      ACPIPatcher.efi

FS0:> ls ACPI/
      SSDT-TEST.aml
      DSDT.aml
```

## How to Build:
Place `ACPIPatcherPkg` in your EDKII folder and build as usual. Prebuilt binaries are provided however. 
`build -p ACPIPatcherPkg/ACPIPatcherPkg.dsc`

### To Do:
This is my first EFI application so I'm sure there is much work to be done here.  Things I can think of:

* Rewrite as a driver so the application does not need to be called before every boot
* Better memory management/error handling
